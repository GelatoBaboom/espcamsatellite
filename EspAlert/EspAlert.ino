
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include "Gsender.h"
#include <EEPROM.h>

#include "index.h"
#include "jsgzip.h"

#pragma region Globals
const char* ssid = "GelatoBaboom";               // WIFI network name
const char* password = "friofrio";               // WIFI network password
char* localIp = "";
IPAddress apIP(192, 168, 4, 1);
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
#pragma endregion Globals
#define LEDPIN D4

DNSServer dnsServer;
AsyncWebServer  server = AsyncWebServer(80);

String getConfigs(int addrStart, int addrCount)
{
  bool hasVal = false;
  char v;
  String val = "";
  for (int i = addrStart; i < addrStart + addrCount; i++ ) {

    v = (char)EEPROM.read(i);
    if (v == '|') {
      hasVal = true;
      break;
    }
    val += v;
  }
  return hasVal ? val : "";
}
void setConfigs(String val, int addrStart)
{
  const char* v = val.c_str();
  int addrPos = addrStart;
  for (int i = 0; i < val.length(); i++ ) {

    EEPROM.write(addrPos, v[i]);
    addrPos++;
  }
  EEPROM.write(addrPos, '|');
  if (EEPROM.commit()) {
    Serial.println("EEPROM successfully committed");
  } else {
    Serial.println("ERROR! EEPROM commit failed");
  }
}
void index_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_gz, index_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}
char* ip2CharArray(IPAddress ip) {
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}
void setup()
{
  EEPROM.begin(512);
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA );
  String wifissidStr = getConfigs(0, 20);
  String wifipassStr = getConfigs(20, 20);
  if (wifissidStr.length() > 0) {
    WiFi.begin(wifissidStr.c_str(), (wifipassStr.length() == 0 ? NULL : wifipassStr.c_str() ));
    uint8_t wtries = 0;
    while (WiFi.status() != WL_CONNECTED && wtries++ < 10) {//wait 10 seconds
      digitalWrite(LEDPIN, LOW);
      delay(500);
      digitalWrite(LEDPIN, HIGH);
      delay(500);
    }
    if (  WiFi.status() == WL_CONNECTED) {

      localIp = ip2CharArray(WiFi.localIP());

      //localIp =  WiFi.localIP().toString().c_str();
      Serial.print("Connected! IP address: ");
      Serial.println( WiFi.localIP().toString());
      Serial.println(localIp);
    } else
    {
      Serial.print("Not connected to LAN");
    }
  } else
  {
    WiFi.mode(WIFI_AP);
  }
   WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  //WiFi.softAP(apssid, appass);
  String ssidStr = "NivelAlerta AP";
  String appassStr = getConfigs(40,10);
  WiFi.softAP(ssidStr.c_str(), ((appassStr.length() == 0) ? NULL : appassStr.c_str() ));

  //dnsServer.start(53, "*", apIP);
  Serial.println("Get EEPROM 50-100");
  Serial.println(getConfigs(10, 50));
  Serial.println("Set EEPROM 50-100");
  setConfigs("idx001@gmail.com", 10);

  //Pages
  server.on("/", HTTP_GET, index_handler);
  server.onNotFound(index_handler);

  //  Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
  //  String subject = "Prueba envio 3";
  //  if (gsender->Subject(subject)->Send("idx001@gmail.com", "Setup test")) {
  //    Serial.println("Message send.");
  //  } else {
  //    Serial.print("Error sending message: ");
  //    Serial.println(gsender->getError());
  //  }
}

void loop() {}
