import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.awt.Point;
import java.net.URISyntaxException;

int[][][] points = new int[5][30][2];
int[] ids = new int[5];

Point windowLocation = new Point(0, 0);


int TOUCH_START = 18;
int TOUCH_UPDATE = 19;
int TOUCH_END = 20;

OutputStream procIn = null;
InputStream procOut = null;
BufferedReader br = null;

void setup() {  
  size(displayWidth, displayHeight);
  for (int i = 0; i < points.length; ++i) {
    for (int j = 0; j < points[i].length; ++j) {
      for (int l = 0; l < points[i][j].length; ++l) {
        points[i][j][l] = -1;
      }
    }
  }
  for (int i = 0; i < ids.length; ++i) {
    ids[i] = -1;
  }

  colorMode(HSB, points.length, 255, 255, points[0].length*points[0].length);

  prepareExitHandler();

  try {
    Process process = Runtime.getRuntime().exec (sketchPath("../X11TouchTest"));
    procIn = process.getOutputStream();
    procOut = process.getInputStream();
    br = new BufferedReader(new InputStreamReader(procOut, "UTF-8"));
  }
  catch (IOException e)
  {
    e.printStackTrace();
  }

  thread("getNewLine");
}

void clearArrayVals(int arrIndex) {
  for (int j = 0; j < points[arrIndex].length; ++j) {
    points[arrIndex][j][0] = -1;
    points[arrIndex][j][1] = -1;
  }
}

void shiftAddArrayVals(int arrIndex, int newX, int newY) {
  int len = points[arrIndex].length;
  for (int j = 1; j < len; ++j) {
    points[arrIndex][j-1][0] = points[arrIndex][j][0];
    points[arrIndex][j-1][1] = points[arrIndex][j][1];
  }
  points[arrIndex][len-1][0] = newX;
  points[arrIndex][len-1][1] = newY;
}

int findId(int x) {
  for (int i = 0; i < ids.length; ++i) {
    if (ids[i] == x) {
      return i;
    }
  }
  return -1;
}

int click = 0;
boolean wasMousePressed = false;

void draw() {


  if (isShowing()) {
    windowLocation = getLocationOnScreen();
  }


  background(0);
  if (mousePressed) {
    if (!wasMousePressed) {
      click++;
      click %= points.length;
    }
    //shiftAddArrayVals(click, mouseX, mouseY);
  } else {
    if (wasMousePressed) {
      //clearArrayVals(click);
    }
  }
  wasMousePressed = mousePressed;

  for (int i = 0; i < points.length; ++i) {
    for (int j = 0; j < points[i].length; ++j) {
      if (points[i][j][0] != -1 && points[i][j][1] != -1) {
        noStroke();
        fill(i, 255, 255, j*j);
        ellipse(points[i][j][0] - windowLocation.x, points[i][j][1] - windowLocation.y, 50, 50);
      }
    }
  }
}


void getNewLine() {
  try {
    String sCurrentLine;
    while ( (sCurrentLine = br.readLine ()) != null) {
      String[] parts = sCurrentLine.split(",");
      int type = Integer.parseInt(parts[0]);
      int id = Integer.parseInt(parts[1]);
      int x = Integer.parseInt(parts[2]);
      int y = Integer.parseInt(parts[3]);

      int arrIndex = findId(id); // lookup the id

        if (arrIndex == -1) { //  We have a new id
        if (type == TOUCH_START) { // It's the start of a touch

          int emptyIndex = findId(-1); // Try to find an empty space
          if (emptyIndex == -1) { // Opps, no space
            System.out.println("No room for new touch " + id);
          } else { 
            ids[emptyIndex] = id; // Store this new id
            shiftAddArrayVals(emptyIndex, x, y); // Add this touch's data to points
          }
        }
      } else { // We have an existing id
        if ( type == TOUCH_UPDATE) { // We are just updating the touch
          shiftAddArrayVals(arrIndex, x, y); // Add this touch's data to points
        }
        if (type == TOUCH_END) { // We are getting rid of a touch
          clearArrayVals(arrIndex); // Clear this touch's data
          ids[arrIndex] = -1; // Remove this id so we can reuse this index
        }
      }
    }
  } 
  catch (IOException e) {
    e.printStackTrace();
  }
}

private void prepareExitHandler () {

  Runtime.getRuntime().addShutdownHook(new Thread(new Runnable() {

    public void run () {

      try {
        Process process = Runtime.getRuntime().exec ("pkill X11TouchTest");
      } 
      catch (IOException e) {
        e.printStackTrace();
      }
    }
  }
  ));
}

boolean sketchFullScreen() {
  return true;
}

