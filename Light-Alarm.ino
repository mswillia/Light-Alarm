/*
 * Light alarm clock V0.1
 * Created November 1st 2015
 * by gannon
 * 
 * Based heavily on the Time_NTP and NTPClient example sketches
 * 
 * TODO:
 *   Add RTC support
 *   Add better alarm support
 */
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Time.h>

//=============CONFIGURATION==============
char ssid[] = "";  // your network SSID (name)
char pass[] = "";  // your network password

const  long timeZoneOffset = 18000L; // set this to the offset in seconds to your local time;
 
long alarm = 27000L; //07:30
int range = 1200; //time around alarm for light to be on

const int pwmPin = 14; //PWM output pin
//===========END CONFIGURATION=============

int pwm = 0; //PWM output value

time_t prevDisplay = 0; // when the digital clock was displayed

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";  //MUST use time.mtu.edu on MTU campus

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  pinMode(pwmPin, OUTPUT);
  analogWrite(pwmPin, 0);
  
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
  setSyncProvider(getNtpTime);
  while(timeStatus()== timeNotSet); // wait until the time is set by the sync provider
}

void loop(){  
  if( now() != prevDisplay) {
    prevDisplay = now();
    digitalClockDisplay();  
  }
}

void digitalClockDisplay(){

  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
  
  if (weekday() > 1 && weekday() < 7) { //weekdays
    long secondOfDay = (hour() * 60 * 60) + minute() * 60 + second();
    if (abs(secondOfDay - alarm) <= range) {
      pwm = 1024 * (1.0 - abs(secondOfDay - alarm) / (float)range);
      if (pwm < 0) {
        pwm = 0;
      }
    }
  }
  
  Serial.print("PWM: ");
  Serial.print(pwm); 
  Serial.println();
  analogWrite(pwmPin, pwm);
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

//=======NTP CODE=======

long getNtpTime() {
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (cb)  {
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL + timeZoneOffset;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

    return epoch;
  }
  return 0;
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address) {
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
