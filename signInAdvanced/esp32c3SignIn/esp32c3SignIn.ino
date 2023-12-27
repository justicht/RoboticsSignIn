//Include required libraries
#include "WiFi.h"
#include <HTTPClient.h>
#include "time.h"
#include <SignInData.h>
#include <ArduinoJson.h>
//Libraries for display
#include <Wire.h>
#include <U8g2lib.h>

//Libraries for Reader
#include <SPI.h>
#include <MFRC522.h>

//SDA and Reset pins for card reader
#define SS_PIN D7
#define RST_PIN D0

// Google Sheets setup (do not edit)
const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = "";
//String url = String("/macros/s/") + GScriptId + "/exec";;
//HTTPSRedirect* client = nullptr;
bool data_published = false;


int value0 ;
int return_value_1;
const char* return_value_2;
const char* return_value_3;
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";
SignInData SIData;

// WiFi credentials
const char* ssid     = SIData.SSID;
const char* password = SIData.Password;
// Google script ID and required credentials
String GOOGLE_SCRIPT_ID =SIData.gScriptID;//   ;//  // change Gscript ID
String url = String("/macros/s/") + GOOGLE_SCRIPT_ID + "/exec";
int count = 0;
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ D1, /* data=*/ D2, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

//Card Reader instance
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];
int loopCount = 0;

unsigned long compTime;

void setup() {
  compTime = millis();
  
  u8g2.begin(); 
  //printDisplay("Yo","it","works!");
  //delay(1000);
  Serial.begin(115200);
  //delay(1000);


  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  // connect to WiFi
  // Serial.println();
  // Serial.print("Connecting to wifi: ");
  // Serial.println(ssid);
  // Serial.flush();
  WiFi.begin(ssid, password);
  String connectingDots = "";
  while (WiFi.status() != WL_CONNECTED) {
    printDisplay("Connecting to",ssid, connectingDots);
    delay(500);
    connectingDots = connectingDots + ".";
    Serial.print(".");
  }
  printDisplay("Connected :)", "", "");
  // Init and get the time
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}
void loop() {

  loopCount = millis() - compTime;
  Serial.println(loopCount);
  
    if(loopCount > 1500){

      //Keep last card data for 1.5 seconds  to ensure no double tap
      //After 1.5 seconds , clear last card data so they can sign in/out again
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = 0xFF;
     }
     u8g2.clearDisplay();
     
    }

      //End the loop if a card is not present
  if ( ! rfid.PICC_IsNewCardPresent()){
    //Serial.println("No Card");
      return;
  }
    

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial()){
    
    return;
  }

  //Serial.println("Card Read");
    if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3])  {
      
      //Store card data to a variable for processing
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = rfid.uid.uidByte[i];
      }

      //Convert data bytes to a integer for us to use. They're probably stored backwards, but that's ok, all we care about is the unique numbers.
      value0 = 0;
      for (byte i = 0; i <4 ; i++) {
        value0 = value0<<8;
        value0 += nuidPICC[i];
        //Below commented code used for debugging. Shows each Byte of read card on display for 2 seconds
        //printDisplay(String(nuidPICC[i]),"","");
        //delay(2000);
        
      }
      
      if (WiFi.status() == WL_CONNECTED) {
    Serial.print("RSSI = ");
    Serial.println(WiFi.RSSI());
    printDisplay(String(value0,HEX),"RSSI =" ,String(WiFi.RSSI()));
    //printDisplay("RSSI = ",String(WiFi.RSSI()),"dB");
    static bool flag = false;
    Serial.print("POST data to spreadsheet:");
    accessToGoogleSheets(String(value0,HEX));

  }
    }
  
   
  count++;
  
} 



void accessToGoogleSheets(String id) {
    HTTPClient http;
    String URL = "https://script.google.com/macros/s/";
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    URL += GOOGLE_SCRIPT_ID;
    URL += "/exec?";
    URL += "command=insert_row";
    URL += "&sheet_name=Sheet1";
    URL += "&value0=";
    URL += id;

    Serial.println("[HTTP] begin...");
    Serial.println(URL);
    // access to your Google Sheets
    Serial.println();
    // configure target server and url
    http.begin(URL);
    printDisplay("please","wait","logging...");
    Serial.println("[HTTP] GET...");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.print("[HTTP] GET... code: ");
        Serial.println(httpCode);
        
        // file found at server
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
         
            Serial.println(payload);
            printResponse(payload);
            compTime = millis();
        }
    } else {
        Serial.print("[HTTP] GET... failed, error: ");
        Serial.println(http.errorToString(httpCode).c_str());
        printDisplay(String(httpCode),"Fail","");
        compTime = millis();
    }
}

void printDisplay(String line1, String line2, String line3){
  
  u8g2.clearDisplay();
  u8g2.firstPage();
  do {

    int charSize = 20;
    char b[charSize];

    u8g2.setFont(u8g2_font_ncenB10_tr); //Font could change, but this is fine
    line1.toCharArray(b,charSize); //Convert string to chars. String input for ease of use.
    u8g2.drawStr(/*Start x:*/ 0,/*Start Y:*/ 12, /* chars*/b);

    line2.toCharArray(b,charSize);
    u8g2.drawStr(0,30,b);

    line3.toCharArray(b,charSize);
    u8g2.drawStr(0,48,b);
  
  } while ( u8g2.nextPage() );

}

void printResponse(String response){
//Variable needed for JSON parsing
StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, response);

        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }

        return_value_1 = doc["return_value_1"]; // 1
        return_value_2 = doc["return_value_2"]; // 2
        return_value_3 = doc["return_value_3"]; // 3



        printDisplay(String(return_value_2),"Signed " + String(return_value_3),"Hours: " + String(return_value_1));
}
