





// Example Arduino/ESP8266 code to upload data to Google Sheets when a button is pressed
// Follow setup instructions found here:
// https://github.com/StorageB/Google-Sheets-Logging
// reddit: u/StorageB107
// email: StorageUnitB@gmail.com
// MFRC522 code from library examples
// U8G2 library for the display. Fairly certain Arduino 1306 would have been appropriate, but this U8G2 allows us to define the I2C data pins

//Libraries for NodeMCU Chip set (ESP8266) with WiFi
#include "WiFi.h"
#include <Arduino.h>
//#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include <ArduinoJson.h>
#include <SignInData.h>


//Libraries for display
#include <Wire.h>
#include <U8g2lib.h>

//Libraries for Reader
#include <SPI.h>
#include <MFRC522.h>

//SDA and Reset pins for card reader
#define SS_PIN 8
#define RST_PIN 2

//Set display to D1 for clock and D2 for data
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ 3, /* data=*/ 4, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
SignInData SIData;
// Enter network credentials:
const char* ssid;
const char* password;


// Enter Google Script Deployment ID:
const char* GScriptId;

// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

// Google Sheets setup (do not edit)
const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = "";
String url;
HTTPSRedirect* client = nullptr;

// Declare variables that will be published to Google Sheets
int value0 ;
const char* value1 ;
const char* value2 ;

// Declare variables that will keep track of whether or not the data has been published
bool data_published = false;
int error_count = 0;

//Declare variables that will be returned from google sheet
int return_value_1;
const char* return_value_2;
const char* return_value_3;

//Variable needed for JSON parsing
StaticJsonDocument<128> doc;

//Card Reader instance
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];
int loopCount = 0;

void setup() {
  
  // Enter network credentials:
ssid     = SIData.SSID;
password = SIData.Password;

// Enter Google Script Deployment ID:
GScriptId = SIData.gScriptID;

url = String("/macros/s/") + GScriptId + "/exec";


  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //Init Display
  u8g2.begin(); 
  //init serial communication
  Serial.begin(9600);        
  delay(10);
  Serial.println('\n');
  
  
  // Connect to WiFi
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");
  printDisplay("Connecting to",ssid,"");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  
  Serial.print("Connecting to ");
  Serial.println(host);
  printDisplay("Connecting to",host,"");

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
       flag = true;
       Serial.println("Connected");
       printDisplay("Connected","(:","");
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
      printDisplay("Connection","Failed :(","Retrying...");
  }
  if (!flag){
    Serial.print("Could not connect to server: ");
    printDisplay("Cannot","Connect","X(");
    Serial.println(host);
    return;
  }
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
}


void loop() {

   loopCount++;
    if(loopCount > 250){

      //Keep last card data for 250 loops to ensure no double tap
      //After 250 loops, clear last card data so they can sign in/out again
      for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = 0xFF;
     }
     u8g2.clearDisplay();
     loopCount = 0;
    }
    
  //End the loop if a card is not present
  /*if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

   
  // attempt to publish to Google Sheets when new card is read
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

      loopCount = 0;
      printDisplay("Card Read","Please Wait","Logging...");
      
      // before attempting to publish to Google Sheets, set the data_published variable to false and error_count to 0
      data_published = false;
      error_count = 0;
    
    // Fillers for value1 and 2 so the program will run without major mods to both this and the sheet. Not used presently, maybe will in the future    
      value1 = 0;
      value2 = 0;

    // the while loop will attempt to publish data up to 3 times
    while(data_published == false && error_count < 3){

      static bool flag = false;
      if (!flag){
        client = new HTTPSRedirect(httpsPort);
        client->setInsecure();
        flag = true;
        client->setPrintResponseBody(true);
        client->setContentTypeHeader("application/json");
      }
      if (client != nullptr){
        if (!client->connected()){
          client->connect(host, httpsPort);
        }
      }
      else{
        Serial.println("Error creating client object!");
      }

      // Create json object string to send to Google Sheets
      payload = payload_base + "\"" + String(value0,HEX) + "," + value1 + "," + value2 + "\"}";
  
      // Publish data to Google Sheets
      Serial.println("Publishing data...");
      Serial.println(payload);
    
      if(client->POST(url, host, payload)){ 
        // do stuff here if publish was successful
        data_published = true;
        String input = client->getResponseBody();


        DeserializationError error = deserializeJson(doc, input);

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
      else{
       // do stuff here if publish was not successful
       Serial.println("Error while connecting");

       printDisplay("Error", "while","connecting");
       error_count++;
       delay(2000);
      }
      yield();
    } 
  }//end if

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
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