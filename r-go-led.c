#define _POSIX_C_SOURCE 2  /* for getopt() */

#include <fcntl.h>
#include <glob.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/* Choose one. */
enum RGoLEDColor
{
    RGO_LED_COLOR_UNCHANGED,
    RGO_LED_COLOR_RED,
    RGO_LED_COLOR_GREEN,
    RGO_LED_COLOR_YELLOW,
    RGO_LED_COLOR_OFF,
};

/* Bitmasks. */
uint8_t STANDARD_LED_NUMLOCK = 1;
uint8_t STANDARD_LED_CAPSLOCK = 2;
uint8_t STANDARD_LED_SCROLLLOCK = 4;

void
set_leds_on_hidraw(char *path, int16_t vendor, int16_t product,
                   enum RGoLEDColor rgo_color, bool change_standard_leds,
                   uint8_t standard_led_mask)
{
    int fd;
    struct hidraw_devinfo info = {0};
    uint8_t buf[8] = {0};
    uint8_t color_byte = 0;

    fd = open(path, change_standard_leds ? O_RDWR : O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "open %s: ", path);
        perror("");
        return;
    }

    if (ioctl(fd, HIDIOCGRAWINFO, &info) < 0)
    {
        fprintf(stderr, "ioctl for dev info %s: ", path);
        perror("");
        close(fd);
        return;
    }

    if (info.vendor == vendor && info.product == product)
    {
        if (rgo_color != RGO_LED_COLOR_UNCHANGED)
        {
            switch (rgo_color)
            {
                case RGO_LED_COLOR_RED:    color_byte = 0x01; break;
                case RGO_LED_COLOR_GREEN:  color_byte = 0x02; break;
                case RGO_LED_COLOR_YELLOW: color_byte = 0x03; break;
                case RGO_LED_COLOR_OFF:    color_byte = 0x04; break;
                case RGO_LED_COLOR_UNCHANGED: /* make compiler happy */ break;
            }

            /* This ioctl() sends a "feature report" to the device.
             *
             * hidraw devices allow us to send "control transfer"
             * requests to endpoint 0 *without* having to detach the
             * kernel driver. We can't send arbitrary data, but it's
             * good enough to control the special R-Go Tools LED on the
             * keyboard.
             *
             * bmRequestType: Fixed at 0x21 by Linux
             * bRequest: Fixed at 0x09 by Linux
             * wValue: High byte fixed at 0x03 by Linux (determined by
             *         the fact that we call the ioctl() to do a feature
             *         report)
             * wValue: Low byte (report ID) is the first byte from the
             *         payload (0x30) and it's also duplicated into the
             *         the actual payload
             * wIndex: Fixed by Linux, depends on which device we use
             *         ("interface_number")
             *
             * Remaining payload (0x91, ...) as documented by
             * manufacturer. The trailing 0x00 don't appear to be
             * strictly necessary, but that's what they told me to use,
             * so we'll to that.
             */
            buf[0] = 0x30;
            buf[1] = 0x91;
            buf[2] = color_byte;
            buf[3] = 0x00;
            buf[4] = 0x00;
            buf[5] = 0x00;
            buf[6] = 0x00;
            buf[7] = 0x00;
            if (ioctl(fd, HIDIOCSFEATURE(8), buf) < 0)
            {
                fprintf(stderr, "ioctl for sending feature report to %s: ", path);
                perror("");
            }
        }

        if (change_standard_leds)
        {
            /* To change the standard LEDs, we'll send an "output
             * report". This is a control transfer as well.
             *
             * hidraw lets use do this, too, but be aware that the
             * kernel driver keeps an internal state of the LEDs. It
             * won't know that we changed it and will overwrite us the
             * next time the user hits a key like numlock. Also,
             * changing the LED does not affect the actual state of that
             * modifier, e.g. changing the LED of caps lock does not
             * activate caps lock on your Linux host -- but it MIGHT
             * trigger something in the keyboard. Nobody knows.
             *
             * tl;dr: Use with caution.
             *
             * bmRequestType: Fixed at 0x21 by Linux
             * bRequest: Fixed at 0x09 by Linux
             * wValue: High byte fixed at 0x02 by Linux (determined by
             *         the fact that we call "write()")
             * wValue: Low byte (report ID) is the first byte from the
             *         payload (0x00), NOT duplicated into real payload
             * wIndex: Fixed by Linux, depends on which device we use
             *         ("interface_number")
             *
             * Remaining payload indicates state of each LED:
             *
             * - Bit 0: Numlock
             * - Bit 1: Caps lock
             * - Bit 2: Scroll lock
             *
             * Changing these standard LEDs is documented in numerous
             * places:
             *
             * https://www.usb.org/sites/default/files/documents/hid1_11.pdf
             * https://wiki.osdev.org/USB_Human_Interface_Devices#LED_lamps
             */
            buf[0] = 0x00;
            buf[1] = standard_led_mask;
            if (write(fd, buf, 2) < 0)
            {
                fprintf(stderr, "ioctl for sending output report to %s: ", path);
                perror("");
            }
        }
    }

    close(fd);
}

void
set_leds(int16_t vendor, int16_t product, enum RGoLEDColor rgo_color,
         bool change_standard_leds, uint8_t standard_led_mask)
{
    glob_t globres;
    size_t path_i;

    glob("/dev/hidraw*", GLOB_NOSORT, NULL, &globres);
    for (path_i = 0; path_i < globres.gl_pathc; path_i++)
    {
        set_leds_on_hidraw(globres.gl_pathv[path_i], vendor, product,
                           rgo_color, change_standard_leds, standard_led_mask);
    }
    globfree(&globres);
}

int
main(int argc, char **argv)
{
    enum RGoLEDColor rgo_color = RGO_LED_COLOR_UNCHANGED;
    uint8_t standard_led_mask = 0;
    bool change_standard_leds = false;
    int16_t vendor = 0x0911, product = 0x2188;
    int opt;

    while ((opt = getopt(argc, argv, "rgyoNCSOv:p:")) != -1)
    {
        switch (opt)
        {
            case 'r': rgo_color = RGO_LED_COLOR_RED; break;
            case 'g': rgo_color = RGO_LED_COLOR_GREEN; break;
            case 'y': rgo_color = RGO_LED_COLOR_YELLOW; break;
            case 'o': rgo_color = RGO_LED_COLOR_OFF; break;
            case 'N':
                standard_led_mask |= STANDARD_LED_NUMLOCK;
                change_standard_leds = true;
                break;
            case 'C':
                standard_led_mask |= STANDARD_LED_CAPSLOCK;
                change_standard_leds = true;
                break;
            case 'S':
                standard_led_mask |= STANDARD_LED_SCROLLLOCK;
                change_standard_leds = true;
                break;
            case 'O':
                standard_led_mask = 0;
                change_standard_leds = true;
                break;
            case 'v': vendor = atoi(optarg); break;
            case 'p': product = atoi(optarg); break;
        }
    }

    if (rgo_color == RGO_LED_COLOR_UNCHANGED && !change_standard_leds)
    {
        fprintf(stderr, "Need one of [-r|-g|-y|-o] to specify color "
                        "or one of [-N|-C|-S|-O] to change standard LED\n");
        return 1;
    }

    if (vendor == 0 || product == 0)
    {
        fprintf(stderr, "Invalid vendor or product ID\n");
        return 1;
    }

    set_leds(vendor, product, rgo_color, change_standard_leds, standard_led_mask);

    return 0;
}
