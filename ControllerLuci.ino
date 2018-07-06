#include <WiFiUdp.h>  
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

String token = "dk4i93bfo5k6c";

// API endpoint
String api = "/luci";

//port
WiFiServer server(8888);

short pinLed = D4; // pin setup
short status = 1;  // led 1 spento

short local_hour_offset_UTC = 1;

short h_turnsOn_morning = 7 - local_hour_offset_UTC;
short h_turnsOff_morning = 9 - local_hour_offset_UTC;

short h_turnsOn_evening = 20 - local_hour_offset_UTC;
short h_turnsOff_evening = 24 - local_hour_offset_UTC;

short h_turnsOn_evening_weekend = 20 - local_hour_offset_UTC;
short h_turnsOff_evening_weekend = 24 - local_hour_offset_UTC;




unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


void setup() {

    //const char ssid[] = "TAI-OSPITI"; // inserire l'ssid della rete  
    //const char pass[] = "TAIw14pPI";  // password della rete  
    const char ssid[] = "Telecom-15855951"; // inserire l'ssid della rete  
    const char pass[] = "Casedipinte2010!"; // password della rete
    //char ssid[] = "iPhone di Giuseppe Graziano";  //  your network SSID (name)
    //char pass[] = "ciaociao";       // your network password

    Serial.begin(115200); // serial monitor
    delay(5);

    pinMode(pinLed, OUTPUT);
    digitalWrite(pinLed, status); 

    // Connessione alla rete WiFi  
    Serial.print("Connessione alla rete: ");
    Serial.println(ssid);

    /*  
    *  Viene impostata l'impostazione station (differente da AP o AP_STA) 
    * La modalità STA consente all'ESP8266 di connettersi a una rete Wi-Fi 
    * (ad esempio quella creata dal router wireless), mentre la modalità AP  
    * consente di creare una propria rete e di collegarsi 
    * ad altri dispositivi (ad esempio il telefono). 
    */
    WiFi.mode(WIFI_STA);

    // config static IP
    //IPAddress ip(192, 168, 1, 99);
    //IPAddress gateway(192, 168, 1, 1);  // set gateway to match your network
    //IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your
  
    //WiFi.config(ip, gateway, subnet); 
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {  
        delay(100);
        Serial.print(".");
    }

    Serial.print("WiFi connected. ");

    // Avvia il server
    server.begin();

    Serial.println("Starting UDP");
    udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(udp.localPort());

    //print ip
    Serial.print("Local IP: ");
    Serial.print(WiFi.localIP());
}

void loop() {
    //get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP);

    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);

    int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second

    if ( (epoch  % 86400L) / 3600 == h_turnsOn_morning || (epoch  % 86400L) / 3600 == h_turnsOn_evening ) {
        if ( 0 <= (epoch  % 3600) / 60 <= 5){
            if (status == 1) {
              status = 0;
              digitalWrite(pinLed, status);
            }  
        }
    } else if ( (epoch  % 86400L) / 3600 == h_turnsOff_morning  || (epoch  % 86400L) / 3600 == h_turnsOff_evening ) {
        if ( 25 <= (epoch  % 3600) / 60 <= 30){
            if (status == 0) {
              status = 1;
              digitalWrite(pinLed, status);
            }
        }
    }
  }
  // wait ten seconds before asking for the time again
  delay(10000);

    // Verifca se il client e' connesso  
    WiFiClient client = server.available();
    if (!client) {
        return;
    }

    // wait new connections
    Serial.println("New client");
    while (!client.available()) {  
        delay(25);
    }  

    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush(); 

    int apiIndex = request.indexOf(api);
    if (apiIndex != -1) {

        if (status == 1) {
            status = 0;
        } else {
            status = 1;
        }

        digitalWrite(pinLed, status);

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("");

        client.println("<!DOCTYPE HTML>");
        client.println("<html>");

        // titolo della pagina  
        client.println("<h2>Welocome to stranger things</h2>");

        client.print("<div style=\"font-size: 20px;\">");

        if (status == 1) {
            client.println("<h4>Luci spente</h4>");
        } else {
            client.println("<h4>Luci accese</h4>");
        }

        client.print("</div>");
        client.println("</html>");
    } else {
        client.println("<!DOCTYPE HTML>");
        client.println("<html>");
        client.println("HTTP/1.1 400 Not Found");
        client.println("</html>"); 
    }

    // chiusura connessione
    client.stop(); 
    Serial.println("Client disconnesso");
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
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
  udp.endPacket(); //////////////////////
}
