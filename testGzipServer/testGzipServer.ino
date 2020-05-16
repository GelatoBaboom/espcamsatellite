
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#include "index.h"

#define DBG_OUTPUT_PORT Serial

const char* ssid = "GelatoBaboom";
const char* password = "friofrio";
const char* host = "esp8266sd";

AsyncWebServer  server = AsyncWebServer(80);

void temp_handler(AsyncWebServerRequest *request) {
  static char json_response[1024];

  char * p = json_response;
  *p++ = '{';
  p += sprintf(p, "\"temp\":\"%.1f\"", 87);
  *p++ = '}';
  *p++ = 0;

  AsyncWebServerResponse *response = request->beginResponse(200,"application/json", json_response);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}


void index_handler(AsyncWebServerRequest *request) {
  // Dump the byte array in PROGMEM with a 200 HTTP code (OK)
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_gz, index_len);

  // Tell the browswer the contemnt is Gzipped
  response->addHeader("Content-Encoding", "gzip");
  // And set the last-modified datetime so we can check if we need to send it again next time or not
  //response->addHeader("Last-Modified", last_modified);
  request->send(response);
}


void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);

  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
    delay(500);
  }
  if (i == 21) {
    DBG_OUTPUT_PORT.print("Could not connect to");
    DBG_OUTPUT_PORT.println(ssid);
    while (1) delay(500);
  }
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local");
  }


  server.on("/", HTTP_GET, index_handler);
  server.on("/temp", HTTP_GET, temp_handler);
  //server.onNotFound(handleNotFound);

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

}

void loop(void) {

}
