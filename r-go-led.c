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

enum RGoLEDColor
{
    RGO_LED_COLOR_RED,
    RGO_LED_COLOR_GREEN,
    RGO_LED_COLOR_YELLOW,
    RGO_LED_COLOR_OFF,
};

void
set_leds_on_hidraw(char *path, int16_t vendor, int16_t product,
                   enum RGoLEDColor rgo_color)
{
    int fd;
    struct hidraw_devinfo info = {0};
    uint8_t buf[8] = {0};
    uint8_t color_byte = 0;

    fd = open(path, O_RDONLY);
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
        switch (rgo_color)
        {
            case RGO_LED_COLOR_RED:    color_byte = 0x01; break;
            case RGO_LED_COLOR_GREEN:  color_byte = 0x02; break;
            case RGO_LED_COLOR_YELLOW: color_byte = 0x03; break;
            case RGO_LED_COLOR_OFF:    color_byte = 0x04; break;
        }

        /* This ioctl() sends a "feature report" to the device.
         *
         * hidraw devices allow us to send control transfer requests to
         * endpoint 0 *without* having to detach the kernel driver. We
         * can't send arbitrary data, but it's good enough to control
         * the special R-Go Tools LED on the keyboard.
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
         * Remaining payload (0x91, ...) as documented by manufacturer.
         * The trailing 0x00 don't appear to be strictly necessary, but
         * that's what they told me to use, so we'll to that.
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

    close(fd);
}

void
set_leds(int16_t vendor, int16_t product, enum RGoLEDColor rgo_color)
{
    glob_t globres;
    size_t path_i;

    glob("/dev/hidraw*", GLOB_NOSORT, NULL, &globres);
    for (path_i = 0; path_i < globres.gl_pathc; path_i++)
    {
        set_leds_on_hidraw(globres.gl_pathv[path_i], vendor, product, rgo_color);
    }
    globfree(&globres);
}

int
main(int argc, char **argv)
{
    enum RGoLEDColor rgo_color = RGO_LED_COLOR_OFF;
    int16_t vendor = 0x0911, product = 0x2188;
    int opt;

    while ((opt = getopt(argc, argv, "rgyv:p:")) != -1)
    {
        switch (opt)
        {
            case 'r': rgo_color = RGO_LED_COLOR_RED; break;
            case 'g': rgo_color = RGO_LED_COLOR_GREEN; break;
            case 'y': rgo_color = RGO_LED_COLOR_YELLOW; break;
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

    set_leds(vendor, product, rgo_color);

    return 0;
}
