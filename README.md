# mcu-to-i2cshell-tool
Through serial port on the MCU board as a i2c and ttl tools 
Function as below
1. Support most MCUs, such as esp8266, esp32C3, atmega32u4 and so on. General, if you can complie the code via Arduino, then you can use this code to achieve this function.
2. This tool both can as a i2c tool and a TTL tool
3. If you MCU board is esp8266 or esp32, you can use the tcp port tool to send the command via WIFI of local network.
4. The I2C frequence can be set by the button, it support 100Khz and 400Khz.
5. Support I2C devices scan and list all the devices, support dump the device, support write and read the regesister.
7. If you connect the MCU to Internet, you can through press botton to get the clock time from the Internet.
8. The example is ESP32C3 and a 0.96 inch screen with St7735s, the size is 160x80.
9. After you burn the complie file to the MCU board, the screen which show the menu, you can press the button to choose multifunction.
10. The default baud rate is 115200, the COM poart base on you computer generate
11. About the serial command formart as below, you can send your command follow below formart.
    --scan i2c: <s>
    
    dump i2c: <d,addr>

    write or read i2c: <w/r,addr,reg,value>

    Enter TTL mode and set serial baud rate: <t,baud>

    
