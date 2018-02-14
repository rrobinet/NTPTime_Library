/*
   This routine gets the unixtime from a NTP server and adjusts it to the time zone 
  Author: Andreas Spiess V1.0 2016-6-28
   - Based on work from John Lassen: http://www.john-lassen.de/index.php/projects/esp-8266-arduino-ide-webconfig
  Modified by ROBVAN v1.1 2017-12-7
   - Support boards such as Wemos LOLIN32 - Wemos mini and Arduino Ethernet
   - Correct DaylightSaving variable type from boolean to byte to cope with the US summerTime function (!not tested)
   - Add 12 or 24 hours option (AM/PM)
   - Adapt the Send Interval delay to be minimum 10 seconds
   - Implement UNIX time to local time conversion rather than actual NTP time 
*/

#ifndef NTPtime_h
#define NTPtime_h
#endif
// Check for ESP32 Devices
#if defined (ARDUINO_LOLIN32)
 #include <WiFi.h>                	 	// ESP32 WiFi library
 #include <WiFiUdp.h>					// WiFi UDP library
// Check for ESP8266 Devices 
#elif defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined (ARDUINO_ESP8266_ESP01)
 #include <ESP8266WiFi.h>            	// ESP8266 WiFi library
 #include <WiFiUdp.h>					// WiFi UDP library
#endif 
#if defined(ARDUINO_AVR_ETHERNET) 
// Use by default Arduino Ethernet Libary
 #include <Ethernet.h>               	// Standard Ethernet Library used by the Ethernet Shield
 #include <EthernetUdp.h>            	// Ethernet UDP library
 #include <Dns.h>						// DNS library
#endif

//#define DEBUG_ON

struct strDateTime
{
  byte hour;
  byte minute;
  byte second;
  int year;
  byte month;
  byte day;
  byte dayofWeek;
  String amPm = "  ";
  boolean valid;
};

class NTPtime {
  public:
    NTPtime(String NTPtime);
    strDateTime getNTPtime(float _timeZone, byte _DayLightSaving, boolean _amPm, unsigned long _unixTime);
    void printDateTime(strDateTime _dateTime);
    bool setSendInterval(unsigned long _sendInterval);  // in seconds
    bool setRecvTimeout(unsigned long _recvTimeout);    // in seconds

  private:
    bool _sendPhase;
    unsigned long _sentTime;
    unsigned long _sendInterval;
    unsigned long _recvTimeout;

    strDateTime ConvertUnixTimestamp( unsigned long _tempTimeStamp, boolean _amPm);
    boolean summerTime(unsigned long _timeStamp, boolean _amPm );
    boolean daylightSavingTime(unsigned long _timeStamp, boolean _amPm);
    unsigned long adjustTimeZone(unsigned long _timeStamp, float _timeZone, byte _DayLightSavingSaving, boolean amPm);
// Create UDP instance according to the Device type
#if defined (ARDUINO_AVR_ETHERNET)
	EthernetUDP UDPNTPClient;
	DNSClient dns;  // Add DNS instance for debugging Arduino Ethernet Devices	
#else
    WiFiUDP UDPNTPClient;  
#endif    
};
