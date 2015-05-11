#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <string.h>

#include "TuioServer.h"
#include "TuioCursor.h"

/*

 Author: Tyler Schrock (tschrock123@gmail.com)

 Referances:
     http://who-t.blogspot.com/2009/05/xi2-recipes-part-1.html
     http://who-t.blogspot.com/2009/07/xi2-recipes-part-4.html
     http://who-t.blogspot.com/2011/12/multitouch-in-x-getting-events.html
     https://gist.github.com/akovalenko/4605420
     http://www.tuio.org/api/cpp/main.html

*/

struct CursorId
{
    int id;
    TUIO::TuioCursor *cursor;
    CursorId(int i, TUIO::TuioCursor *c)
    {
        id = i;
        cursor = c;
    }
};

void showHelp()
{
    std::cout << "Usage: x112tuio [options]" << std::endl
              << "" << std::endl
              << "Options:" << std::endl
              << "-k                    Don't start TUIO server (implies -t)" << std::endl
              << "-t                    Print touches" << std::endl
              << "-d device_id          Listen to device_id for touches" << std::endl
              << "-s host               Start the TUIO server with the specified host" << std::endl
              << "-p port               Start the TUIO server on the specified port" << std::endl
              << "" << std::endl;
}

CursorId* getCursorIdFromList(const int id);
void handleEvent(XIDeviceEvent* event);

TUIO::TuioServer *tuioServer;
TUIO::TuioCursor *cursor;
std::list<CursorId*> cursorIdList;

bool runServer = true;
bool printTouches = false;
int device = 0;
char* host = "localhost";
int port = 0;
int dWidth = 1;
int dHeight = 1;

int main(int argc, char *argv[])
{

    // Check CLI Arguments

    int c;
    opterr = 0;
    while ((c = getopt (argc, argv, "kthd:s:p:")) != -1)
        switch (c)
        {
        case 'k':
            runServer = false;
            printTouches = true;
            break;
        case 't':
            printTouches = true;
            break;
        case 'h':
            showHelp();
            return 0;
            break;
        case 'd':
            device = atoi(optarg);
            break;
        case 's':
            host = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                         "Unknown option character `\\x%x'.\n",
                         optopt);
            showHelp();
            return 1;
        default:
            abort ();
        }


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
            }
        }
    }

    if(foundDevices < 1)
    {
        fprintf(stderr, "Couldn't find a direct touch device!\n");
        return -1;
    }

    if(foundDevices == 1) fprintf(stderr, "Found one direct touch device!\nListening to device %i.\n", device);

    if(foundDevices > 1) fprintf(stderr, "Found multiple direct touch devices!\nListening to last found device %i.\n", device);


    // Grab the root window and set up the event mask/filter

    Window root = DefaultRootWindow(display);

    XWindowAttributes xwAttr;
    XGetWindowAttributes( display, root, &xwAttr );
    dWidth = xwAttr.width;
    dHeight = xwAttr.height;

    XIEventMask eventmask;

    eventmask.deviceid = device;
    eventmask.mask_len = XIMaskLen(XI_TouchBegin)
                         + XIMaskLen(XI_TouchUpdate)
                         + XIMaskLen(XI_TouchEnd);

    eventmask.mask = (unsigned char*) calloc(eventmask.mask_len, sizeof(char));

    if (!eventmask.mask)
    {
        printf("Out of memory"); // Yea, cause this is likely to happen (Hint: sarcasm)
        return -1;
    }

    XISetMask(eventmask.mask, XI_TouchBegin);
    XISetMask(eventmask.mask, XI_TouchUpdate);
    XISetMask(eventmask.mask, XI_TouchEnd);

    XISelectEvents(display, root, &eventmask, 1);
    XSync(display, False);


    // Start TUIO server
    if(runServer)
    {
        if ((strcmp(host,"localhost")==0) && (port==0)) tuioServer = new TUIO::TuioServer();
        else tuioServer = new TUIO::TuioServer(host, port);
    }


    // Start the main loop

    while (1)
    {
        XEvent xevent;
        XNextEvent(display, &xevent);
        if (XGetEventData(display, &xevent.xcookie) && (xevent.xcookie.type == GenericEvent) && (xevent.xcookie.extension == xi_opcode))
        {
            handleEvent((XIDeviceEvent*) xevent.xcookie.data);
            XFreeEventData(display, &xevent.xcookie);
        }
    }
}

void handleEvent(XIDeviceEvent* event)
{
    if(printTouches)
    {
        printf("%d,%d,%d,%d\n", event->evtype, event->detail, (int) event->event_x, (int) event->event_y);
        fflush(stdout);
    }

    if(runServer)
    {

        TUIO::TuioTime currentTime = TUIO::TuioTime::getSessionTime();
        tuioServer->initFrame(currentTime);

        switch(event->evtype)
        {
        case XI_TouchBegin:
            cursor = tuioServer->addTuioCursor((float) event->event_x / dWidth,(float) event->event_y / dHeight);
            cursorIdList.push_back(new CursorId(event->detail, cursor));
            break;

        case XI_TouchUpdate:
            tuioServer->updateTuioCursor(getCursorIdFromList(event->detail)->cursor,(float) event->event_x / dWidth,(float) event->event_y / dHeight);
            break;

        case XI_TouchEnd:
            CursorId *id = getCursorIdFromList(event->detail);
            tuioServer->removeTuioCursor(id->cursor);
            cursorIdList.remove(id);
            delete id;
            break;
        }

        tuioServer->stopUntouchedMovingCursors();
        tuioServer->commitFrame();
    }
}

CursorId* getCursorIdFromList(const int id)
{
    CursorId *match = NULL;
    for (std::list<CursorId*>::iterator tuioCursor = cursorIdList.begin(); tuioCursor!=cursorIdList.end(); tuioCursor++)
    {
        if ((*tuioCursor)->id == id)
        {
            match = *tuioCursor;
            break;
        }
    }
    return match;
}

