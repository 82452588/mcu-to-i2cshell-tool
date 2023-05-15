#include <FS.h>  
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>

#define PROTOCOL_TCP
#define UART_BAUD 115200

#include <WiFiClient.h>
#define RXPIN 18
#define TXPIN 19

const int port = 9876;
WiFiClient client;
WiFiServer server(port);

#include "time.h"
#include "sntp.h"
const char* ntpServer1 = "ntp.aliyun.com";
const char* ntpServer2 = "ntp.tuna.tsinghua.edu.cn";
const char* time_zone = "CST-8";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

#include <Wire.h>
#include <stdio.h>
#include <string.h>
#define SDA 0 //0
#define SCL 1 //1

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Button2.h>

// Define the pins for the buttons
#define btn_M 4 //4
#define btn_Down 9  //8
#define btn_R 8 //5
#define btn_L 13 //9
#define btn_Up 5 //13

const int TFT_CS = 7;
const int TFT_DC = 6;
const int TFT_MOSI = 3;
const int TFT_SCLK = 2;
const int TFT_RST = 10;
const int TFT_BACKLIGHT = 11;

// Initialize the TFT display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Initialize the Button2 library buttons
Button2 btnM, btnUp, btnDown, btnL, btnR;

// Define the menu items
String menuItems[] = {"Scan I2C", "I2c Clock", "TTL mode", "Time", "Reboot"};
int selectedMenuItem = 0;

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];
char opCode[2],tmpdevice[10],tmpregister[10], tmpdata[10];
byte deviceAddress[10],registerAddress[10], data[10];
char value[8];
const char startMarker = '<';
const char endMarker = '>';
bool newData = false;

//const int ledPinL = 13;
const int ledPinR = 12;
unsigned long LEDinterval = 30;
unsigned long prevLEDmillis = 0;
unsigned long curMillis;

void setup() {
  //pinMode(ledPinL, OUTPUT);
  pinMode(ledPinR, OUTPUT);
  btnsetup();
  Wire.begin(SDA,SCL); //SDA,SCL
  Serial.begin(UART_BAUD);
  // Initialize the TFT display and clear the screen
  tft.initR(INITR_MINI160x80);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(2);
  tft.setFont(&FreeMonoBold18pt7b);
  tft.setCursor(7, 80);
  tft.setTextColor(ST7735_BLUE);
  tft.print("I2C"); //logo display on the screen
  delay(200);

  tft.setFont(NULL);
  displayMenu();
  clearscreen();
  tft.println(F("Type <h> help"));
  Serial.println(F("Type <h> help"));
  scani2c();
  btnsetup();
  timesetup();
  wifisetup();

  //button.setClickHandler(handler);

}

void btnsetup() {
  btnM.begin(btn_M);
  btnM.setDebounceTime(200);
  btnUp.begin(btn_Up);
  btnUp.setDebounceTime(200);
  btnDown.begin(btn_Down);
  btnDown.setDebounceTime(200);
  btnL.begin(btn_L);
  btnR.begin(btn_R);
}

void wifisetup() {

  WiFiManager wifiManager;
  wifiManager.setBreakAfterConfig(false);
  wifiManager.setDebugOutput(false);
  wifiManager.setTimeout(15); //waiting 15s to setup the wifi then exit configure in ap mode
  //tries to connect to last known settings
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP" with password "password"
  //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("Tools", "82452588")) {
    clearscreen();
    Serial.println(F("AP:192.168.4.1")); tft.println(F("192.168.4.1"));
  }
  else {
  Serial.print(F("STA: "));
  clearscreen();
  Serial.println(WiFi.localIP());
  tft.println(WiFi.localIP());
   }


  #ifdef PROTOCOL_TCP
  server.begin(); // start TCP server 
  #endif
  }

void timesetup() {
     sntp_set_time_sync_notification_cb( timeavailable );
     sntp_servermode_dhcp(1);    // (optional)
     //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
     configTzTime(time_zone, ntpServer1, ntpServer2);
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    tft.setTextColor(ST77XX_BLUE);
    tft.println(F("Time not available"));
    tft.setTextColor(ST77XX_GREEN);
    clearscreen();
    return;
  }
  //tft.setTextColor(ST77XX_WHITE);
  clearscreen();
  clearscreen();
  //tft.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  tft.println(&timeinfo, "%A, %B %d %H:%M:%S");
  //tft.setTextColor(ST77XX_GREEN);
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void loop() {
  recvWithStartEndMarkers();
  parseData();
  btnloop();
}

void btnloop() {
  // Check for button presses
  btnM.loop();
  btnUp.loop();
  btnDown.loop();
  btnL.loop();
  btnR.loop();
  //displayMenu();
  // Handle button presses
  if (btnUp.getType() == single_click) {
    previousMenuItem();
  }
  else if (btnDown.getType() == single_click) {
    nextMenuItem();
  }
  else if (btnL.wasPressed()) {
    // Do something when left button is pressed
  }
  else if (btnR.wasPressed()) {
    // Do something when right button is pressed
  }
  else if (btnM.getType() == single_click) {
    // Run the selected program
    runProgram(selectedMenuItem);
  }
}
void displayMenu() {
  // Clear the screen
  tft.fillScreen(ST7735_BLACK);
  // Display the menu items
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_GREEN);
  clearscreen();
  clearscreen();
  clearscreen();
  clearscreen();
  clearscreen();
  clearscreen();
  clearscreen();
  tft.println("=============");
  tft.setTextColor(ST7735_WHITE);
  for (int i = 0; i < sizeof(menuItems) / sizeof(String); i++) {
    if (i == selectedMenuItem) {
      tft.setTextColor(ST7735_RED);
      tft.print("->");
    } else {
      tft.setTextColor(ST7735_WHITE);
      tft.print("  ");
    }
    tft.println(menuItems[i]);
  }
  tft.setTextColor(ST7735_GREEN);
  tft.println("=============");
}


void previousMenuItem() {
  // Decrement the selected menu item index, wrapping around if necessary
  selectedMenuItem--;
  if (selectedMenuItem < 0) {
    selectedMenuItem = sizeof(menuItems) / sizeof(String) - 1;
  }

  // Redraw the menu
  displayMenu();
}

void nextMenuItem() {
  // Increment the selected menu item index, wrapping around if necessary
  selectedMenuItem++;
  if (selectedMenuItem >= sizeof(menuItems) / sizeof(String)) {
    selectedMenuItem = 0;
  }

  // Redraw the menu
  displayMenu();
}

void runProgram(int programIndex) {
  // Run the selected program based on its index
  switch (programIndex) {
    case 0:
      // Run program 1
      scani2c();
      break;
    case 1:
      // Run program 2
                  static bool seti2cclock = false;
            clearscreen();
            if (seti2cclock) {
            Wire.setClock(100000UL);
            seti2cclock = false;
            tft.println(F("I2C to 100KHz")); }
            else { Wire.setClock(400000UL);
                   seti2cclock = true;
                   tft.println(F("I2C to 400KHz")); }
      break;
    case 2:
      // Run program 3
      ttlsetup(115200);
      break;
    case 3:
      // Run program 4
      printLocalTime();
      break;
    case 4:
      // Run program 5
      {
        ESP.restart();
            }
      break;
    default:
      break;
  }
}

void recvWithStartEndMarkers() {
  static bool recvInProgress = false;
  static byte ndx = 0;
  char rc;
  if(!client.connected()) { // if client not connected
    client = server.available(); // wait for it to connect
    digitalWrite(ledPinR, LOW);
  } else digitalWrite(ledPinR, HIGH);
  while (( client.available() || Serial.available() ) && newData == false)  {
      if (Serial.available() > 0)
      rc = Serial.read();
      else rc = (uint8_t)client.read();
    if (rc == endMarker) {
      recvInProgress = false;
      newData = true;
      receivedChars[ndx] = 0;
      strcpy(tempChars, receivedChars);
    }
    
    if(recvInProgress) {
      receivedChars[ndx] = rc;
      ndx ++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }

    if (rc == startMarker) { 
      ndx = 0; 
      recvInProgress = true;
    }
  }
}


void parseData() {
    if (newData == true) {
    int select = sscanf(tempChars, "%[^,],%[^,],%[^,],%s", opCode, tmpdevice, tmpregister, tmpdata);
    
     switch(select) {
      case 1: {
        if ( strcmp(opCode,"s") == 0 )
        scani2c();
        else if ( strcmp(opCode,"h") == 0 ) {
           Serial.println(F("Format as below:\n1.scan i2c: <s>\n2.dump i2c: <d,addr>\n3.write or read i2c: <w/r,addr,reg,value>\n4.Enter TTL mode and set serial baud rate: <t,baud>\n"));
           client.println(F("Format as below:\n1.scan i2c: <s>\n2.dump i2c: <d,addr>\n3.write or read i2c: <w/r,addr,reg,value>\n4.Enter TTL mode and set serial baud rate: <t,baud>\n"));
        }
        break; }
      case 2: {  
    if ( strcmp(opCode,"d") == 0 ) {
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        clearscreen();
        dumpi2c(deviceAddress);
        }
    else if ( strcmp(opCode,"t" ) == 0 ) {
        int BAUD = atoi(tmpdevice);
        client.stop();
        ttlsetup(BAUD); 
        }
        break; }
      case 3: {
        clearscreen();
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        byte registerAddress = (byte)strtol(tmpregister, NULL, 16);
        byte data = readI2C(deviceAddress, registerAddress);
        printlog(select, deviceAddress, registerAddress, data);
        tft.setTextColor(ST77XX_GREEN);
        break; }
      case 4: {
        clearscreen();
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        byte registerAddress = (byte)strtol(tmpregister, NULL, 16);
        byte data = (byte)strtol(tmpdata, NULL, 16);
        byte result = writeI2C(deviceAddress, registerAddress, data);
        printlog(select, deviceAddress, registerAddress, data); 
        tft.setTextColor(ST77XX_GREEN);
        break; }
     }
   newData = false;
    }
}

void scani2c() {
  byte error, address;
  int nDevices;
  char logString[30];
  clearscreen();
  sprintf(logString, "Scanning...");
  Serial.println(logString);
  tft.println(logString);
  client.println(logString);
  nDevices = 0;
  for(address = 5; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    flashLED();
    if (error == 0) {
      clearscreen();
      sprintf(logString, "Found 0x%02X...", address);
      Serial.println(logString);
      tft.println(logString);
      client.println(logString);
      flashLED();
      nDevices++;
    }
    else if (error==4) {
      clearscreen();
      sprintf(logString, "Unknown 0x%02X...", address);
      Serial.println(logString);
      tft.println(logString);
      client.println(logString);
    }
  }
  if (nDevices == 0) {
    clearscreen();
    sprintf(logString, "No addr found");
    Serial.println(logString);
    tft.println(logString);
    client.println(logString);
    flashLED();
  } else  {
    clearscreen();
    sprintf(logString, "Found %d devs.", nDevices);
    Serial.println(logString);
    tft.setTextColor(ST7735_BLUE);
    tft.println(logString);
    tft.setTextColor(ST7735_GREEN);
    client.println(logString);
  }
}

  void dumpi2c(byte deviceAddress) {
  byte error;
  byte data[16];
  int address, row, i;
  char logString[100];
  sprintf(logString, "Dump 0x%02X...", deviceAddress);
  Serial.println(logString);
  clearscreen();
  tft.println(logString);
  client.println(logString);

  sprintf(logString, "    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F");
  Serial.println(logString);
  client.println(logString);

  for (address = 0; address <= 255; address += 16) {
    sprintf(logString, "%02X: ", address);
    for (row = 0; row < 16; row++) {
      if (address + row <= 255) {
        Wire.beginTransmission(deviceAddress);
        Wire.write(address + row);
        error = Wire.endTransmission();
        if (error == 0) {
          Wire.requestFrom(deviceAddress, 1);
          if (Wire.available()) {
            data[row] = Wire.read();
            sprintf(logString + strlen(logString), "%02X ", data[row]); // Remove leading zero
          } else {
            sprintf(logString + strlen(logString), "?? ");
          }
        } else {
          sprintf(logString + strlen(logString), "-- ");
        }
      } else {
        sprintf(logString + strlen(logString), "   ");
      }
    }
    Serial.println(logString);
    //tft.println(logString);
    client.println(logString);
  }
}



void clearscreen() {
    static byte clearscreen = 0;
      if (clearscreen >= 20) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0,0);
        clearscreen = 0;
      }
    clearscreen++;
    flashLED();
    }


void flashLED() {
  static bool ledState = false;
  static unsigned long prevLedTime = 0;
  if (millis() - prevLedTime >= LEDinterval) {
    prevLedTime += LEDinterval;
    ledState = !ledState;
    digitalWrite(ledPinR, ledState);
  }
}

byte readI2C(byte deviceAddress, byte registerAddress) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  if (Wire.endTransmission(false) != 0) {
    tft.setTextColor(ST77XX_MAGENTA);
    Serial.print(F("Error "));
    //tft.print(F("Error "));
    client.print(F("Error "));
    return 255;
  }
  Wire.requestFrom(deviceAddress, 1);
  if (Wire.available()) {
    byte result = Wire.read();
    return result;
  } else {
    tft.setTextColor(ST77XX_MAGENTA);
    Serial.print(F("Error "));
    //tft.print(F("Error "));
    client.print(F("Error "));
    return 255;
  }
}

bool writeI2C(byte deviceAddress, byte registerAddress, byte data) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  Wire.write(data);
  if (Wire.endTransmission() != 0) {
    tft.setTextColor(ST77XX_BLUE);
    Serial.print(F("Error ")); 
    //tft.print(F("Error "));
    client.print(F("Error "));
    //tft.setTextColor(ST77XX_GREEN);
    return 255;
  }
  else return 1;
 }

void printlog(int select, byte deviceAddress, byte registerAddress, byte data) {
  char logString[30];
  sprintf(logString, "%c 0x%02X,%02X,%02X", (select == 3) ? 'r' : 'w', deviceAddress, registerAddress, data);

  Serial.println(logString);
  tft.println(logString);
  client.println(logString);
}


void ttlsetup ( int BAUD ) {
 Serial.begin(BAUD);
 Serial1.begin(BAUD, SERIAL_8N1, RXPIN, TXPIN);
 const byte bufferSize = 1024;
 uint8_t buf1[bufferSize];
 uint16_t i1=0;

 uint8_t buf2[bufferSize];
 uint16_t i2=0;
 bool isTcpConnected = false; // Flag to indicate whether a TCP client is connected
  clearscreen();
  Serial.println(F("19:RX 18:TX"));
  client.println(F("19:RX 18:TX"));
  tft.println(F("19:RX 18:TX"));
  clearscreen();
  Serial.print(F("Set baud to "));
  client.print(F("Set baud to "));
  tft.print(F("Baud "));
  Serial.println(BAUD);
  client.println(BAUD);
  tft.println(BAUD);
    
 while(digitalRead(btn_M) != LOW) {
  // Handle TCP client connection
  WiFiClient client = server.available();
  if (client) {
    Serial.println("TTL mode:TCP client connected");
    client.println("TTL mode:TCP client connected");
    clearscreen();
    tft.println("TTL Mode");
    isTcpConnected = true;

    while (client.connected() && (digitalRead(btn_M) == HIGH))  {
      digitalWrite(ledPinR, HIGH);
      while(Serial1.available()) {
      buf2[i2] = (char)Serial1.read(); // read char from UART
      if(i2<bufferSize-1) i2++;
    }
    // now send to WiFi:
    client.write((char*)buf2, i2);
    i2 = 0;       
  while(client.available()) {
      buf1[i1] = (uint8_t)client.read(); // read char from client 
      if(i1<bufferSize-1) i1++;
    }
    // now send to UART:
    Serial1.write(buf1, i1);
    i1 = 0;
  }
    client.stop();  
    digitalWrite(ledPinR, LOW);
    Serial.println("TCP client disconnected");
    isTcpConnected = false;
  }

  // If no TCP client is connected, bridge between Serial and HardwareSerial
  if (!isTcpConnected) {
    if (Serial.available()) {
      Serial1.write(Serial.read());
      flashLED();
    }
    if (Serial1.available()) {
      Serial.write(Serial1.read());
      flashLED();
    }
  }
 }
    Serial.println("Enter to I2C mode");
    //Serial.begin(UART_BAUD);
    clearscreen();
    tft.println("I2C Mode");
    delay(500);
}
