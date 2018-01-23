#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <SD.h>
#include <SPI.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//Define pins for TouchScreen
#define YP A3
#define XM A2
#define YM 9
#define XP 8
//Define pins for LCD
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4
//Define pin for SD
#define PIN_SD_CS 10
//Touch Minimum and Maximum
#define TS_MINX 135
#define TS_MAXX 920
#define TS_MINY 90
#define TS_MAXY 910
#define MINPRESSURE 10
#define MAXPRESSURE 1000
//Initialize LCDScreen
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
//Initialize TouchScreen
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
//Initialize Global Variables
//previous and current coordinates pressed, and pressure
int x, y, pressure, prevX, prevY;
//variables to check if the screen is currently pressed, time it has not been pressed
int pressed = 0, count = 0;
//current file number
int numf = 0;
//variable to break out of all the loops when person is done drawing
int br=0;
//variable for the size of the colour boxes
int button=tft.width()/5;
//structure defines the colour for the arduino and for the file
typedef struct {
  int ard, html;
} colour;
//variable that holds the current colour
colour currentColour;
//Array of colour structure defines hex code for the arduino and the file (white, black, blue, red, green)
colour col[] = {{0xFFFF, 0xFFF}, {0x0000, 0x000}, {0x001F, 0x00F}, {0XF800, 0xF00}, {0x07E0, 0x0F0}};
void setup(void) {
  
  //setting up the screen
  tft.reset();
  uint16_t identifier = 0x9341;
  tft.begin(identifier);
  tft.setRotation(2);
  tft.fillScreen(col[0].ard);
  currentColour = col[2];
  pinMode(13, OUTPUT);
  pinMode(10, OUTPUT);
  SD.begin(PIN_SD_CS);
  //Drawing the colour panel
  for(int i = 0, j = 1; i<tft.width()-button;i+=button, j++){
    tft.fillRoundRect(i,0,button,button,5,col[j].ard);
  }
  
  //Make the exit button
  tft.drawRoundRect(tft.width()-button,0,button,button,5,col[1].ard);
  tft.setCursor(tft.height()-button,tft.width()-2*button/3);
  tft.setTextColor(col[3].ard);
  tft.setTextSize(2);
  tft.setRotation(1);
  tft.print("Exit");
  tft.setRotation(2);
  //checking which number of file to start with based on which files already exist
  File root=SD.open("/");
  while(1){
   File entry=root.openNextFile();
   //if the there are no more files break out of the loop
   if(!entry) break;
   //set pointer to hold name of file and variable to hold length
   char *src=entry.name();
   int len=strlen(src);
   //variable to hold max file number currently created
   int curr=0;
   
   //If the file starts with IMG then get the length of the number
   if(!strncmp(src, "IMG",3)){
    int j=0;
    src+=3;
    for(;j<len;j++){
      if(src[j]=='.') break;
    }
    //convert file number to an integer value
    char num[len];
    strncpy(num, src, j);
    num[j]='/0';
    for(int i=0; i<len-7; i++){
      curr=10*curr+num[i]-48;
    }
   }
   
   //if the file number found is greater than the current file number set the current file number to be one greater than the file number found
   if(curr>=numf) numf=curr+1;
   //close the file
   entry.close();
  }
  
}
void loop() {
  
  //Draw the start button and wait until user presses
  while(1){
    tft.drawRoundRect(button,2*button,tft.width()-2*button,tft.height()-3*button,5,col[1].ard);
    tft.setCursor(button*3/2,button*2);
    tft.setTextColor(col[2].ard);
    tft.setTextSize(5);
    tft.setRotation(1);
    tft.print("Start");
    tft.setRotation(2);
    digitalWrite(13, HIGH);
    //Get coordinates
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    y = (tft.height() - map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
    pressure = p.z;
    //if the screen is pressed break
    if(pressure>MINPRESSURE&&pressure<MAXPRESSURE){
      tft.fillRect(0,button,tft.width(),tft.height(),col[0].ard);
      break;
    }
  }
  
 //variable that saves the hex colour that is currently being used as a string
  char color[4];
  //assigns a name to the file
  char filename[10];
  sprintf(filename, "img%d.svg", numf);
  //initializes the file
  File f = SD.open(filename, FILE_WRITE);
  
  //writes to the file the starting tag with the max of the height and width
  f.println("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"");
  f.print(tft.height());
  f.print("\" height=\"");
  f.print(tft.width());
  f.print("\">\n");
  
  //writes html code to the file until the user presses the exit button, each loop is a line
  while (1) {
    //prints the selected color hex code to the svg file
    f.print("<polyline style=\"fill:none;stroke:#");
    sprintf(color, "%03x", currentColour.html);
    f.print(color);
    f.print(";stroke-width:3\" points=\"");
    //writes the line coordinates to the file until the user stops pressing the screen, each loop is a set of coordinates
    while (1) {
      //gets the coordinates from the screen
      digitalWrite(13, HIGH);
      TSPoint p = ts.getPoint();
      digitalWrite(13, LOW);
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      y = (tft.height() - map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
      pressure = p.z;
      //determines if the screen is being pressed or not
      if (pressure < MINPRESSURE || pressure > MAXPRESSURE) {
        //if the person has not pressed the screen for a certain amount of time (50 loops) break out of the loop
        if (pressed && count > 50) {
          pressed = 0;
          break;
        } else if (pressed) count++;
        //runs back to get new input
        continue;
      }
      //colour buttons or the exit button is pressed
      if (y < button) {
        //determines which button was pressed
        for(int i=0,j=1;i<tft.width();i+=button,j++){
          //if the exit button is pressed break from both loops and erase the screen
          if(x>4*button&&x<5*button){
            br=1;
            tft.fillRect(0,button,tft.width(),tft.height(),col[0].ard);
            break;
          }
          //if one of the colour buttons was pressed set br to 2 so that it breaks for the inner while loop
          else if (x>i&&x<i+button) currentColour=col[j];
        }
        //break from the 'line loop' to set a new line with the new color
        break;
        
      } else {
        //draw a line between the coordinates pressed on the screen
        if (pressed) tft.drawLine(prevX, prevY, x, y, currentColour.ard);
        //set previous x and y variables to thr current ones, and reset count
        prevY = y;
        prevX = x;
        count = 0;
        pressed = 1;
        //writes the line drawn on the screen to the file
        f.print(tft.height()-y); f.print(","); f.print(x); f.print(" ");
      } 
    }
    //end the line in the file
    f.println("\"/>");
    //break out of loop and reset break variable to 0 if exit button was pressed
    if (br) {
      br=0;
      break;
    }
    
  }
  
  //prints out the closing tag of the file
  f.print("</svg>\n\n");
  f.close();
  //increment the file number
  numf++;
}
