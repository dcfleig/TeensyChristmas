#include <Arduino.h>
#include <stdint.h>
#include <math.h>
#include <Time.h>
#include <TimeLib.h>
#include <SmartMatrix3.h>
#include <MsTimer2.h>

#include "gimpbitmap.h"
#include "santa_anim.c"
#include "snow_scene_pink.c"
#include "MerryChristmas.c"


//
// ******************** Matrix Setup *******************************
//

#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
const uint8_t kMatrixWidth = 128;        // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 32;       // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = 48;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;   // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options

const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);

#define TIME_HEADER  "T"   // Header tag for serial time sync message

const int ledPin = 13;

const uint8_t santaFrameHeight = 13;
int santaX = 128;
int santaY = 5;
boolean flipSantaHorizontal = false;
boolean flipSantaVertical = false;
uint8_t santaPathMode = 0;
int8_t pathIndexOffset = 1;
int8_t pathRandomizer = random(100);
long santaDelayMillis = millis();

//const int defaultBrightness = 100*(255/100); // full brightness
const int defaultBrightness = 50*(255/100);    // half brightness
//const int defaultBrightness = 15*(255/100);  // dim: 15% brightness

const rgb24 gold = {0xf0, 0xe0, 0x00};
const rgb24 yellow = {0xff, 0xff, 0x00};
const rgb24 red = { 0xff, 0x00, 0x00};
const rgb24 white = { 0xff, 0xff, 0xff};
const rgb24 lgrey = { 0xd6, 0xd6, 0xd6};
const rgb24 dgrey = { 0xc0, 0xc0, 0xc0};
const rgb24 green = { 0x00, 0xff, 0x00};
const rgb24 blue = { 0x00, 0x00, 0xff};
const rgb24 black = { 0x00, 0x00, 0x00};
const rgb24 defaultBackgroundColor = { 0x00, 0x00, 0x00};

const int lightColorCount = 5;
const rgb24 lightColors[] = {red,yellow,green,blue};

//
// ******************** App Globals *******************************
//

typedef SM_RGB (*PixelColorTransformFunction) (const int16_t x, const int16_t y, SM_RGB pixel);

enum DisplayMode {
	christmasCountdown,
	merryChristmas
}; 

DisplayMode displayMode = christmasCountdown;

time_t eventTime;

// the setup() method runs once, when the sketch starts
void setup() {
		
	Serial.begin(115200);
	Serial.println(F("Starting display!\n"));

  // Init serial connection to esp8266
  Serial1.begin(115230);
  setSyncProvider(getESP8266Time);
  setSyncInterval(36000);
  setTime(getESP8266Time());  

  pinMode(ledPin, OUTPUT);

  matrix.addLayer(&backgroundLayer); 
  matrix.begin();

	backgroundLayer.setBrightness(defaultBrightness);
	backgroundLayer.enableColorCorrection(true);
	backgroundLayer.fillScreen(defaultBackgroundColor);

  eventTime = getPartyTime();

  randomSeed(analogRead(14));

  if (now() - getPartyTime() < 0) eventTime = getPartyTime();
  else eventTime = getChristmasTime();

  //eventTime = now() + 10;
}

void loop() {
	
	time_t remainingTime = eventTime - now();
	if (remainingTime < 0L) displayMode = merryChristmas;
  
	switch(displayMode) {
		case christmasCountdown:
      backgroundLayer.fillScreen(black);
      drawBitmap(0,0,(const gimp_bitmap*)&snow_scene, snowscene);
  		drawCountdown(remainingTime);
  		drawCountdownText();
      drawSanta();
		break;
		case merryChristmas:
      backgroundLayer.fillScreen(black);
      drawBitmap(0,0,(const gimp_bitmap*)&merry_christmas, randomflash);
      drawSanta();
		break;
	}
	backgroundLayer.swapBuffers();
}

void drawSanta() {

  nextSantaPosition();
  drawBitmap(santaX, santaY, (const gimp_bitmap*)&santa_anim, doNothing, flipSantaVertical, flipSantaHorizontal, santaFrameHeight, random(6), true);

}

void nextSantaPosition() {
  
  if (pathIndexOffset > 1) {
    flipSantaHorizontal = true;    
  } else {
     flipSantaHorizontal = false;      
  }
  
  const uint8_t pathCount = 10;

  switch(santaPathMode) {
    case 0:
      santaX = santaX + pathIndexOffset;
      santaY = 4 * sin(santaX/8) + 5;
    break;
    case 1:
      santaX = santaX + pathIndexOffset;
      santaY = 4 * sin(santaX/20) + 10;
    break;
    case 2:
      santaX = santaX + pathIndexOffset;
      santaY = 5;
    break;
    case 3:
      santaX = santaX + pathIndexOffset;
      santaY = santaY + random(3) - 1;
    break;
    case 5:
      santaX = santaX + (pathIndexOffset*2);
      santaY = floor(santaX / 4) - (pathRandomizer / 5);
    break;
    case 6:
      santaX = santaX + pathIndexOffset;
      santaY = 4 * sin(santaX/8) + 5;
    break;
    case 7:
      santaX = santaX + random(4) - 1;
      santaY = santaY + random(3) - 1;
    break;
    default:
      santaPathMode = random(pathCount - 1);
    break;
  }


  if (santaX > kMatrixWidth + 80) {
        pathIndexOffset=-1 * (random(4) + 1);
        santaPathMode = random(pathCount - 1);
        pathRandomizer = random(100);
        flipSantaVertical = random(10) > 6;
  }

  if (santaX < -80) {
        pathIndexOffset=random(4) + 1;
        santaPathMode = random(pathCount - 1);
        pathRandomizer = random(100);
        flipSantaVertical = random(10) > 6;
  }

  if (random(1000) > 995) {
    pathIndexOffset = pathIndexOffset * -1;
    pathRandomizer = random(100);
  }
}

void drawCountdownText() {
	backgroundLayer.setFont(font5x7);
	backgroundLayer.drawString( 7, 0, gold, "days");
	backgroundLayer.drawString( 36, 0, gold, "hours");
	backgroundLayer.drawString( 71, 0, gold, "mins");
	backgroundLayer.drawString( 102, 0, gold, "secs");
}

void drawCountdown(time_t remainingTime) {
	backgroundLayer.setFont(font8x13);

	int n;
	char digits[2];

	// prints the duration in days, hours, minutes and seconds
	//if(remainingTime >= SECS_PER_DAY){
	
	n = sprintf (digits, "%02d", remainingTime / SECS_PER_DAY);
	
	backgroundLayer.drawString( 9, 9, gold, digits);
	//Serial.print(digits);
	//Serial.print(" day(s) ");
	remainingTime = remainingTime % SECS_PER_DAY;
	//}
	//if(remainingTime >= SECS_PER_HOUR){
	
	n = sprintf (digits, "%02d", remainingTime / SECS_PER_HOUR);
	
	backgroundLayer.drawString(40, 9, gold, digits);
	//Serial.print(digits);
	//Serial.print(" hour(s) ");
	remainingTime = remainingTime % SECS_PER_HOUR;
	//}
	//if(remainingTime >= SECS_PER_MIN){
	n = sprintf (digits, "%02d", remainingTime / SECS_PER_MIN);

	backgroundLayer.drawString(72, 9, gold, digits);
	//Serial.print(digits);
	//Serial.print(" minute(s) ");
	remainingTime = remainingTime % SECS_PER_MIN;
	//}

	n = sprintf (digits, "%02d", remainingTime);

	backgroundLayer.drawString(104,9, gold, digits);
	//Serial.print(digits);
	//Serial.print(" second(s) \n");
}

void drawBitmap(int16_t x, int16_t y, const gimp_bitmap* bitmap, PixelColorTransformFunction colorTransform) {
  drawBitmap(x,y,bitmap, colorTransform, false, false, bitmap->height, 0, true);
}

void drawBitmap(int16_t x, int16_t y, const gimp_bitmap* bitmap, PixelColorTransformFunction colorTransform, boolean flipVertical, boolean flipHorizontal, unsigned int frame_height, uint8_t frameNum, boolean skipBlack) {

  for(unsigned int i=0; i < frame_height; i++) {
    for(unsigned int j=0; j < bitmap->width; j++) {
      
      SM_RGB pixel = { 
                      bitmap->pixel_data[((i+(frame_height * frameNum))*bitmap->width + j)*3 + 0],
                      bitmap->pixel_data[((i+(frame_height * frameNum))*bitmap->width + j)*3 + 1],
                      bitmap->pixel_data[((i+(frame_height * frameNum))*bitmap->width + j)*3 + 2] };

        int16_t newX = x + j;
        int16_t newY = y + i;
        
        if (flipHorizontal) {
          newX = x + bitmap->width - j; 
        }

        if (flipVertical) {
          newY = y + frame_height - i; 
        }

        // Don't draw black pixels
        if (skipBlack && !(pixel.red || pixel.green || pixel.blue)) {
          continue;
        } 

        // Don't draw pixels outside the matrix
        if (newX < 0 || newX > kMatrixWidth || newY < 0 || newY > kMatrixHeight) {
          continue;
        } 

        backgroundLayer.drawPixel(newX, newY, colorTransform(x,y,pixel));                  
    }
  }
}

SM_RGB doNothing(int16_t x, int16_t y, SM_RGB pixel) {
  return pixel;
}

SM_RGB twinkle(int16_t x, int16_t y, SM_RGB pixel) {
  if (pixel.blue == 0xff) {
      uint8_t starBrightness = random(0x80);
      pixel.red = starBrightness;
      pixel.green = starBrightness;
      pixel.blue = starBrightness;
  }
  return pixel;
}

SM_RGB swapColor(int16_t x, int16_t y, SM_RGB pixel) {
  if (pixel.blue == 0xfe) {
    pixel = lightColors[random(lightColorCount)];
  }
  return pixel;
}

SM_RGB randomflash(int16_t x, int16_t y, SM_RGB pixel) {
  if (random(254) > 250) {
    pixel = white;
  }
  return pixel;
}

SM_RGB snowscene(int16_t x, int16_t y, SM_RGB pixel) {

  pixel = twinkle(x, y, pixel);
  pixel = swapColor(x, y, pixel);
  
  return pixel;
}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

time_t getESP8266Time()
{
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013
  digitalWrite(ledPin, LOW);   // set the LED on

  while (pctime == 0L) {
    if (Serial1.find(TIME_HEADER)) {
      pctime = Serial1.parseInt();
      setTime(pctime); // Sync Arduino clock to the time received on the serial port
      digitalWrite(ledPin, HIGH);   // set the LED on
    }
  }
  return pctime;
}

time_t getPartyTime() {
  
  tmElements_t tm;
  tm.Hour = 16;
  tm.Minute = 30;
  tm.Second = 00;
  tm.Day = 24;
  tm.Month = 12;
  tm.Year = CalendarYrToTm(year());
  return makeTime(tm);

}

time_t getChristmasTime() {
  
  tmElements_t tm;
  tm.Hour = 06;
  tm.Minute = 00;
  tm.Second = 00;
  tm.Day = 25;
  tm.Month = 12;
  tm.Year = CalendarYrToTm(year());
  return makeTime(tm);
}

time_t getNewYearsTime() {
  
  tmElements_t tm;
  tm.Hour = 24;
  tm.Minute = 59;
  tm.Second = 59;
  tm.Day = 31;
  tm.Month = 12;
  tm.Year = CalendarYrToTm(year());
  return makeTime(tm);
}

void outputTime(time_t t){
 
  // digital clock display of the time
  Serial.print(hour(t));
  printDigits(minute(t));
  printDigits(second(t));
  Serial.print(" ");
  Serial.print(day(t));
  Serial.print(" ");
  Serial.print(month(t));
  Serial.print(" ");
  Serial.print(year(t));
  Serial.println();
}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
  Serial.print('0');
  Serial.print(digits);
}

