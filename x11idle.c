#include <stdio.h>
#include <X11/extensions/scrnsaver.h>

int
main()
{
    Display *dpy;
    XScreenSaverInfo *info;
    int event_base_return, error_base_return;

    if (!(dpy = XOpenDisplay(NULL)))
    {
        fprintf(stderr, "x11idle: Cannot open display\n");
        return 1;
    }

    if (!XScreenSaverQueryExtension(dpy, &event_base_return, &error_base_return))
    {
        fprintf(stderr, "x11idle: Cannot screensaver extension version \n");
        return 1;
    }

    info = XScreenSaverAllocInfo();
    XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), info);
    printf("%lu\n", info->idle);

    return 0;
}
