#include <FS.h>  
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#define PROTOCOL_TCP
#define UART_BAUD 4800
#include <WiFiClient.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(0, 2); // D3:RX, D4:TX
#define bufferSize 1024
const int port = 9876;

#include <NTPtimeESP.h> //for time setting
//#define DEBUG_ON //for time setting
NTPtime NTPch("ch.pool.ntp.org");
strDateTime dateTime;
  
#include <Wire.h>
#include <stdio.h>
#include <string.h>
#include <Adafruit_ssd1306syp.h>
#define SDA_PIN 12
#define SCL_PIN 14
#define RESET_WATCHDOG

Adafruit_ssd1306syp display(SDA_PIN,SCL_PIN);
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];
const int ledPin = 15;
const int buttonPin = 13;
bool newData = false;

WiFiClient client;
WiFiServer server(port);

  
void wifisetup() {

  WiFiManager wifiManager;
  wifiManager.setBreakAfterConfig(false);
  wifiManager.setDebugOutput(false);
  wifiManager.setTimeout(15); //waiting 15s to setup the wifi then exit configure in ap mode
  //tries to connect to last known settings
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP" with password "password"
  //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("Tools", "liugengrong")) {
    Serial.println(F("AP:192.168.4.1")); display.println(F("AP:192.168.4.1"));
    clearscreen();
  }
  else {
  Serial.print(F("STA: "));
  display.print(F("STA:"));
  Serial.println(WiFi.localIP());
  display.println(WiFi.localIP());
  clearscreen(); }


  #ifdef PROTOCOL_TCP
  server.begin(); // start TCP server 
  #endif
  }

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  Wire.begin();
  Serial.begin(UART_BAUD);
  display.initialize();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(F("Type <h> for help"));
  clearscreen();
  Serial.println(F("Type <h> for help"));
  scani2c();
  wifisetup();
}

void loop() {
  settime();
  if (digitalRead(buttonPin) == LOW) scani2c();
  recvWithStartEndMarkers();
  if (newData == true) {
    char opCode[2],tmpdevice[10],tmpregister[10], tmpdata[10];
    byte deviceAddress[10],registerAddress[10], data[10];
    char value[8];
    int index=0;
    int select = sscanf(tempChars, "%[^,],%[^,],%[^,],%s", opCode, tmpdevice, tmpregister, tmpdata);
    
     switch(select) {
      case 1: {
        if ( strcmp(opCode,"s") == 0 )
        scani2c();
        else if ( strcmp(opCode,"h") == 0 ) {
           Serial.println(F("Format as below:"));
           Serial.println(F("1.scan i2c: <s>"));
           Serial.println(F("2.dump i2c: <d,addr>")); 
           Serial.println(F("3.write or read i2c: <w/r,addr,reg,value>")); 
           Serial.println(F("4.Enter TTL mode and set serial baud rate: <t,baud>"));     

           client.println(F("Format as below:"));
           client.println(F("1.scan i2c: <s>"));
           client.println(F("2.dump i2c: <d,addr>")); 
           client.println(F("3.write or read i2c: <w/r,addr,reg,value>")); 
           client.println(F("4.Enter TTL mode and set serial baud rate: <t,baud>"));  
        }
        break; }
      case 2: {  
    if ( strcmp(opCode,"d") == 0 ) {
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        dumpi2c(deviceAddress);
        clearscreen();}
    else if ( strcmp(opCode,"t" ) == 0 ) {
        int BAUD = atoi(tmpdevice);
        Serial.println(F("D3:RX D4:TX"));
        client.println(F("D3:RX D4:TX"));
        display.println(F("D3:RX D4:TX"));
        clearscreen();
        Serial.print(F("Set baud to "));
        client.print(F("Set baud to "));
        display.print(F("Set baud to "));
        Serial.println(BAUD);
        client.println(BAUD);
        display.println(BAUD);
        clearscreen();
        delay(200);
        client.stop();
        ttlsetup(BAUD); 
        }
        break; }
      case 3: {
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        byte registerAddress = (byte)strtol(tmpregister, NULL, 16);
        byte data = readI2C(deviceAddress, registerAddress);
        printlog(select, deviceAddress, registerAddress, data);
        clearscreen();
        break; }
      case 4: {
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        byte registerAddress = (byte)strtol(tmpregister, NULL, 16);
        byte data = (byte)strtol(tmpdata, NULL, 16);
        byte result = writeI2C(deviceAddress, registerAddress, data);
        printlog(select, deviceAddress, registerAddress, data);
        clearscreen();
        break; }
     }
   newData = false;
    }
  }

void recvWithStartEndMarkers() {
  static bool recvInProgress = false;
  static byte ndx = 0;
  const char startMarker = '<';
  const char endMarker = '>';
  char rc;
  if(!client.connected()) { // if client not connected
    client = server.available(); // wait for it to connect
    digitalWrite(ledPin, HIGH);
  } else digitalWrite(ledPin, LOW);
  while (( client.available() > 0 || Serial.available() > 0 ) && newData == false)  {
      if ( client.available() > 0 )
      rc = (uint8_t)client.read();
      else if ( Serial.available() > 0 )
      rc = Serial.read();
    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      } else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        strcpy(tempChars, receivedChars);
        newData = true;
      }
    } else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void scani2c() {
      byte error, address;
      int nDevices;
      Serial.println(F("Scanning..."));
      display.println(F("Scanning..."));
      client.println(F("Scanning..."));
      clearscreen();
      nDevices = 0;
      for(address = 6; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        ledblink(15,20,1);
        if (error == 0) {
          Serial.print(F("Found addr 0x"));
          display.print(F("Found addr 0x"));
          client.print(F("Found addr 0x"));
          if (address<16) {
            Serial.print(F("0"));
            display.print(F("0"));
            client.print(F("0"));
          }
          Serial.println(address,HEX);
          display.println(address,HEX);
          client.println(address,HEX);
          clearscreen();
          ledblink(100,50,15);
          nDevices++;
        }
        else if (error==4) {
          Serial.print(F("Unknown addr 0x"));
          display.print(F("Unknown addr 0x"));
          client.print(F("Unknown addr 0x"));
          if (address<16) {
            Serial.print(F("0"));
            display.print(F("0"));
            client.print(F("0"));
          }
          Serial.println(address,HEX);
          display.println(address,HEX);
          client.println(address,HEX);
          clearscreen();
        }
      }
      if (nDevices == 0) {
        Serial.println(F("No addr found"));
        display.println(F("No addr found"));
        client.println(F("No addr found"));
      }
      else  {Serial.println(F("Done")); client.println(F("Done"));}
      clearscreen();
}

  void dumpi2c(byte deviceAddress) {
  byte error, data;
  int address, row, i;
  Serial.print(F("Dumping 0x"));
  display.print(F("Dumping 0x"));
  client.print(F("Dumping 0x"));
  if (deviceAddress < 16) {
    Serial.print(F("0"));
    display.print(F("0"));
    client.print(F("0"));
  }
  Serial.print(deviceAddress, HEX);
  display.print(deviceAddress, HEX);
  client.print(deviceAddress, HEX);
  Serial.println(F("..."));
  display.println(F("..."));
  client.println(F("..."));
  Serial.println(F("    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F"));
  client.println(F("    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F"));
  for (address = 0; address <= 255; address += 16) {
    ESP.wdtEnable(0);
    if (address == 0) {
    Serial.print(F("0")); client.print(F("0")); }
    Serial.print(address, HEX); client.print(address, HEX);
    Serial.print(F(": ")); client.print(F(": "));
    for (row = 0; row < 16; row++) {
      if (address + row <= 255) {
        Wire.beginTransmission(deviceAddress);
        Wire.write(address + row);
        error = Wire.endTransmission();
        if (error == 0) {
          Wire.requestFrom(deviceAddress, 1);
          if (Wire.available()) {
            data = Wire.read();
            if (data < 16) {
              Serial.print(F("0")); client.print(F("0"));
            }
            Serial.print(data, HEX); client.print(data, HEX);
            Serial.print(F(" ")); client.print(F(" "));
          } else {
            Serial.print(F("?? ")); client.print(F("?? "));
          }
        } else {
          Serial.print(F("-- ")); client.print(F("-- "));
        }
      } else {
        Serial.print(F("   ")); client.print(F("   "));
      }
    }
    Serial.println();client.println();
  }
}

void clearscreen() {
    static byte clearscreen = 0;
    clearscreen++;
    display.update();
      if (clearscreen >= 8) {
        display.clear();
        display.setCursor(0,0);
        clearscreen = 0;
      }
    }

void ledblink(unsigned long ontime ,unsigned long offtime,int count) {

      for (int i = 0; i < count; i++) {
         if (digitalRead(ledPin) == LOW) {
         digitalWrite(ledPin, HIGH);
         delay(ontime);
         } else {
         digitalWrite(ledPin, LOW);
         delay(offtime);
         }
       }
      }

byte readI2C(byte deviceAddress, byte registerAddress) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  if (Wire.endTransmission() != 0) {
    Serial.print(F("Error ")); display.print(F("Error ")); client.print(F("Error "));
    return 255;
  }
  Wire.requestFrom(deviceAddress, 1);
  if (Wire.available()) {
    byte result = Wire.read();
    return result;
  }
}

bool writeI2C(byte deviceAddress, byte registerAddress, byte data) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  Wire.write(data);
  byte result=Wire.endTransmission();
  if (result != 0) {
    Serial.print(F("Error ")); display.print(F("Error "));client.print(F("Error "));
    return 255;
  }
  else { Wire.requestFrom(deviceAddress, 1);
       if (Wire.available()) { byte result = Wire.read();return result;}
        }
 }

void printlog(int select, byte deviceAddress, byte registerAddress, byte data) {
     if (select == 3) { Serial.print(F("r 0x")); display.print(F("r 0x")); client.print(F("r 0x"));}
     if (select == 4) { Serial.print(F("w 0x")); display.print(F("w 0x")); client.print(F("w 0x"));} 
     if (deviceAddress < 16) { Serial.print("0"); display.print("0"); client.print("0"); }
     Serial.print(deviceAddress, HEX);
     display.print(deviceAddress, HEX);
     client.print(deviceAddress, HEX);
     Serial.print(F(","));
     display.print(F(","));
     client.print(F(","));
     if (registerAddress < 16) { Serial.print("0"); display.print("0"); client.print("0");}
     Serial.print(registerAddress, HEX);
     display.print(registerAddress, HEX);
     client.print(registerAddress, HEX);
     Serial.print(F(","));
     display.print(F(","));
     client.print(F(","));
     if (data < 16) { Serial.print(F("0")); display.print(F("0")); client.print(F("0")); }
     Serial.println(data, HEX); 
     display.println(data, HEX);
     client.println(data, HEX);
    }
void ttlsetup ( int BAUD ) {
 Serial.begin(BAUD);
 mySerial.begin(BAUD);
 uint8_t buf1[bufferSize];
 uint16_t i1=0;

 uint8_t buf2[bufferSize];
 uint16_t i2=0;
 bool isTcpConnected = false; // Flag to indicate whether a TCP client is connected
 
 while(digitalRead(buttonPin) == HIGH) {
  // Handle TCP client connection
  WiFiClient client = server.available();
  if (client) {
    Serial.println("TTL mode:TCP client connected");
    client.println("TTL mode:TCP client connected");
    display.println("Now is TTL mode");
    clearscreen();
    isTcpConnected = true;

    while (client.connected() && (digitalRead(buttonPin) == HIGH))  {
      digitalWrite(ledPin, LOW);
      while(mySerial.available()) {
      buf2[i2] = (char)mySerial.read(); // read char from UART
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
    mySerial.write(buf1, i1);
    i1 = 0;
  }
    client.stop();  
    digitalWrite(ledPin, HIGH);
    Serial.println("TCP client disconnected");
    isTcpConnected = false;
  }

  // If no TCP client is connected, bridge between Serial and HardwareSerial
  if (!isTcpConnected) {
    if (Serial.available()) {
      mySerial.write(Serial.read());
    }
    if (mySerial.available()) {
      Serial.write(mySerial.read());
    }
  }
 }
    Serial.println("Enter to I2C mode");
    display.println("Now is I2C mode");
    clearscreen();
}
void settime() {

  // first parameter: Time zone in floating point (for India); second parameter: 1 for European summer time; 2 for US daylight saving time; 0 for no DST adjustment; (contributed by viewwer, not tested by me)
  dateTime = NTPch.getNTPtime(8.0, 0);

  // check dateTime.valid before using the returned time
  // Use "setSendInterval" or "setRecvTimeout" if required
  if(dateTime.valid){
    NTPch.printDateTime(dateTime);
    
    byte actualHour = dateTime.hour;
    byte actualMinute = dateTime.minute;
    byte actualsecond = dateTime.second;
    int actualyear = dateTime.year;
    byte actualMonth = dateTime.month;
    byte actualday =dateTime.day;
    byte actualdayofWeek = dateTime.dayofWeek;
    display.print(actualyear);
    display.print( "/");
    display.print(actualMonth);
    display.print( "/");
    display.print(actualday);
    display.print( " ");
    switch(actualdayofWeek) {
    case 1:
    display.print(F("Mon"));
    break;
    case 2:
    display.print(F("Tue"));
    break;
    case 3:
    display.print(F("Wed"));
    break;
    case 4:
    display.print(F("Thur"));
    break;
    case 5:
    display.print(F("Fri"));
    break;
    case 6:
    display.print(F("Sat"));
    break;
    case 7:
    display.print(F("Sun"));
    break;
    }
    display.print( " ");
    display.print(actualHour);
    display.print( ":");
    display.print(actualMinute);
    display.println();
    clearscreen();
  }
}