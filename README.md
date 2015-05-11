MultiTouch Stuff
================

X11TouchTest
------------
Listens to touch events from X11 and prints touch data to the console.  
Output format is: `touchType,touchId,xPos,yPos`  
touchType: 18 is Touch Down, 19 is Touch Update, 20 is Touch Up.  
touchId is a unique identifier for each touch.  

Needs: libX11, libXi

Processing_X11TouchTest
-----------------------
A simple demonstration that grabs output from X11TouchTest and shows it on-screen.

x112tuio
--------
A TUIO output Bridge that translates X11 touch events. Similar to mtdev2tuio, but it doesn't need root access to the device.

Needs: libX11, libXi, libpthread, and TUIO (https://github.com/mkalten/TUIO11_CPP)

```
Usage: x112tuio [options]

Options:
-k                    Don't start TUIO server (implies -t)
-t                    Print touches
-d device_id          Listen to device_id for touches
-s host               Start the TUIO server with the specified host
-p port               Start the TUIO server on the specified port
```
