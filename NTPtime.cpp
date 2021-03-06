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
   - Improve the printDateTime function 
   Modified by ROBVAN v1.2 2018-3-5   
   - Replace the strDateTime.valid boolean with uint8_t allowing to report on NTP host time-out
     while testing on data.available
   		strDateTime.valid = 0 - No Data
   		strDateTime.valid = 1 - Data Available
   		strDateTime.valid = 2 - Host Time-out   		
*/


#include <Arduino.h>
#include "NTPTime.h"

#define LEAP_YEAR(Y) ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

#define SEC_TO_MS             1000
#define RECV_TIMEOUT_DEFAULT  1       // 1 second
#define SEND_INTRVL_DEFAULT   10      // 10 second
#define MAX_RECV_TIMEOUT      60      // 60 seconds

const int NTP_PACKET_SIZE = 48;
byte _packetBuffer[ NTP_PACKET_SIZE];
static const uint8_t _monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

float _timeZone=0.0;
String _NTPserver="";

#ifdef DEBUG_ON
 IPAddress DNSServer (8,8,8,8);		// Add DNS Server IP address for debugging with Arduino Ethernet
#endif 


// NTPserver is the name of the NTPserver

bool NTPtime::setSendInterval(unsigned long _sendInterval_) {
	bool retVal = false;
	if(_sendInterval_ >= SEND_INTRVL_DEFAULT) {
		_sendInterval = _sendInterval_ * SEC_TO_MS;
		retVal = true;
	}

	return retVal;
}

bool NTPtime::setRecvTimeout(unsigned long _recvTimeout_) {
	bool retVal = false;
	if(_recvTimeout_ <= MAX_RECV_TIMEOUT) {
		_recvTimeout = _recvTimeout_ * SEC_TO_MS;
		retVal = true;
	}

	return retVal;  
}

NTPtime::NTPtime(String NTPserver) {
	_NTPserver = NTPserver;
	_sendPhase = true;
	_sentTime  = 0;
	_sendInterval = SEND_INTRVL_DEFAULT * SEC_TO_MS;
	_recvTimeout = RECV_TIMEOUT_DEFAULT * SEC_TO_MS;
// Start DNS Client while defining the NTP Server for Arduino Ethernet
#ifdef DEBUG_ON	
#if defined (ARDUINO_AVR_ETHERNET)
	dns.begin(DNSServer);
#endif	
#endif	
}

void NTPtime::printDateTime(strDateTime _dateTime) {
	if (_dateTime.valid) {
		switch (_dateTime.dayofWeek) 
      	{
    	    case (1):
        	Serial.print ("Sunday");
        	break;
        	case (2):
          	Serial.print ("Monday");
          	break;
        	case (3):
          	Serial.print ("Tuesday");
          	break;
        	case (4):
         	 Serial.print ("Wednesday");
          	break;
        	case (5):
          	Serial.print ("Thursday");
          	break;
        	case (6):
          	Serial.print ("Friday");
          	break;
        	case (7):
          	Serial.print ("Saturday");
          	break;
        	default:
          	Serial.print("Invalid Day");
        	break;       
      	}
      	Serial.print ("-"),
        Serial.print (_dateTime.day),
      	Serial.print ("-");
	  	switch (_dateTime.month) 
      	{
        	case (1):
     	    Serial.print ("January");
       		break;
       		case (2):
          	Serial.print ("February");
        	break;
        	case (3):
          	Serial.print ("Match");
        	break;
        	case (4):
          	Serial.print ("April");
        	break;
        	case (5):
          	Serial.print ("May");
        	break;
        	case (6):
          	Serial.print ("June");
        	break;
        	case (7):
          	Serial.print ("July");
        	break;
        	case (8):
          	Serial.print ("August");
        	break;
        	case (9):
          	Serial.print ("September");
        	break;
        	case (10):
          	Serial.print ("October");
        	break;
        	case (11):
          	Serial.print ("November");
        	break;
        	case (12):
          	Serial.print ("December");
        	break;
        	default:
          	Serial.print("Invalid Month");
        	break;
      	}
        Serial.print ("-"),
        Serial.print (_dateTime.year);
        Serial.print (" "),
      	Serial.print (_dateTime.hour), Serial.print(":"), 
      	Serial.print (_dateTime.minute), Serial.print("."), 
      	Serial.print (_dateTime.second), Serial.print (" "), 
      	Serial.println (_dateTime.amPm);
      	Serial.println();
	} 
	else 
	{
#ifdef DEBUG_ON
		Serial.println("Invalid time !!!");
		Serial.println("");
#endif    
	}
}

// Converts a unix time stamp to a strDateTime structure
strDateTime NTPtime::ConvertUnixTimestamp( unsigned long _tempTimeStamp, boolean _amPm) {
	strDateTime _tempDateTime;
	uint8_t _year, _month, _monthLength;
	uint32_t _time;
	unsigned long _days;

	_time = (uint32_t)_tempTimeStamp;
	_tempDateTime.second = _time % 60;
	_time /= 60; // now it is minutes
	_tempDateTime.minute = _time % 60;
	_time /= 60; // now it is hours
	_tempDateTime.hour = _time % 24;
	_time /= 24; // now it is _days
	_tempDateTime.dayofWeek = ((_time + 4) % 7) + 1;  // Sunday is day 1
	_year = 0;
	_days = 0;
	while ((unsigned)(_days += (LEAP_YEAR(_year) ? 366 : 365)) <= _time) {
		_year++;
	}
	_tempDateTime.year = _year; // year is offset from 1970

	_days -= LEAP_YEAR(_year) ? 366 : 365;
	_time  -= _days; // now it is days in this year, starting at 0

	_days = 0;
	_month = 0;
	_monthLength = 0;
	for (_month = 0; _month < 12; _month++) {
		if (_month == 1) { // february
			if (LEAP_YEAR(_year)) {
				_monthLength = 29;
			} else {
				_monthLength = 28;
			}
		} else {
			_monthLength = _monthDays[_month];
		}

		if (_time >= _monthLength) {
			_time -= _monthLength;
		} else {
			break;
		}
	}
	_tempDateTime.month = _month + 1;  // jan is month 1
	_tempDateTime.day = _time + 1;     // day of month
	_tempDateTime.year += 1970;

// Correct hours according to the 12 /24 setting
	if (_amPm)
	{
		if (_tempDateTime.hour>=12)
		{
	      _tempDateTime.amPm= "PM";
	      if (_tempDateTime.hour>=13) _tempDateTime.hour=_tempDateTime.hour-12;
		}
	    else _tempDateTime.amPm= "AM";
	}
	else   _tempDateTime.amPm= ""; 
	return _tempDateTime;
}


//
// Summertime calculates the daylight saving time for middle Europe. Input: Unixtime in UTC
//
boolean NTPtime::summerTime(unsigned long _timeStamp, boolean _amPm) {

	strDateTime  _tempDateTime;
	_tempDateTime = ConvertUnixTimestamp(_timeStamp, _amPm);
	// printTime("Innerhalb ", _tempDateTime);

	if (_tempDateTime.month < 3 || _tempDateTime.month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
	if (_tempDateTime.month > 3 && _tempDateTime.month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
	if (_tempDateTime.month == 3 && (_tempDateTime.hour + 24 * _tempDateTime.day) >= (3 +  24 * (31 - (5 * _tempDateTime.year / 4 + 4) % 7)) || _tempDateTime.month == 10 && (_tempDateTime.hour + 24 * _tempDateTime.day) < (3 +  24 * (31 - (5 * _tempDateTime.year / 4 + 1) % 7)))
	return true;
	else
	return false;
}

boolean NTPtime::daylightSavingTime(unsigned long _timeStamp, boolean _amPm) {

	strDateTime  _tempDateTime;
	_tempDateTime = ConvertUnixTimestamp(_timeStamp, _amPm);

	// here the US code
	//return false;
	// see http://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date
	// since 2007 DST begins on second Sunday of March and ends on first Sunday of November. 
	// Time change occurs at 2AM locally
	if (_tempDateTime.month < 3 || _tempDateTime.month > 11) return false;  //January, february, and december are out.
	if (_tempDateTime.month > 3 && _tempDateTime.month < 11) return true;   //April to October are in
	int previousSunday = _tempDateTime.day - (_tempDateTime.dayofWeek - 1);  // dow Sunday input was 1,
	// need it to be Sunday = 0. If 1st of month = Sunday, previousSunday=1-0=1
	//int previousSunday = day - (dow-1);
	// -------------------- March ---------------------------------------
	//In march, we are DST if our previous Sunday was = to or after the 8th.
	if (_tempDateTime.month == 3 ) {  // in march, if previous Sunday is after the 8th, is DST
		// unless Sunday and hour < 2am
		if ( previousSunday >= 8 ) { // Sunday = 1
			// return true if day > 14 or (dow == 1 and hour >= 2)
			return ((_tempDateTime.day > 14) || 
			((_tempDateTime.dayofWeek == 1 && _tempDateTime.hour >= 2) || _tempDateTime.dayofWeek > 1));
		} // end if ( previousSunday >= 8 && _dateTime.dayofWeek > 0 )
		else
		{
			// previousSunday has to be < 8 to get here
			//return (previousSunday < 8 && (_tempDateTime.dayofWeek - 1) = 0 && _tempDateTime.hour >= 2)
			return false;
		} // end else
	} // end if (_tempDateTime.month == 3 )

	// ------------------------------- November -------------------------------

	// gets here only if month = November
	//In november we must be before the first Sunday to be dst.
	//That means the previous Sunday must be before the 2nd.
	if (previousSunday < 1)
	{
		// is not true for Sunday after 2am or any day after 1st Sunday any time
		return ((_tempDateTime.dayofWeek == 1 && _tempDateTime.hour < 2) || (_tempDateTime.dayofWeek > 1));
		//return true;
	} // end if (previousSunday < 1)
	else
	{
		// return false unless after first wk and dow = Sunday and hour < 2
		return (_tempDateTime.day <8 && _tempDateTime.dayofWeek == 1 && _tempDateTime.hour < 2);
	}  // end else
} // end boolean NTPtime::daylightSavingTime(unsigned long _timeStamp)


unsigned long NTPtime::adjustTimeZone(unsigned long _timeStamp, float _timeZone, byte _DayLightSaving, boolean _amPm) {
    if (_DayLightSaving > 2) _DayLightSaving =0; // Incorrect option 
#ifdef DEBUG_ON
	Serial.print ("Day Light Saving time is set to: "); 
	if (_DayLightSaving == 0)
	{
		Serial.println ("None"); 
	}
	else if (_DayLightSaving == 1) Serial.println ("European");
	else Serial.println ("US");
    Serial.print ("12 hours Display is set to "), Serial.println (_amPm?"TRUE":"FALSE");
#endif	
	strDateTime _tempDateTime;
	_timeStamp += (unsigned long)(_timeZone *  3600.0); // adjust timezone
	if (_DayLightSaving ==1 && summerTime(_timeStamp, _amPm)) _timeStamp += 3600; // European Summer time
	if (_DayLightSaving ==2 && daylightSavingTime(_timeStamp, _amPm)) _timeStamp += 3600; // US daylight time
	return _timeStamp;
}

// parameter 1: Time zone is the difference to UTC in hours
// parameter 2: Day Light Saving Time 
//				if  not 0, time will be adjusted accordingly
// 						0 means no time adjust
// 						1 means European time adjust
// 						2 means US time adjust
// parameter 3: AM/PM option; 
//						0 means 24 hours mode
//						1 means 12 hours mode
// parameter 4: Unix time stamp to convert to local time 
//						0 means Actual NTP get time function
//						
// Use returned time only after checking "_dateTime.valid" flag

strDateTime NTPtime::getNTPtime(float _timeZone, byte _DayLightSaving, boolean _amPm, unsigned long _unixTime) {
	if (_DayLightSaving >2) _DayLightSaving = 0; // Do not set invalid Dayalight Saving time value
	int cb;
	strDateTime _dateTime;
//	unsigned long _unixTime = 0;
	_dateTime.valid = false;
	unsigned long _currentTimeStamp;
	if ( _unixTime != 0)
	{
		_currentTimeStamp = adjustTimeZone(_unixTime, _timeZone, _DayLightSaving, _amPm);
		_dateTime = ConvertUnixTimestamp(_currentTimeStamp, _amPm);
		_dateTime.valid = true;
	}
	else
	{
	    if (_sendPhase) {
		if (_sentTime && ((millis() - _sentTime) < _sendInterval)) {
			return _dateTime;
		}

		_sendPhase = false;
		UDPNTPClient.begin(1337); // Port for NTP receive

#ifdef DEBUG_ON
		IPAddress _timeServerIP;
		// Check syntax according the devive type
		#if defined (ARDUINO_AVR_ETHERNET)
		  dns.getHostByName(_NTPserver.c_str(), _timeServerIP);
		#else
		   WiFi.hostByName(_NTPserver.c_str(), _timeServerIP); 
		#endif    
		Serial.println();
		Serial.print("Time Server IP address is: ");
		Serial.println(_timeServerIP);
		Serial.println("Sending NTP packet");
#endif

		memset(_packetBuffer, 0, NTP_PACKET_SIZE);
		_packetBuffer[0] = 0b11100011; // LI, Version, Mode
		_packetBuffer[1] = 0;          // Stratum, or type of clock
		_packetBuffer[2] = 6;          // Polling Interval
		_packetBuffer[3] = 0xEC;       // Peer Clock Precision
		_packetBuffer[12] = 49;
		_packetBuffer[13] = 0x4E;
		_packetBuffer[14] = 49;
		_packetBuffer[15] = 52;
		UDPNTPClient.beginPacket(_NTPserver.c_str(), 123);
		UDPNTPClient.write(_packetBuffer, NTP_PACKET_SIZE);
		UDPNTPClient.endPacket();

		_sentTime = millis();
	} else {
		cb = UDPNTPClient.parsePacket();
		if (cb == 0) {
			if ((millis() - _sentTime) > _recvTimeout) {
				_sendPhase = true;
				_sentTime = 0;
			    _dateTime.valid = 2; // Added to indiacte a NTP host failure
			}
		} else {
#ifdef DEBUG_ON
			Serial.print("NTP packet received, length=");
			Serial.println(cb);
#endif

			UDPNTPClient.read(_packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
			unsigned long highWord = word(_packetBuffer[40], _packetBuffer[41]);
			unsigned long lowWord = word(_packetBuffer[42], _packetBuffer[43]);
			unsigned long secsSince1900 = highWord << 16 | lowWord;
			const unsigned long seventyYears = 2208988800UL;
			_unixTime = secsSince1900 - seventyYears;
			if (_unixTime > 0) {
				_currentTimeStamp = adjustTimeZone(_unixTime, _timeZone, _DayLightSaving, _amPm);
				_dateTime = ConvertUnixTimestamp(_currentTimeStamp, _amPm);
				_dateTime.valid = true;
			} else
			_dateTime.valid = false;

			_sendPhase = true;
		}
	}

	return _dateTime;
	}

}
