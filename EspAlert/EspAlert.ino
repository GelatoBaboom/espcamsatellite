
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
//#include "Gsender.h"
#include <EEPROM.h>
#include <base64.h>

#include "index.h"
#include "jsgzip.h"

#pragma region Globals
const char* ssid = "";      // WIFI network name
const char* password = "";  // WIFI network password
bool sendTest = false;
bool mailSendStatus = false;
uint32_t timerTempLoop = 30 * 1000;

char* localIp = "";
IPAddress apIP(192, 168, 4, 1);
uint16_t reconnect_interval = 10000; // If not connected wait time to try again
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
bool AwaitSMTPResponse(WiFiClient &client, const String &resp = "", uint16_t timeOut = 10000)
{
  String serverResponce = "";
  uint32_t ts = millis();
  serverResponce = "";
  while (!client.available())
  {
    if (millis() > (ts + timeOut)) {
      Serial.println( "SMTP Response TIMEOUT!");
      return false;
    }
  }
  while (client.available())
  {
    String sl = client.readStringUntil('\n');
    serverResponce += sl;
    Serial.println(sl);
  }

  if (resp && serverResponce.indexOf(resp) == -1) return false;
  return true;
}
bool Send(const String &to, const String &subject,  const String &message)
{
  WiFiClient client;
  //client.setFingerprint(fingerprint);
  //client.setInsecure();
  String SMTP_SERVER = getConfigs(40, 50);
  int SMTP_PORT = getConfigs(190, 5).toInt();
  String EMAILBASE64_LOGIN = getConfigs(90, 50);
  String EMAILBASE64_PASSWORD = getConfigs(140, 50);
  String FROM = getConfigs(250, 50);
  Serial.print("Connecting to :");
  Serial.println(SMTP_SERVER);

  if (!client.connect(SMTP_SERVER, SMTP_PORT)) {
    Serial.println("Could not connect to mail server");
    return false;
  }
  if (!AwaitSMTPResponse(client, "220")) {
    Serial.println( "Connection Error");
    return false;
  }


  Serial.println("HELO friend:");

  client.println("EHLO ESP8266_test");
  if (!AwaitSMTPResponse(client, "250")) {
    Serial.println( "identification error");
    return false;
  }

  Serial.println("AUTH LOGIN:");

  client.println("AUTH LOGIN");
  AwaitSMTPResponse(client);

  Serial.println("EMAILBASE64_LOGIN:");
  Serial.println(EMAILBASE64_LOGIN);
  client.println(base64::encode((EMAILBASE64_LOGIN)));
  AwaitSMTPResponse(client);


  Serial.println("EMAILBASE64_PASSWORD:");
  Serial.println(EMAILBASE64_PASSWORD);
  client.println(base64::encode((EMAILBASE64_PASSWORD)));
  if (!AwaitSMTPResponse(client, "235")) {
    Serial.println( "SMTP AUTH error");
    return false;
  }

  String mailFrom = "MAIL FROM: <" + String(FROM) + '>';

  Serial.println(mailFrom);
  client.println(mailFrom);
  AwaitSMTPResponse(client);

  String rcpt = "RCPT TO: <" + to + '>';

  Serial.println(rcpt);

  client.println(rcpt);
  AwaitSMTPResponse(client);


  Serial.println("DATA:");
  client.println("DATA");
  if (!AwaitSMTPResponse(client, "354")) {
    Serial.println( "SMTP DATA error");
    return false;
  }

  client.println("From: <" + FROM + '>');
  client.println("To: <" + to + '>');

  client.print("Subject: ");
  client.println(subject);

  client.println("Mime-Version: 1.0");
  client.println("Content-Type: text/html; charset=\"UTF-8\"");
  client.println("Content-Transfer-Encoding: 7bit");
  client.println();
  String body = "<!DOCTYPE html><html lang=\"en\">" + message + "</html>";
  client.println(body);
  client.println(".");
  if (!AwaitSMTPResponse(client, "250")) {
    Serial.println("Sending message error");
    return false;
  }
  client.println("QUIT");
  if (!AwaitSMTPResponse(client, "221")) {
    Serial.println( "SMTP QUIT error");
    return false;
  }
  return true;
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
  json_response += "\"},";

  json_response += "{\"key\":\"smtpfrom\",\"value\":\"";
  json_response += getConfigs(250, 50);
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
  if (k == "smtpfrom") {
    setConfigs( v, 250 );
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
void sendtest_handler(AsyncWebServerRequest * request) {
  Serial.println("Sending test");
  sendTest = true;
//  while (sendTest) {
//    delay(1000);
//  }
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"result\":" + String(mailSendStatus ? "true" : "false") + "}");
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
//smtp

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
  server.on("/api/sendtest", HTTP_GET, sendtest_handler);
  server.begin();

  Send(getConfigs(195, 55), "Prueba envio 4",  "Body Alerta");
  //serverSmtp = getConfigs(40, 50).c_str();
  //Gsender *gsender = new Gsender(getConfigs(40, 50).c_str(), getConfigs(90, 50).c_str(), getConfigs(140, 50).c_str(), getConfigs(190, 5).toInt() ); // Getting pointer to class instance
  //Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
  //gsender->setConfig(getConfigs(40, 50).c_str(), getConfigs(90, 50).c_str(), getConfigs(140, 50).c_str(), getConfigs(190, 5).toInt() );

  //gsender->setConfig("smtp-relay.sendinblue.com", "ignacio@ineva.com.ar", "hBtECj7GSf9InyT2", 587 );
  //String subject = "Prueba envio 3";
  //  if (gsender->Subject(subject)->Send(getConfigs(195, 55), "Body Alerta")) {
  //    Serial.println("Message send.");
  //  } else {
  //    Serial.print("Error sending message: ");
  //    Serial.println(gsender->getError());
  //  }
}

void loop() {
  dnsServer.processNextRequest();
  if (((millis() - timerTempLoop) / 1000) > (5))
  {
    if (sendTest)
    {
      mailSendStatus =  Send(getConfigs(195, 55), "Test SMTP",  "Body Alerta");
      sendTest = false;
    }
  }

}
