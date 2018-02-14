
/*
This sketch is an example of using the NTPTime library to get local time from an NTP server
The NTPtime library adapts automatically the day and the hour according to the leap year and daylight saving time
It also configure the appropriate library according to the processor type like:
 ESP32 Devices such as the Wemos Lolin 32
 ESP8266 Wemos Mini D1 and D2 and generic ESP8266
 Arduino Ethernet
The NTPTime library includes the WiFi, Ethernet and UDP libraries that do not need to be included in the sketch
   However the sketch should also be adapted to the appropriate Ethernet IP settings for the particular device
   Typically ESP32 and ESP8266 (Wifi) or Arduino Ethernet (Ethernet)
   Note that in both cases DHCP is used
Appropriate NTP Time server pool may have to be selected according the particular country, to improve the reply time   
More information of the library function are described in the library code.
*/

#include <NTPtime.h>                 // Include the NTPTime library

String NTPServer = "be.pool.ntp.org";
NTPtime NTPbe(NTPServer);           // Choose server pool as required (here Belgium)
strDateTime dateTime;               // Define a dateTime instance
float GMT;                          // GMT ofsset use decimal value; eg. 1.0 
byte dayLight;                      // Day Light saving time option (0: none - 1 European; 2-USA) 
boolean amPm;                       // 24 /12 option (1: 12 housr)          
long int unixTime;                  // UNIX time to convert

void setup()
{
  Serial.begin(115200);
#if defined (ARDUINO_AVR_ETHERNET)              // Configure the Ethernet / IP in case of Arduino Ethernet
  static byte myMac[] = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}; // Server MAC address (Locally Administrated)  
  Serial.println("Connecting to DHCP Server");
  while (Ethernet.begin(myMac) == 0)            // Wait until IP address is configured
  {  
    Serial.print(".");                    
    delay (500);
  }
   Serial.println ("Connected to IP Network"); 
#else                                             //Configure the Wifi in case of ESP32 or ESP8266
  const char *ssid      = "";                // Set you WiFi SSID for ESP32 or ESP8266  
  const char *password  = "";             // Set you WiFi password for ESP32 or ESP8266  
  Serial.println("Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin (ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected");
#endif
  NTPbe.setSendInterval (10);                     // Set NTP request interval time out to 10 seconds
  NTPbe.setRecvTimeout (2);                       // Set NTP request receive time out to 2 seconds
}

void loop() {
   // first parameter: Time zone in floating point (for India); second parameter: 1 for European summer time; 2 for US daylight saving time (contributed by viewwer, not tested by me), third parameter is AM/PM option
   // fourth parameter: is UNIX time to convert (0 means actual NTP time)
   GMT = 1.0;                  // Central Europe
   dayLight = 1;               // Europe
   amPm     = 0;               // 24 hours format
   unixTime = 1514112900;      // if != 0; Unix time to convert to local time
   Serial.print ("UNIX time: "); Serial.print (unixTime) ; Serial.print (" - GMT = "), 
   Serial.print (GMT); Serial.print (" - DayLightSaving Time is: "), Serial.print (dayLight);  
   Serial.print (" - 12 hours Display is set to: "), Serial.println (amPm?"TRUE":"FALSE");  Serial.print ("Local Time: ");
   NTPbe.printDateTime(NTPbe.getNTPtime(GMT, dayLight, amPm, unixTime));    // Example using the printDateTime function
   
   delay (20000);
   GMT = 1.0;                 // Central Europe
   dayLight = 0;              // None
   amPm     = 1;              // 12 hours format
   unixTime = 0;                                                  // if != 0; Unix time to convert to local time
   dateTime = NTPbe.getNTPtime(GMT, dayLight, amPm, unixTime);              // Call NTP get or time conversion
   // check dateTime.valid before using the returned time
   // Use "setSendInterval" or "setRecvTimeout" if require
   if(dateTime.valid)
   {
      Serial.print("Actual time from NTP Server: "); Serial.println (NTPServer);
      Serial.print ("GMT = "), Serial.print (GMT); Serial.print (" - DayLightSaving Time is: "), Serial.print (dayLight);
      Serial.print (" - 12 hours Display is set to: "), Serial.println (amPm?"TRUE":"FALSE");  Serial.print ("NTP Time: ");
      NTPbe.printDateTime(dateTime);                              // Example using the printDateTime function
   }
}

