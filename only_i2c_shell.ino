#include <Wire.h>
#include <stdio.h>
#include <string.h>
//#include <SoftwareSerial.h>
#define SDA 0 //i2c
#define SCL 1 //i2c
#define RXPIN 18 //serial
#define TXPIN 19 //serial

const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];
char opCode[2],tmpdevice[10],tmpregister[10], tmpdata[10];
byte deviceAddress[10],registerAddress[10], data[10];
char value[8];

const char startMarker = '<';
const char endMarker = '>';

const int ledPin = 13;
unsigned long LEDinterval = 30;
unsigned long prevLEDmillis = 0;
unsigned long curMillis;

bool newData = false;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin,HIGH); //HIGH is ON
  flashLED();
  Wire.begin(SDA,SCL); //SDA,SCL
}

void loop() {
  recvWithStartEndMarkers();
  parseData();
  }

void recvWithStartEndMarkers() {
  static bool recvInProgress = false;
  static byte ndx = 0;
  char rc;

  while ( Serial.available() > 0 && newData == false)  {
      rc = Serial.read();
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
        }
        break; }
      case 2: {  
    if ( strcmp(opCode,"d") == 0 ) {
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        dumpi2c(deviceAddress);
        }
    else if ( strcmp(opCode,"t" ) == 0 ) {
        long BAUD = atol(tmpdevice);
        Serial.println(F("16:RX 17:TX"));
        Serial.println(F("Enter to TTL mode"));
        Serial.print(F("Set baud to "));
        Serial.println(BAUD);
        ttlsetup(BAUD); 
        }
        break; }
      case 3: {
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        byte registerAddress = (byte)strtol(tmpregister, NULL, 16);
        byte data = readI2C(deviceAddress, registerAddress);
        printlog(select, deviceAddress, registerAddress, data);
        flashLED();
        break; }
      case 4: {
        byte deviceAddress = (byte)strtol(tmpdevice, NULL, 16);
        byte registerAddress = (byte)strtol(tmpregister, NULL, 16);
        byte data = (byte)strtol(tmpdata, NULL, 16);
        byte result = writeI2C(deviceAddress, registerAddress, data);
        printlog(select, deviceAddress, registerAddress, data);
        flashLED();
        break; }
     }
   newData = false;
    }
}

void scani2c() {
      byte error, address;
      int nDevices;
      Serial.println(F("Scanning..."));
      nDevices = 0;
      for(address = 5; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        //ledblink(ledPin,15,20,1);
        flashLED();
        if (error == 0) {
          Serial.print(F("Found addr 0x"));
          if (address<16) {
            Serial.print(F("0"));
          }
          Serial.println(address,HEX);
          flashLED();
          //ledblink(ledPin,1,0,1);
          nDevices++;
        }
        else if (error==4) {
          Serial.print(F("Unknown addr 0x"));
          if (address<16) {
            Serial.print(F("0"));
          }
          Serial.println(address,HEX);
        }
      }
      if (nDevices == 0) {
        Serial.println(F("No addr found"));
        flashLED();
        //ledblink(ledPin,30,20,16);
      }
      else Serial.println(F("Done"));
}

byte readI2C(byte deviceAddress, byte registerAddress) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(deviceAddress, 1);
  if (Wire.available()) {
    byte result = Wire.read();
    return result;
  } else {
    Serial.print(F("Error ")); 
    return 255;
   }
}

bool writeI2C(byte deviceAddress, byte registerAddress, byte data) {
  Wire.beginTransmission(deviceAddress);
  Wire.write(registerAddress);
  Wire.write(data);
  byte result=Wire.endTransmission();
  if (result != 0) {
    Serial.print(F("Error "));
    return 255;
  }
  else return 1;
 }

void printlog(int select, byte deviceAddress, byte registerAddress, byte data) {
     if (select == 3) { Serial.print(F("r 0x"));}
     if (select == 4) { Serial.print(F("w 0x"));} 
     if (deviceAddress < 16) { Serial.print("0"); }
     Serial.print(deviceAddress, HEX);
     Serial.print(F(","));
     if (registerAddress < 16) { Serial.print("0");}
     Serial.print(registerAddress, HEX);
     Serial.print(F(","));
     if (data < 16) { Serial.print(F("0"));}
     Serial.println(data, HEX); 
    }

  void dumpi2c(byte deviceAddress) {
  byte error, data;
  int address, row, i;
  Serial.print(F("Dumping 0x"));
  if (deviceAddress < 16) {
    Serial.print(F("0"));
  }
  Serial.print(deviceAddress, HEX);
  Serial.println(F("..."));
  Serial.println(F("    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F"));
  for (address = 0; address <= 255; address += 16) {
    //ESP.wdtEnable(0);
    if (address == 0) {
    Serial.print(F("0")); }
    Serial.print(address, HEX);
    Serial.print(F(": ")); 
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
              Serial.print(F("0")); 
            }
            Serial.print(data, HEX); 
            Serial.print(F(" ")); 
          } else {
            Serial.print(F("?? "));
          }
        } else {
          Serial.print(F("-- ")); 
        }
      } else {
        Serial.print(F("   "));
      }
    }
    Serial.println();
  }
}


void flashLED() {
    curMillis = millis();
    if (curMillis - prevLEDmillis >= LEDinterval) {
       prevLEDmillis += LEDinterval;
       digitalWrite( ledPin, ! digitalRead( ledPin) );
    }
}

void ttlsetup ( long BAUD ) {

 Serial.begin(BAUD); 
 Serial1.begin(BAUD, SERIAL_8N1, RXPIN, TXPIN);// RX, TX
 while(1) {

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
