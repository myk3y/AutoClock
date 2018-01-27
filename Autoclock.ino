/* 
 *******************************************************************************************************************
 * This sketch gratefully reuses and reworks code and libraries from Sparkfun:
 * https://github.com/sparkfun/SparkFun_Micro_OLED_Arduino_Library/blob/master/examples/MicroOLED_Clock/MicroOLED_Clock.ino)
 * by Jim Lindblom @ SparkFun Electronics
 * Original Creation Date: October 27, 2014
 ********************************************************************************************************************
 * Modified for WeMos D1 Mini (ESP-12) and WeMos OLED shield/Sparkfun Micro OLED breakout to use NTP, 
 * ipapi.com, timezonedb.api for automatic time setting and update 
 * APIs parsed with json and wireless connected using tzapu's brilliant WiFiManager library - just don't bother using
 * anything else for ESPs and WiFi: https://github.com/tzapu/WiFiManager
 *
 * NOTE!!!!! YOU WILL NEED TO OBTAIN AN API KEY FROM TIMEZONEDB.COM TO resolve your local timezone offset. There are many timezone 
 * NOTE!!!!! services in the Naked City, this is just one of them (and it works).
 *
 * Additional code by Myk3y January 2018
 *
 *  Development environment specifics:
 *  Arduino 1.8.5
 *  WeMos D1 Mini R2.0 (http://wemos.cc)
 *  WeMos Micro OLED Shield
 * 
 * This code is beerware; if you are ever in Borneo, look me up!
 * And you've found the code helpful, feel free to buy me a pint (or two...) Same goes for the blokes at Sparkfun.
 * 
 * Distributed as-is; no warranty implied or given.
 */

#include <NTPClient.h>                  // NTP time client
#include <ESP8266WiFi.h>                // ESP8266 WiFi library
#include <WiFiManager.h>                // Smart WiFi Manager to handle networking - genius! https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>                    // UDP library to transport NTP data
#include <ArduinoJson.h>                // json library for parsing http results
#include <ESP8266HTTPClient.h>          // ESP8266 http library
#include<TimeLib.h>                     // Advanced time functions.
#include <Wire.h>                       // I2C functions
#include <SFE_MicroOLED.h>              // Include the SFE_MicroOLED  library
#define KEY "XXXXXXXXXXX"               // timezonedb.com API Key

//////////////////////////
// WeMos OLED Definition //
//////////////////////////
#define PIN_RESET D3                    // Connect RST on shield to pin D3 (I2C) on mini
#define DC_JUMPER 0                     // DC jumper setting(I2C only) -  set to 0 on WeMos shield.

MicroOLED oled(PIN_RESET, DC_JUMPER);   // I2C 

char urlData[180];                      //array for holding complied query URL
int  offset;                            // local UTC offset in seconds

// Global variables for the clock face

const int MIDDLE_Y = oled.getLCDHeight() / 2;
const int MIDDLE_X = oled.getLCDWidth() / 2;

int CLOCK_RADIUS;
int POS_12_X, POS_12_Y;
int POS_3_X, POS_3_Y;
int POS_6_X, POS_6_Y;
int POS_9_X, POS_9_Y;
int S_LENGTH;
int M_LENGTH;
int H_LENGTH;

unsigned long lastDraw = 0;

WiFiUDP ntpUDP;                                 //initialise UDP NTP
NTPClient ntpClient(ntpUDP, "time.google.com"); // initialist NTP client with server name 

HTTPClient http;                                // Initialise HTTP Client

void setup()
{
  Serial.begin(115200);
  WiFiManager wifiManager;            // tzapu's brilliant wifi configuration wizard: https://github.com/tzapu/WiFiManager
  wifiManager.autoConnect("Clock");   // configuration for the access point
  
  Serial.println("WiFi Client connected!");
  
  ntpClient.begin();            // Start NTP client
  hourFormat12();               // Set to 12 hour AM.PM time 

  getIPtz();                    // get ip-api.com Timezone data for your location
  getOffset();                  // get timezonedb.com UTC offset
  
  setSyncProvider(getNTPTime);  // Specify function to sync time_t with NTP
  setSyncInterval(600);         // Set time_t sync interval - 10 minutes
  
  time_t getNTPTime();        
     if(timeStatus()==timeNotSet) // test for time_t set
     Serial.println("Unable to sync with NTP");
  else
     Serial.print("NTP has set the system time to: "); Serial.print(hour()); Serial.print(":"); Serial.print(minute()); Serial.print(":"); Serial.println(second());;
 
  oled.begin();     // Initialize the OLED
  oled.clear(PAGE); // Clear the display's internal memory
  oled.clear(ALL);  // Clear the library's display buffer
  oled.display();   // Display what's in the buffer (splashscreen)
  
  initClockVariables();
  
  oled.clear(ALL);
  drawFace();
  drawArms(hour(), minute(), second());
  oled.display(); // display the memory buffer drawn
}

void loop()
{   delay(100);         // Allow ESP to do housekeeping
                        // Draw the clock:
    oled.clear(PAGE);   // Clear the buffer
    drawFace();         // Draw the face to the buffer
    drawArms(hour(), minute(), second());  // Draw arms to the buffer
    oled.display();     // Draw the memory buffer
  
}

void getIPtz() { // pull timezone data from ip-api.com and create URL string to query http://api.timezonedb.com

  http.begin("http://ip-api.com/json/?fields=timezone");      //get gelocation timezone data from ip-api
  int httpCode = http.GET();                                  //Send the request
  String ipapi = http.getString();                            // Return to the string
  http.end();   //Close connection                            // Close http client
  DynamicJsonBuffer jsonBuffer;                               //Set up dynamic json buffer
  JsonObject& root = jsonBuffer.parseObject(ipapi);           // Parse ip-api data
  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  char* tz = strdup(root["timezone"]); // copy timezone data from json parse

  urlData[0] = (char)0;// clear array
  // build query url for timezonedb.com
  strcpy (urlData, "http://api.timezonedb.com/v2/get-time-zone?key=");
  strcat (urlData, KEY); // api key
  strcat (urlData, "&format=json&by=zone&zone=");
  strcat (urlData, tz); // timezone data from ip-api.com
}

void getOffset() {
 
  http.begin(urlData);
  int httpCode = http.GET();                              //Send the request
  String timezonedb = http.getString();
  http.end();   //Close connection
  DynamicJsonBuffer jsonBuffer;   // allocate json buffer

  JsonObject& root = jsonBuffer.parseObject(timezonedb); // parse return from timzonedb.com

  offset = (root["gmtOffset"]); // get offset from UTC/GMT 
}

time_t getNTPTime(){  // Return current UTC time to time_t
  ntpClient.update();
  return ntpClient.getEpochTime() + offset;
  Serial.println( ntpClient.getEpochTime() + offset);
}

void initClockVariables()
{
  // Calculate constants for clock face component positions:
  oled.setFontType(0);
  CLOCK_RADIUS = min(MIDDLE_X, MIDDLE_Y) - 1;
  POS_12_X = MIDDLE_X - oled.getFontWidth();
  POS_12_Y = MIDDLE_Y - CLOCK_RADIUS + 2;
  POS_3_X  = MIDDLE_X + CLOCK_RADIUS - oled.getFontWidth() - 1;
  POS_3_Y  = MIDDLE_Y - oled.getFontHeight()/2;
  POS_6_X  = MIDDLE_X - oled.getFontWidth()/2;
  POS_6_Y  = MIDDLE_Y + CLOCK_RADIUS - oled.getFontHeight() - 1;
  POS_9_X  = MIDDLE_X - CLOCK_RADIUS + oled.getFontWidth() - 2;
  POS_9_Y  = MIDDLE_Y - oled.getFontHeight()/2;
  
  // Calculate clock arm lengths
  S_LENGTH = CLOCK_RADIUS - 2;
  M_LENGTH = S_LENGTH * 0.7;
  H_LENGTH = S_LENGTH * 0.5;
}

// Draw the clock's three arms: seconds, minutes, hours.
void drawArms(int h, int m, int s)
{
  double midHours;  // this will be used to slightly adjust the hour hand
  static int hx, hy, mx, my, sx, sy;
  
  // Adjust time to shift display 90 degrees ccw
  // this will turn the clock the same direction as text:
  h -= 3;
  m -= 15;
  s -= 15;
  if (h <= 0)
    h += 12;
  if (m < 0)
    m += 60;
  if (s < 0)
    s += 60;
  
  // Calculate and draw new lines:
  s = map(s, 0, 60, 0, 360);  // map the 0-60, to "360 degrees"
  sx = S_LENGTH * cos(PI * ((float)s) / 180);  // woo trig!
  sy = S_LENGTH * sin(PI * ((float)s) / 180);  // woo trig!
  // draw the second hand:
  oled.line(MIDDLE_X, MIDDLE_Y, MIDDLE_X + sx, MIDDLE_Y + sy);
  
  m = map(m, 0, 60, 0, 360);  // map the 0-60, to "360 degrees"
  mx = M_LENGTH * cos(PI * ((float)m) / 180);  // woo trig!
  my = M_LENGTH * sin(PI * ((float)m) / 180);  // woo trig!
  // draw the minute hand
  oled.line(MIDDLE_X, MIDDLE_Y, MIDDLE_X + mx, MIDDLE_Y + my);
  
  midHours = minute()/12;  // midHours is used to set the hours hand to middling levels between whole hours
  h *= 5;  // Get hours and midhours to the same scale
  h += midHours;  // add hours and midhours
  h = map(h, 0, 60, 0, 360);  // map the 0-60, to "360 degrees"
  hx = H_LENGTH * cos(PI * ((float)h) / 180);  // woo trig!
  hy = H_LENGTH * sin(PI * ((float)h) / 180);  // woo trig!
  // draw the hour hand:
  oled.line(MIDDLE_X, MIDDLE_Y, MIDDLE_X + hx, MIDDLE_Y + hy);
}

// Draw an analog clock face
void drawFace()
{
  // Draw the clock border
  oled.circle(MIDDLE_X, MIDDLE_Y, CLOCK_RADIUS);
  
  // Draw the clock numbers
  oled.setFontType(0); // set font type 0, please see declaration in SFE_MicroOLED.cpp
  oled.setCursor(POS_12_X, POS_12_Y); // points cursor to x=27 y=0
  oled.print(12);
  oled.setCursor(POS_6_X, POS_6_Y);
  oled.print(6);
  oled.setCursor(POS_9_X, POS_9_Y);
  oled.print(9);
  oled.setCursor(POS_3_X, POS_3_Y);
  oled.print(3);
}
