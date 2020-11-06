
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
String getConfigsToJSON()
{
  String json_response = "[";

  json_response += "{\"key\":\"appass\",\"value\":\"";
  json_response += getConfigs(40, 10);
  json_response += "\"},";

  json_response += "{\"key\":\"wifissid\",\"value\":\"";
  json_response += getConfigs(0, 20);
  json_response += "\"},";

  json_response += "{\"key\":\"wifipass\",\"value\":\"";
  json_response += getConfigs(20, 20);
  json_response += "\"},";

  json_response += "{\"key\":\"smtpserver\",\"value\":\"";
  json_response += getConfigs(40, 50);
  json_response += "\"},";

  json_response += "{\"key\":\"smtpuser\",\"value\":\"";
  json_response += getConfigs(90, 50);
  json_response += "\"},";

  json_response += "{\"key\":\"smtppass\",\"value\":\"";
  json_response += getConfigs(140, 50);
  json_response += "\"},";

  json_response += "{\"key\":\"smtpport\",\"value\":\"";
  json_response += getConfigs(190, 5);
  json_response += "\"},";

  json_response += "{\"key\":\"smtpemailalerta\",\"value\":\"";
  json_response += getConfigs(195, 55);
  json_response += "\"}";

  json_response += "]";
  return json_response;
}
void index_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_gz, index_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void bootstrapjs_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", bootstrapjs_gz, bootstrapjs_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}
void bootstrapcss_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", bootstrapcss_gz, bootstrapcss_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}
void jquerymin_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", jquerymin_gz, jquerymin_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}
void configsjs_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", configsjs_gz, configsjs_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}
void configsjson_handler(AsyncWebServerRequest *request) {
  String json_response = getConfigsToJSON();
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json_response);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void setconfig_handler(AsyncWebServerRequest *request) {
  String k = "";
  String v = "";
  int params = request->params();
  for (int i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if ((p->name()) == "k") {
      k = (p->value());
    }
    if ((p->name()) == "v") {
      v = (p->value());
    }
  }
  Serial.println("Set Config, " + k + ", " + v );
  if (k == "appass") {
    setConfigs( v, 40 );
  }
  if (k == "wifissid") {
    setConfigs( v, 0 );
  }
  if (k == "wifipass") {
    setConfigs( v, 20 );
  }
  if (k == "smtpserver") {
    setConfigs( v, 40 );
  }
  if (k == "smtpuser") {
    setConfigs( v, 90 );
  }
  if (k == "smtppass") {
    setConfigs( v, 140 );
  }
  if (k == "smtpport") {
    setConfigs( v, 190 );
  }
  if (k == "smtpemailalerta") {
    setConfigs( v, 195 );
  }



  bool done = true;

  AsyncWebServerResponse * response = request->beginResponse(200, "application/json", "{\"result\":" + String(done ? "true" : "false") + "}");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
void reboot_handler(AsyncWebServerRequest * request) {
  Serial.println("Restart request");
  //terminar esto
  //rebootRequest = true;
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"result\":true}");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
void logosvg_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg+xml", logosvg_gz, logosvg_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}
void monodevsvg_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "image/svg+xml", monodevsvg_gz, monodevsvg_len);
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
  String wifissidStr = "GelatoBaboom";//getConfigs(0, 20);
  String wifipassStr = "friofrio";//getConfigs(20, 20);
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

  String ssidStr = "AlertaNivel AP";
  String appassStr = getConfigs(40, 10);
  WiFi.softAP(ssidStr.c_str(), ((appassStr.length() == 0) ? NULL : appassStr.c_str() ));

  dnsServer.start(53, "*", apIP);
  //  Serial.println("Get EEPROM 50-100");
  //  Serial.println(getConfigs(10, 50));
  //  Serial.println("Set EEPROM 50-100");
  //  setConfigs("idx001@gmail.com", 10);

  //Pages
  server.on("/", HTTP_GET, index_handler);
  server.onNotFound(index_handler);
  //statics
  server.on("/bootstrap.bundle.min.js", HTTP_GET, bootstrapjs_handler);
  server.on("/bootstrap.css", HTTP_GET, bootstrapcss_handler);
  server.on("/jquery-3.js", HTTP_GET, jquerymin_handler);
  server.on("/configs.js", HTTP_GET, configsjs_handler);
  server.on("/logo.svg", HTTP_GET, logosvg_handler);
  server.on("/monodev.svg", HTTP_GET, monodevsvg_handler);
  //res
  server.on("/api/getConfigsJson", HTTP_GET, configsjson_handler);
  server.on("/api/setConfig", HTTP_GET, setconfig_handler);
  server.on("/api/reboot", HTTP_GET, reboot_handler);
  server.begin();
  //  Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
  //  String subject = "Prueba envio 3";
  //  if (gsender->Subject(subject)->Send("idx001@gmail.com", "Setup test")) {
  //    Serial.println("Message send.");
  //  } else {
  //    Serial.print("Error sending message: ");
  //    Serial.println(gsender->getError());
  //  }
}

void loop() {
  dnsServer.processNextRequest();
}
