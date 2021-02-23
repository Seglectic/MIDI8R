/* ------------------------------------- Libraries ------------------------------------ */

#include <Arduino.h>                  //Arduino SDK for ESP platform
#include <SPI.h>                      //Serial Peripheral Interface for Serial comms
#include <Wire.h>                     //I²C library (2 Wire Interface)
#include <Adafruit_GFX.h>             //Display graphics library core
#include <Adafruit_SSD1306.h>         //Library for SSD1306 OLED display
#include <Adafruit_I2CDevice.h>       //Adafruit I²C, SPI, UART abstraction layer
//#include <FS.h>

/* ----------------------------------- DISPLAY SETUP ---------------------------------- */

// ┌──────────────────────────────────────────────────────────────┐
// │  Not sure why the Adafruit library has this display          │
// │  listed with the wrong screen address, but should be '0x3C'  │
// │                                                              │
// │  This display is 128x64 with a 16px tall yellow row across   │
// │  the top, separated by a 1px border.                         │
// └──────────────────────────────────────────────────────────────┘

#define SCREEN_WIDTH 128      // OLED display width, in pixels
#define SCREEN_HEIGHT 64      // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C   // i²C display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, LED_BUILTIN);

/* ------------------------------------ Pico Puddle ----------------------------------- */

const int cCount = 8;         // Amount of critters
int critterX[cCount];         // X location
int critterY[cCount];         // Y location
int cXVel[cCount];            // X Velocity
int cYVel[cCount];            // Y Velocity
int cTimer[cCount];           // Movement update timer
int cTime[cCount];            // Current movement time
int cSize[cCount];            // Critter Size

//Initialize the 'puddle' critters
void picoInit(){
  for(int i=0;i<cCount;i++){
    cSize[i] = random(1,4);                                              //Set critter size
    critterX[i] = random(cSize[i],127-cSize[i]);                         // Init a random location within bounds
    critterY[i] = random(16+cSize[i],63-cSize[i]);                       // 
    cXVel[i] = random(-1,2);                                             // Init a random velocity
    cYVel[i] = random(-1,2);                                             //
    cTimer[i] = random(50,100);                                          // Arbitrary random update time limit
  }
}

void picoPuddle(){
  for(int i=0;i<cCount;i++){
    display.drawCircle(critterX[i],critterY[i],cSize[i],BLACK);          // Erase last position
    if(cTime[i]>cTimer[i]){                                              // Update velocity when timer is up
      cTime[i] = 0;
      cXVel[i] =random(-1,2);
      cYVel[i] =random(-1,2);
    }
    critterX[i]+=cXVel[i];                                               //Increment velocities
    critterY[i]+=cYVel[i];                                               //
    cTime[i]+=5;                                                         //Increment timer  
    //Check boundary collisions
    if(critterX[i]<cSize[i]){critterX[i]=cSize[i];} 
      else if(critterX[i]>127-(cSize[i])){critterX[i]=127-(cSize[i]);}
    if(critterY[i]<16+(cSize[i])){critterY[i]=16+(cSize[i]);}
      else if(critterY[i]>63-(cSize[i])){critterY[i]=63-(cSize[i]);}  
    display.drawCircle(critterX[i],critterY[i],cSize[i],WHITE);          //Draw critter
  }
}

/* -------------------------------- Sine Wave Header -------------------------------- */

float Wave        = 0;    // Main sin oscillator
float waveTimer   = 0;    // Dictates movement of wave over time
float waveSpeed   = 0.1;  // Speed timer increments
int   waveAmp     = 8;    // Amplitude
int   waveCenter  = 7;    // Center location of wave

void waveDisplay(){
  display.fillRect(0,0,128,16,BLACK);
  for(int x=0;x<127;x++){
    waveTimer+=waveSpeed;
    Wave = sin(waveTimer);
    if(waveTimer>=3600){waveTimer=0;}
    if(x%2){display.drawPixel(x,7+(-Wave*waveAmp),WHITE);}
    if(x%4){display.drawPixel(x,7+(Wave*waveAmp),WHITE);}
  }
}

/* ----------------------------------- Staff Display ---------------------------------- */

// ┌────────────────────────────────────────────────────────────┐
// │  Creates a music staff to display notes received via midi  │
// └────────────────────────────────────────────────────────────┘
int nX[50];   //note X locations
int nY[50];   //note Y locations
int nM[50];   //note symbol modifiers

int nCount = 0;   //Current # of notes on screen
int debugTimer = 0;
int scrollSpeed = 3; // Speed notes fly backwards
byte channel = 1; // TODO make the midi channel be 1001nnnn or whatever where channel is nnnn

//Note MIDI value array starting from C3 (for now) (to C4 for now) (24 values)
int nID[] = {60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83};
//Which gets to note position on staff in pixels
int nPos[]= {40,40,38,38,36,34,34,32,32,30,30,28,26,26,24,22,22,20,20,18,18,16,16,14};
//Note symbol modifier 0=nothing 1=# 2=b 3=-
int nMod[]= {3,1,0,1,0,0,1,0,1,0,2,0,0,1,0,1,0,0,1,0,1,0,2,0,0,1,0,1,0,0,1,0,1,0,2,3};
 

void staffInit(){
  for(int i=0;i<50;i++){
    nX[i] = -20;
    nY[i] = -20;
  }
}

void staff(){

  //Draw both staffs
  for(int y=0; y<5; y++){
    display.drawLine(0,20+(y*4),127,20+(y*4),WHITE);
    display.drawLine(0,45+(y*4),127,45+(y*4),WHITE);
    //Draw note counter
    // display.drawRect(0,0,10,8,BLACK);
    // display.setCursor(0,0);
    // display.println(nCount); //Print note buffer pos TODO: Setup a third array for whether they're active
  }

  //Updates notes displayed and moves them down the staff
  //TODO: Needs to only loop nCount times, and remove when offscreen
  for(int i=0; i<50 ;i++){
    if(nX[i]<-10){continue;} //Don't update note if it's offscreen else will crash when >50 notes
    display.fillCircle(nX[i],nY[i],3,BLACK);
    display.setTextColor(BLACK);
    if(nM[i]==1){display.setCursor(nX[i]-10,nY[i]-4); display.print("#");}
    if(nM[i]==2){display.setCursor(nX[i]-10,nY[i]-4); display.print("b");}
    if(nM[i]==3){display.drawLine(nX[i]-5,nY[i],nX[i]+5,nY[i],BLACK);}
    nX[i] -= scrollSpeed;
    display.fillCircle(nX[i],nY[i],3,WHITE);
    display.setTextColor(WHITE);
    if(nM[i]==1){display.setCursor(nX[i]-10,nY[i]-4); display.print("#");}
    if(nM[i]==2){display.setCursor(nX[i]-10,nY[i]-4); display.print("b");}
    if(nM[i]==3){display.drawLine(nX[i]-5,nY[i],nX[i]+5,nY[i],WHITE);}
    
  }

}

//Places a note on the staff grabbign note data from: http://computermusicresource.com/midikeys.html
void drawNote(int note){
  //Need to map the note# from midi to a position on the screen,
  for(int i=0; i<sizeof(nID)/sizeof(nID[0]); i++){
    if(note == nID[i]){ //If note is in our map
      nX[nCount] = 127;
      nY[nCount] = nPos[i];
      nM[nCount] = nMod[i];
      break;
    }
  }
  nCount++;
  if(nCount>49){nCount=0;} //If 50 are used, maybe 0 is off-screen by now
}

/* ------------------------------------ MIDI Input ------------------------------------ */
byte midiBuf[3];
byte command;
byte note;
byte velocity;
void getMIDI(){
  display.drawRect(0,0,40,8,BLACK); //Clear display area
  display.setCursor(0,0);
  //display.print("Meow1234");
  if(Serial.available()>3){
    display.print(Serial.available()); //Print amt waiting in Serial
    for(int m=0;m<3;m++){
      midiBuf[m] = Serial.read();
      //if(midiBuf[m]<83){drawNote(midiBuf[m]);} //Super hacky..and ugly.
      if(midiBuf[m]==144){
        note = Serial.read();
        velocity = Serial.read();
        if(velocity>0){
          drawNote(note);
        }     
      }
    }

  }
}

/* ------------------------------------------------------------------------------------ */
/* ------------------------------------ MAIN SETUP ------------------------------------ */
/* ------------------------------------------------------------------------------------ */

void setup() {
  Serial.begin(31250);
  
  // ┌─────────────────┐
  // │  Display Setup  │
  // └─────────────────┘
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextSize(1);                         // "1:1 pixel scale" 
  display.clearDisplay();
  display.setTextColor(WHITE);                    //Defaults to black
  
  // ┌──────────┐
  // │  Splash  │
  // └──────────┘
  display.setTextSize(2);
  delay(1000);
  display.clearDisplay();
  display.display();
  //Scroll in title
  for(int x=-80; x<5; x+=5){
    display.fillRect(x,0,100,30,BLACK);
    display.setCursor(x,0);
    display.print("Midi8R");
    display.display();
    delay(10);
  }
  delay(200);
  display.setTextSize(1);

  staffInit();
  picoInit();
}

void loop() {
    debugTimer+=random(1,25);
  
  if(debugTimer>200){
    //drawNote(random(60,83));
    debugTimer=0;
  }
  waveDisplay();
  staff();
  getMIDI();
  //picoPuddle();
  display.display();
}