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

uint8_t STANDARD_LED_NUMLOCK = 1;
uint8_t STANDARD_LED_CAPSLOCK = 2;
uint8_t STANDARD_LED_SCROLLLOCK = 4;

void
set_leds_on_hidraw(char *path, int16_t vendor, int16_t product,
                   uint8_t standard_led_mask)
{
    int fd;
    struct hidraw_devinfo info = {0};
    uint8_t buf[2] = {0};

    fd = open(path, O_RDWR);
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
        /* To change the standard LEDs, we'll send an "output
         * report". This is a control transfer.
         *
         * hidraw lets use do this, too, but be aware that the kernel
         * driver keeps an internal state of the LEDs. It won't know
         * that we changed it and will overwrite us the next time the
         * user hits a key like numlock. Also, changing the LED does not
         * affect the actual state of that modifier, e.g. changing the
         * LED of caps lock does not activate caps lock on your Linux
         * host -- but it MIGHT trigger something in the keyboard.
         * Nobody knows.
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

    close(fd);
}

void
set_leds(int16_t vendor, int16_t product, uint8_t standard_led_mask)
{
    glob_t globres;
    size_t path_i;

    glob("/dev/hidraw*", GLOB_NOSORT, NULL, &globres);
    for (path_i = 0; path_i < globres.gl_pathc; path_i++)
    {
        set_leds_on_hidraw(globres.gl_pathv[path_i], vendor, product,
                           standard_led_mask);
    }
    globfree(&globres);
}

int
main(int argc, char **argv)
{
    uint8_t standard_led_mask = 0;
    int16_t vendor = 0x0911, product = 0x2188;
    int opt;

    while ((opt = getopt(argc, argv, "ncsv:p:")) != -1)
    {
        switch (opt)
        {
            case 'n': standard_led_mask |= STANDARD_LED_NUMLOCK; break;
            case 'c': standard_led_mask |= STANDARD_LED_CAPSLOCK; break;
            case 's': standard_led_mask |= STANDARD_LED_SCROLLLOCK; break;
            case 'v': vendor = atoi(optarg); break;
            case 'p': product = atoi(optarg); break;
            default: return 1;
        }
    }

    if (vendor == 0 || product == 0)
    {
        fprintf(stderr, "Invalid vendor or product ID\n");
        return 1;
    }

    set_leds(vendor, product, standard_led_mask);

    return 0;
}
