/*
 *  Application note: Social counter demo for ArduiTouch ESP  
 *                                        (for ESP32) 
 *  Version 1.0
 *  Copyright (C) 2019  Hartmut Wendt  www.zihatec.de
 *   
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/   


/*______Import Libraries_______*/
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <XPT2046_Touchscreen.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <ArduinoJson.h>           // https://github.com/bblanchon/ArduinoJson
#include "InstagramStats.h"       // https://github.com/witnessmenow/arduino-instagram-stats
#include "JsonStreamingParser.h"  // https://github.com/squix78/json-streaming-parser
#include <TwitterApi.h>            // https://github.com/witnessmenow/arduino-twitter-api  
#include <YoutubeApi.h> 
#include <WiFi.h>

#include <WiFiClientSecure.h> 
#include "usergraphics.h"

/*______End of Libraries_______*/


/*______Define LCD pins for ArduiTouch _______*/
#define TFT_CS   5
#define TFT_DC   4
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  22
#define TFT_MISO 19
#define TFT_LED  15  


#define HAVE_TOUCHPAD
#define TOUCH_CS 14
#define TOUCH_IRQ 2
/*_______End of definitions______*/


/*____Calibrate TFT LCD_____*/
#define TS_MINX 370
#define TS_MINY 470
#define TS_MAXX 3700
#define TS_MAXY 3600
#define MINPRESSURE 10
#define MAXPRESSURE 2000 
/*______End of Calibration______*/


/*______Define WiFi_______*/
#define WIFI_SSID "xxxxxx"       // Enter your SSID here
#define WIFI_PASS "xxxxx"    // Enter your WiFi password here
#define WIFI_HOSTNAME "Social_Counter" 
#define PORT 5444
#define WIFICLIENT_MAX_PACKET_SIZE 100
/*_______End of WiFi definitions______*/
 

/*______Define Facebook config_______*/ 
#define FACEBOOK_HOST "graph.facebook.com"
#define FACEBOOK_PORT 443
#define PAGE_ID ""YOUR_PAGE_ID"
#define ACCESS_TOKEN "YOUR_ACCESS_TOKEN"
// graph.facebook.com SHA1 fingerprint
const char* facebookGraphFingerPrint = "YOUR_FINGER_PRINT";
/*_______End of Facebook config______*/

/*______Define Youtube config_______*/ 
#define API_KEY "YOUR_API_KEY"  // your google apps API Token
#define CHANNEL_ID "YOUR_CHANNEL_ID" // makes up the url of channel
/*_______End of Youtube config______*/

/*______Define Instagram config_______*/ 
String Instagram_userName = "YOUR_USERNAME";    // from their instagram url https://www.instagram.com/userName/ 
/*_______End of Youtube config______*/


/*____Program specific constants_____*/
enum { PM_MAIN, PM_OPTION, PM_CLEANING};  // Program modes

/*______End of specific constants______*/
int FB_Follower=0;
int IG_Follower=0;
long YT_Subscriber=0;
long YT_Videos=0;
long YT_Views=0;


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);


WiFiClientSecure secureClient;
YoutubeApi api(API_KEY, secureClient); 
InstagramStats instaStats(secureClient); 


// #define _debug
 int X,Y;
 int Timer_Backlight = 255; 
 uint8_t PMode = PM_MAIN;         // program mode
 bool Touch_pressed = false;
 uint8_t Timer_Cleaning=0;
 


void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  //Manual Wifi
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  Serial.print("/");
  Serial.println(WIFI_PASS);
  WiFi.mode(WIFI_STA);
  //WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID,WIFI_PASS);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (i>80) i=0;
    i+=10;
    Serial.print(".");
  }
  Serial.print("Connected...");
} 
 

void setup() {
  
  Serial.begin(9600); 
  



  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);    // HIGH to Turn on;

  tft.begin();
  touch.begin();
  #ifdef _debug
  Serial.print("tftx ="); Serial.print(tft.width()); Serial.print(" tfty ="); Serial.println(tft.height());
  #endif

  
  
  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip); 

  tft.setRotation(1); 

  //Sound
  ledcSetup(0,1E5,12);
  ledcAttachPin(21,0);
}


TS_Point p;

void loop() {
  if (Touch_Event()== true) { 
    X = p.y; Y = p.x;
    if (Touch_pressed == false) {
      if (Timer_Backlight) DetectButtons();
      Timer_Backlight = 255; // reset BL timer
      Serial.print(X); Serial.print(','); Serial.println(Y);// + " " + Y);        
    }
    Touch_pressed = true;
    
  } else {
    Touch_pressed = false;
  } 

  // instagram
  getInstagramStatsForUser();
  draw_instagram_screen();
  delay(2000);  
  
  // facebook
  FB_Follower=getPageFansCount();
  draw_facebook_screen();
  delay(2000);

  //Youtube
  getYoutubeStats();
  draw_youtube_screen();
  delay(2000);
  
}


/********************************************************************//**
 * @brief     detects a touch event and converts touch data 
 * @param[in] None
 * @return    boolean (true = touch pressed, false = touch unpressed) 
 *********************************************************************/
bool Touch_Event() {
  p = touch.getPoint(); 
  delay(1);
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, 320);
  p.y = map(p.y, TS_MINY, TS_MAXY, 240, 0);
  if (p.z > MINPRESSURE) return true;  
  return false;  
}





/********************************************************************//**
 * @brief     detecting pressed buttons with the given touchscreen values
 * @param[in] None
 * @return    None
 *********************************************************************/

void DetectButtons()
{

}



/********************************************************************//**
 * @brief     Drawing of the main program screen
 * @param[in] None
 * @return    None
 *********************************************************************/
void draw_main_screen()
{
  tft.fillScreen(ILI9341_BLACK);
  //tft.drawRGBBitmap(50,20, instagram_small,50,50);
  tft.drawRGBBitmap(10,10, youtube_big,100,100);
  tft.drawRGBBitmap(50,100, facebook_small,50,50);
  tft.drawRGBBitmap(50,180, youtube_small,50,50);

}

/********************************************************************//**
 * @brief     Drawing screen for Youtube stats
 * @param[in] None
 * @return    None
 *********************************************************************/
void draw_youtube_screen()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRGBBitmap(10,70, youtube_big,100,100);
  tft.setTextSize(2);
  tft.setCursor(130, 7);
  tft.print("Video Count:"); 
  tft.setCursor(130, 87);
  tft.print("Subscriber:"); 
  tft.setCursor(130, 167);
  tft.print("View Count:"); 
  tft.setTextSize(3);
  tft.setCursor(130, 40);
  tft.print(YT_Videos); 
  tft.setCursor(130, 120);
  tft.print(YT_Subscriber); 
  tft.setCursor(130, 200);
  tft.print(YT_Views); 
}

/********************************************************************//**
 * @brief     Drawing screen for Facebook follower
 * @param[in] None
 * @return    None
 *********************************************************************/
void draw_facebook_screen()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRGBBitmap(10,70, facebook_big,100,100);
  tft.setTextSize(2);
  tft.setCursor(130, 87);
  tft.print("Fan Count:"); 
  tft.setCursor(130, 120);
  tft.setTextSize(4);
  tft.print(FB_Follower); 
}

/********************************************************************//**
 * @brief     Drawing screen for Instagram follower
 * @param[in] None
 * @return    None
 *********************************************************************/
void draw_instagram_screen()
{
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRGBBitmap(10,70, instagram_big,100,100);
  tft.setTextSize(2);
  tft.setCursor(130, 87);
  tft.print("Follower:"); 
  tft.setCursor(130, 120);
  tft.setTextSize(4);
  tft.print(IG_Follower); 
}

/********************************************************************//**
 * @brief     getting Instagram stats
 * @param[in] None
 * @return    None
 *********************************************************************/
void getInstagramStatsForUser() {
  Serial.println("Getting instagram user stats for channel" + Instagram_userName );
  InstagramUserStats response = instaStats.getUserStats(Instagram_userName);
  Serial.println("Response:");
  Serial.print("Number of followers: ");
  IG_Follower = response.followedByCount;
  Serial.println(response.followedByCount);    
} 

/********************************************************************//**
 * @brief     Getting Youtube stats
 * @param[in] None
 * @return    None
 *********************************************************************/
void getYoutubeStats() {
  Serial.println("Getting youtube for " );
  if(api.getChannelStatistics(CHANNEL_ID))
  {
      Serial.println("---------Stats---------");
      Serial.print("Subscriber Count: ");
      Serial.println(api.channelStats.subscriberCount);
      YT_Subscriber=api.channelStats.subscriberCount;
      Serial.print("View Count: ");
      Serial.println(api.channelStats.viewCount);
      YT_Views=api.channelStats.viewCount;
      Serial.print("Comment Count: ");
      Serial.println(api.channelStats.commentCount);
      Serial.print("Video Count: ");
      Serial.println(api.channelStats.videoCount);
      YT_Videos=api.channelStats.videoCount;
      Serial.println("------------------------");
   } 
}

/********************************************************************//**
 * @brief     Making request graph for Facebook
 * @param[in] None
 * @return    None
 *********************************************************************/
String makeRequestGraph(){
  if (!secureClient.connect(FACEBOOK_HOST, FACEBOOK_PORT)) {
    Serial.println("Host connection failed");
    return "";    
  }
  
  String params = "?pretty=0&fields=fan_count&access_token="+String(ACCESS_TOKEN);
  String path = "/v2.8/" + String(PAGE_ID);
  String url = path + params;
  Serial.print("requesting URL: ");
  Serial.println(url);

  String request = "GET " + url + " HTTP/1.1\r\n" +
    "Host: " + String(FACEBOOK_HOST) + "\r\n\r\n";
  
  secureClient.print(request);

  String response = "";
  String chunk = "";  
  
  do {
    if (secureClient.connected()) {
      delay(5);
      chunk = secureClient.readStringUntil('\n');
      if(chunk.startsWith("{")){
        response += chunk;
      }
    }
  } while (chunk.length() > 0);
    
  Serial.print(" Message ");
  Serial.println(response);  

  return response;
}

/********************************************************************//**
 * @brief     Get FB fan count
 * @param[in] None
 * @return    integer fancount
 *********************************************************************/
int getPageFansCount(){
  String response = makeRequestGraph();  
  
  const size_t bufferSize = JSON_OBJECT_SIZE(2) + 40;
  DynamicJsonBuffer jsonBuffer(bufferSize);  
  
  JsonObject& root = jsonBuffer.parseObject(response);
  
  int fanCount = root["fan_count"];  
  return fanCount;
}
