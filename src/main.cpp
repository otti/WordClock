#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ElegantOTA.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <time.h> 

#define NO_OF_LEDS    (10*11+4) // 10 Rows * 11 Clolumns + 4 Dots
#define NEO_PIXEL_PIN 4

//                  0x00GGRRBB
#define PIXEL_COLOR 0x005D76CF  
#define MAX_BRIGHTNESS 128

// My Neo Pixel do use GRB instead of RGB
#define SWAP_RGB(x) (((x&0x00FF0000)>>8) | ((x&0x0000FF00)<<8) | (x&0x000000FF))

#define NTP_SERVER "at.pool.ntp.org"           
#define TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NO_OF_LEDS, NEO_PIXEL_PIN, NEO_RGB + NEO_KHZ800);

ESP8266WebServer server(80);

unsigned long ota_progress_millis = 0;

// Pixel Layout 
//
// 112                                            113
//    0     1   2   3   4   5   6   7   8   9  10
//   
//    11                                       21
//   
//    22                                       32
//   
//    33                                       43
//   
//    44                                       54
//   
//    55                                       65
//   
//    66                                       76
//   
//    77                                       87
//   
//    88                                       89
//   
//    99  100 101 102 103 104 105 106 107 108 109
// 111                                            110 




typedef struct
{
  uint8_t Row;
  uint8_t Col;
  uint8_t NumLetters;
}sWord_t;


sWord_t ES        = {0,  0, 2};
sWord_t IST       = {0,  3, 3};
#define DOT_1     {10, 2, 1}
#define DOT_2     {10, 3, 1}
#define DOT_3     {10, 0, 1}
#define DOT_4     {10, 1, 1}

#define UHR        {9, 8, 3}
#define FUENF_M    {0, 7, 4}
#define ZEHN_M     {1, 0, 4}
#define ZWANZIG_M  {1, 4, 7}
#define DREI       {2, 0, 4}
#define VIERTEL    {2, 4, 7}
#define VOR        {3, 0, 3}
#define FUNK       {3, 3, 4}
#define NACH       {3, 7, 4}
#define HALB       {4, 0, 4}
#define TERMINATOR {0, 0, 0}

#define ELF_H      {4, 5, 3}
#define FUENF_H    {4, 7, 4}
#define EINS_H     {5, 0, 4}
#define ZWEI_H     {5, 7, 4}
#define DREI_H     {6, 0, 4}
#define VIER_H     {6, 7, 4}
#define SECHS_H    {7, 0, 5}
#define ACHT_H     {7, 7, 4}
#define SIEBEN_H   {8, 0, 6}
#define ZWOELF_H   {8, 6, 5}
#define ZEHN_H     {9, 0, 4}
#define NEUN_H     {9, 3, 4}



sWord_t HOURS[]        = {ZWOELF_H, EINS_H, ZWEI_H, DREI_H, VIER_H, FUENF_H, SECHS_H, SIEBEN_H, ACHT_H, NEUN_H, ZEHN_H, ELF_H};
sWord_t MINUTES[12][4] = {
                          {UHR,       TERMINATOR, TERMINATOR, TERMINATOR},
                          {FUENF_M,   NACH,       TERMINATOR, TERMINATOR},
                          {ZEHN_M,    NACH,       TERMINATOR, TERMINATOR},
                          {VIERTEL,   NACH,       TERMINATOR, TERMINATOR},
                          {ZWANZIG_M, NACH,       TERMINATOR, TERMINATOR},
                          {FUENF_M,   VOR,        HALB,       TERMINATOR},
                          {HALB,      TERMINATOR, TERMINATOR, TERMINATOR},
                          {FUENF_M,   NACH,       HALB,       TERMINATOR},
                          {ZWANZIG_M, VOR,        TERMINATOR, TERMINATOR},
                          {VIERTEL,   VOR,        TERMINATOR, TERMINATOR},
                          {ZEHN_M,    VOR,        TERMINATOR, TERMINATOR},
                          {FUENF_M,   VOR,        TERMINATOR, TERMINATOR}
                         };
sWord_t DOTS[5][4] =     {
                          {TERMINATOR, TERMINATOR, TERMINATOR, TERMINATOR},
                          {DOT_1,      TERMINATOR, TERMINATOR, TERMINATOR},
                          {DOT_1,      DOT_2,      TERMINATOR, TERMINATOR},
                          {DOT_1,      DOT_2,      DOT_3,      TERMINATOR},
                          {DOT_1,      DOT_2,      DOT_3,      DOT_4     }
                        };

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}


void setup()
{
  bool res;
  WiFiManager wm;

  wm.setHostname("WordClock");
  res = wm.autoConnect("WordClock ConfigAP"); 

  if(!res)
    Serial.println("Failed to connect");
  else 
    Serial.println("connected...yeey :)");

  // Init Neo Pixel
  strip.begin();
  strip.setBrightness(MAX_BRIGHTNESS);             // set the maximum LED intensity down to 50

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
  

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

  server.on("/", []() {
    server.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
  });

  configTime(TZ, NTP_SERVER);

  ElegantOTA.begin(&server);    // Start ElegantOTA

  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
}



time_t now;                         // seconds since Epoch (1970) - UTC
tm CurrentTime;                     // the structure tm holds time information in a more convenient way

#define PIXELS_PER_ROW 11

uint8_t Pixels[NO_OF_LEDS];

void ClearPixels(void)
{
  memset(Pixels, 0, sizeof(Pixels));
}

#define NUMBER_OF_DOTS 4

void SetPixels(sWord_t* pWord)
{
  for(int i=0; i<pWord->NumLetters; i++)
  {
    Pixels[pWord->Row*PIXELS_PER_ROW + pWord->Col + i] = 1;
  }
}

void UpdateDisplay(uint8_t Hour, uint8_t Min)
{
  uint8_t u8Dots = Min%5;
  sWord_t* pWord;

  ClearPixels();
  SetPixels(&ES);
  SetPixels(&IST);

  Serial.print("Hour: ");
  Serial.print(Hour);
  Serial.print(" Min: ");
  Serial.print(Min);
  Serial.print(" Dots: ");
  Serial.println(u8Dots);


  pWord = &DOTS[u8Dots][0];
  SetPixels(pWord++);
  SetPixels(pWord++);
  SetPixels(pWord++);
  SetPixels(pWord++);


  pWord = &MINUTES[Min/5][0];
  SetPixels(pWord++);
  SetPixels(pWord++);
  SetPixels(pWord++);
  SetPixels(pWord++);
  

  if( Min >= 25 ) //ab 5 vor halb wird die naechsete Stunde angezeigt  --> 9:25 --> (Es ist fuen vor halb 10 )
    Hour++;

  if( Hour >= 24 )
    Hour = 0;

  if( Hour >= 12 ) // 13 Uhr --> 1 Uhr
    Hour -= 12;

  SetPixels(&HOURS[Hour]);

  for(int i=0; i<NO_OF_LEDS; i++)
  {
    if( Pixels[i] )
      strip.setPixelColor(i, (uint32_t)SWAP_RGB(PIXEL_COLOR));
    else
      strip.setPixelColor(i, 0);
  }

  strip.show();
}

void loop()
{
  server.handleClient();
  ElegantOTA.loop();

  time(&now);                       // read the current time
  localtime_r(&now, &CurrentTime);  // update the structure tm with the current time

  UpdateDisplay(CurrentTime.tm_hour, CurrentTime.tm_min);
  delay(1000); // dirty delay
                    
}
