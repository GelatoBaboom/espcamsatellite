
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
String _serverResponce = "";
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
      Serial.println("SMTP Response TIMEOUT!");
      return false;
    }
  }
  while (client.available())
  {
    String sl = client.readStringUntil('\n');
    serverResponce += sl;
    _serverResponce += "s<-" + sl;
    Serial.println(sl);
  }

  if (resp && serverResponce.indexOf(resp) == -1) return false;
  return true;
}
void sendCommand(WiFiClient &client, String command , bool nolog = false) {
  if (!nolog)_serverResponce += "c->" + command + "\r\n";
  client.println(command);
}
void sendCommand(WiFiClient &client, const char* command , bool nolog = false) {
  if (!nolog)_serverResponce += "c->" + String(command) + "\r\n";
  client.println(command);
}
bool Send(const String &to, const String &subject,  const String &message)
{
  _serverResponce = "";
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

  //client.println("EHLO ESP8266_test");
  sendCommand(client, "EHLO ESP8266_MonoDev");
  if (!AwaitSMTPResponse(client, "250")) {
    Serial.println( "identification error");
    return false;
  }

  Serial.println("AUTH LOGIN:");

  //client.println("AUTH LOGIN");
  sendCommand(client, "AUTH LOGIN");
  AwaitSMTPResponse(client);

  Serial.println("EMAILBASE64_LOGIN:");
  Serial.println(EMAILBASE64_LOGIN);
  //client.println(base64::encode((EMAILBASE64_LOGIN)));
  sendCommand(client, base64::encode((EMAILBASE64_LOGIN)));
  AwaitSMTPResponse(client);


  Serial.println("EMAILBASE64_PASSWORD:");
  Serial.println(EMAILBASE64_PASSWORD);
  //client.println(base64::encode((EMAILBASE64_PASSWORD)));
  sendCommand(client, base64::encode((EMAILBASE64_PASSWORD)));
  if (!AwaitSMTPResponse(client, "235")) {
    Serial.println( "SMTP AUTH error");
    return false;
  }

  String mailFrom = "MAIL FROM: <" + String(FROM) + '>';

  Serial.println(mailFrom);
  //client.println(mailFrom);
  sendCommand(client, mailFrom);
  AwaitSMTPResponse(client);

  String rcpt = "RCPT TO: <" + to + '>';

  Serial.println(rcpt);

  //client.println(rcpt);
  sendCommand(client, rcpt);
  AwaitSMTPResponse(client);


  Serial.println("DATA:");
  //client.println("DATA");
  sendCommand(client, "DATA");
  if (!AwaitSMTPResponse(client, "354")) {
    Serial.println( "SMTP DATA error");
    return false;
  }

  String body = "<!DOCTYPE html><html lang=\"en\"><meta content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0\" name=\"viewport\"><style type=\"text/css\">body { margin: 0px; padding: 0px; background-color: #FFFFFF; }  a { color: #4c6e8b; } font-weight: normal;font-style: normal;}::selection {background: #ff2f2f; /* WebKit/Blink Browsers */}::-moz-selection {background: #ff2f2f; /* Gecko Browsers */}/* Responsive */@media only screen and (max-width:620px) {/* Img */img[class=\"reset\"] { display: inline!important; }img[class=\"scale\"] { width: 100%; }img[class=\"scale-inline\"] { display: inline!important; }}</style><div class=\"contenedores\"><table class=\"modulos ui-droppable\" id=\"\" style=\"background-color: rgb(255, 255, 255);\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FBFBFB\" align=\"center\"><tbody style=\"\"><tr><td><table class=\"scale-remove-border\" style=\"border-left: 1px solid #dcdcdc; border-right: 1px solid #dcdcdc\" width=\"620\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FFFFFF\" align=\"center\"><tbody><tr><td><a href=\"#\" class=\"f-link\"><img src=\"data:image/png;base64,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\" alt=\"\" class=\"scale editable\" style=\"display: block; margin: 0px;\" width=\"618\" border=\"0\"></a></td></tr></tbody></table></td></tr></tbody></table><table class=\"scale modulos ui-droppable\" id=\"\" style=\"\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FBFBFB\" align=\"center\"><tbody style=\"\"><tr><td><table class=\"scale-remove-border\" style=\"border-left: 1px solid #dcdcdc; border-right: 1px solid #dcdcdc\" width=\"620\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FFFFFF\" align=\"center\"><tbody><tr><td><table class=\"scale-90\" width=\"540\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" align=\"center\"><tbody><tr><td style=\"border-bottom: 1px solid #cccccc\">&nbsp;</td></tr></tbody></table></td></tr><tr><td style=\"font-size: 1px\" height=\"20\">&nbsp;</td></tr></tbody></table></td></tr></tbody></table><table class=\"scale modulos ui-droppable\" id=\"\" style=\"\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FBFBFB\" align=\"center\"><tbody style=\"\"><tr><td><table class=\"scale-remove-border\" style=\"border-left: 1px solid #dcdcdc; border-right: 1px solid #dcdcdc\" width=\"620\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FFFFFF\" align=\"center\"><tbody><tr><td style=\"font-size: 1px\" height=\"20\">&nbsp;</td></tr><tr><td align=\"center\"><table class=\"scale\" width=\"540\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" align=\"center\"><tbody><tr><td class=\"scale-center-both\"><div class=\"editable\" style=\"font-size: 24px; color: #c23e48; font-family: 'source_sans_probold', Helvetica, Arial, sans-serif; text-align: center\" id=\"editableEl_modulo_0_7_0\">Sistema de alerta de nivel</div></td></tr><tr><td style=\"font-size: 1px\" height=\"20\">&nbsp;</td></tr><tr><td style=\"color: #424c6d; font-size: 16px; font-family: 'source_sans_proregular', Helvetica, Arial, sans-serif; line-height: 28px;\" class=\"scale-center-both\" align=\"center\"><div>" + message + "</div></td></tr></tbody></table></td></tr></tbody></table></td></tr></tbody></table><table class=\"scale modulos ui-droppable\" id=\"\" style=\"\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FBFBFB\" align=\"center\"><tbody style=\"\"><tr><td><table class=\"scale-remove-border\" style=\"border-left: 1px solid #dcdcdc; border-right: 1px solid #dcdcdc\" width=\"620\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FFFFFF\" align=\"center\"><tbody><tr><td><table class=\"scale-90\" width=\"540\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" align=\"center\"><tbody><tr><td style=\"border-bottom: 1px solid #cccccc\">&nbsp;</td></tr></tbody></table></td></tr><tr><td style=\"font-size: 1px\" height=\"20\">&nbsp;</td></tr></tbody></table></td></tr></tbody></table><!-- 2_col --><!-- cierre tabla --><table class=\"scale\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FBFBFB\" align=\"center\"><tbody><tr><td><table class=\"scale-remove-border-keep-bottom\" style=\"border-left: 1px solid #dcdcdc; border-right: 1px solid #dcdcdc; border-bottom: 1px solid #dcdcdc\" width=\"620\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" bgcolor=\"#FFFFFF\" align=\"center\"><tbody><tr><td style=\"font-size: 1px\" height=\"10\">&nbsp;</td></tr></tbody></table> </td> </tr> <tr><td style=\"font-size: 1px\" height=\"20\">&nbsp;</td></tr></tbody></table></div></html>";
  sendCommand(client, "To: <" + to + '>', true);
  sendCommand(client, "Subject: " + subject, true);
  sendCommand(client, "Mime-Version: 1.0", true);
  sendCommand(client, "Content-Type: text/html; charset=\"UTF-8\"", true);
  sendCommand(client, "Content-Transfer-Encoding: 7bit", true);
  sendCommand(client, "", true);
  sendCommand(client, body, true);
  sendCommand(client, ".");
  //  client.println("From: <" + FROM + '>');
  //  client.println("To: <" + to + '>');
  //  client.print("Subject: ");
  //  client.println(subject);
  //  client.println("Mime-Version: 1.0");
  //  client.println("Content-Type: text/html; charset=\"UTF-8\"");
  //  client.println("Content-Transfer-Encoding: 7bit");
  //  client.println();
  //  client.println(body);
  //  client.println(".");
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
void getresponse_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse(200, "plain/text", _serverResponce );
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
  server.on("/api/getResponse", HTTP_GET, getresponse_handler);
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
