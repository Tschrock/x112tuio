#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    switch (event->evtype) {
    case XI_Motion:
        
        double *val;
        int i;
        val = event->valuators.values;
        for (i = 0; i < event->valuators.mask_len * 8; i++) {
            if (XIMaskIsSet(event->valuators.mask, i)) {
                if (i == 2) {
                    printf("%d\n", (int) *val);
                }
                *val++; 
            }
        } 

        fflush(stdout);
        break;
    case XI_TouchBegin:
    case XI_TouchUpdate:
    case XI_TouchEnd:
        break;
    }
}


int main(int argc, char *argv[])
{

    // Open the X display and check requirements

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


    // Query X Device info

    int ndevices;
    int foundDevices = 0;
    int device = 0;
    char* dname;

    XIDeviceInfo *info = XIQueryDevice(display, device, &ndevices);

    for (int i = 0; i < ndevices; ++i)
    {
        XIDeviceInfo *deviceInfo = &info[i];

        for (int j = 0; j < deviceInfo->num_classes; ++j)
        {
            XIAnyClassInfo *classInfo = deviceInfo->classes[j];

            if (classInfo->type != XITouchClass) continue;

            if (((XITouchClassInfo*)classInfo)->mode == XIDirectTouch)
            {
                foundDevices++;
                device = deviceInfo->deviceid;
                dname = deviceInfo->name;
            fprintf(stderr, "Found DirectTouch device with Id %i.\n", deviceInfo->deviceid);
            }
        }

        if(strstr(deviceInfo->name, "Tablet") != NULL) {
            foundDevices++;
            device = deviceInfo->deviceid;
            dname = deviceInfo->name;
            fprintf(stderr, "Found Tablet-named device with Id %i.\n", deviceInfo->deviceid);
        }
    }

    if(foundDevices < 1)
    {
        fprintf(stderr, "Couldn't find a device!\n");
        return -1;
    }

    if(foundDevices == 1) fprintf(stderr, "Found one device!\nListening to device %i: '%s'.\n", device, dname);

    if(foundDevices > 1) fprintf(stderr, "Found multiple devices!\nListening to last found device %i: '%s'.\n", device, dname);



    Window root = DefaultRootWindow(display);


    XIEventMask eventmask;


    eventmask.deviceid = device;
    eventmask.mask_len = XIMaskLen(XI_Motion)
                         + XIMaskLen(XI_TouchBegin)
                         + XIMaskLen(XI_TouchUpdate)
                         + XIMaskLen(XI_TouchEnd);
//                         + XIMaskLen(XI_ButtonPress)
//                         + XIMaskLen(XI_KeyPress);


    eventmask.mask = (unsigned char*) calloc(eventmask.mask_len, sizeof(char));
    if (!eventmask.mask)
    {
        printf("Out of memory");
        return -1;
    }


    XISetMask(eventmask.mask, XI_TouchBegin);
    XISetMask(eventmask.mask, XI_TouchUpdate);
    XISetMask(eventmask.mask, XI_TouchEnd);
//    XISetMask(eventmask.mask, XI_ButtonPress);
    XISetMask(eventmask.mask, XI_Motion);
//    XISetMask(eventmask.mask, XI_KeyPress);


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
            handleEvent((XIDeviceEvent*) xevent.xcookie.data);
            XFreeEventData(display, &xevent.xcookie);
        }
    }

}
