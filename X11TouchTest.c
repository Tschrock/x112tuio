#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>

/*

 Author: Tyler Schrock (tschrock123@gmail.com)

 Referances:
     http://who-t.blogspot.com/2009/05/xi2-recipes-part-1.html
     http://who-t.blogspot.com/2009/07/xi2-recipes-part-4.html
     http://who-t.blogspot.com/2011/12/multitouch-in-x-getting-events.html
     https://gist.github.com/akovalenko/4605420

*/

void handleEvent(XIDeviceEvent* event)
{
    printf("%d,%d,%d,%d\n", event->evtype, event->detail, (int) event->event_x, (int) event->event_y);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    Display *display = XOpenDisplay(NULL);
    int xi_opcode, event, error;

    if (!display)
    {
        printf("Can't open display");
        return -1;
    }

    if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &event, &error))
    {
        printf("X Input extension not available.\n");
        return -1;
    }

    int major=2, minor=2;
    if (XIQueryVersion(display,&major,&minor)!=Success)
    {
        printf("XI 2.2 not available. Server supports %d.%d\n", major, minor);
        return -1;
    }




    XIDeviceInfo *info;
    int i, j, ndevices;
    int foundDirectDevices = 0;
    int useDevice = 0;

    info = XIQueryDevice(display, XIAllDevices, &ndevices);

    for (i = 0; i < ndevices; i++)
    {
        XIDeviceInfo *dev = &info[i];
        int isDirectTouchDevice = 0;
        for (j = 0; j < dev->num_classes; j++)
        {
            XIAnyClassInfo *class = dev->classes[j];
            XITouchClassInfo *t = (XITouchClassInfo*)class;

            if (class->type != XITouchClass)
                continue;

            isDirectTouchDevice = (t->mode == XIDirectTouch);


        }
        if(isDirectTouchDevice == 1)
        {
            foundDirectDevices++;
            useDevice = dev->deviceid;
        }
    }

    if(foundDirectDevices < 1)
    {
        //fprintf(stderr, "Couldn't find a direct touch device!\n");
        return -1;
    }
    if(foundDirectDevices == 1)
    {
        //fprintf(stderr, "Found one direct touch device!\nListening to device %i.\n", useDevice);
    }
    if(foundDirectDevices > 1)
    {
        //fprintf(stderr, "Found multiple direct touch devices!\nListening to last found device %i.\n", useDevice);
    }



    Window root = DefaultRootWindow(display);


    XIEventMask eventmask;


    eventmask.deviceid = useDevice;
    eventmask.mask_len = XIMaskLen(XI_TouchBegin)
                         + XIMaskLen(XI_TouchUpdate)
                         + XIMaskLen(XI_TouchEnd);


    eventmask.mask = calloc(eventmask.mask_len, sizeof(char));
    if (!eventmask.mask)
    {
        printf("Out of memory");
        return -1;
    }


    XISetMask(eventmask.mask, XI_TouchBegin);
    XISetMask(eventmask.mask, XI_TouchUpdate);
    XISetMask(eventmask.mask, XI_TouchEnd);


    XISelectEvents(display, root, &eventmask, 1);
    XSync(display, False);


    while (1)
    {
        XEvent xevent;

        XNextEvent(display, &xevent);
        if (XGetEventData(display, &xevent.xcookie) &&
                (xevent.xcookie.type == GenericEvent) &&
                (xevent.xcookie.extension == xi_opcode))
        {
            handleEvent(xevent.xcookie.data);
            XFreeEventData(display, &xevent.xcookie);
        }
    }
}
