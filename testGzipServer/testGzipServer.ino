
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#include <ESP8266WebServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
//#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include "DallasTemperature.h"
#include "OneWire.h"
#include <SPI.h>
#include <SD.h>
#include "index.h"
#include "jsgzip.h"
IPAddress apIP(192, 168, 4, 1);
#define DBG_OUTPUT_PORT Serial
#define CS_PIN  D8
#define ONE_WIRE_BUS_PIN  D1

const char* ssid = "GelatoBaboom";
const char* password = "friofrio";
const char* host = "esp8266sd";
uint32_t timerLoop;
DNSServer dnsServer;
File fi;

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature DS18B20(&oneWire);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

AsyncWebServer  server = AsyncWebServer(80);
float getTemperature(uint8_t idx) {
  //Serial.println("index: " + String(idx));
  int cts = 10;
  float temp;
  do {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(idx);
    delay(10);
    cts--;
  } while ((temp == 85.0 || temp == (-127.0)) && cts > 1);

  return temp;
}
void temp_handler(AsyncWebServerRequest *request) {
  static char json_response[1024];

  char * p = json_response;
  *p++ = '{';
  p += sprintf(p, "\"temp\":\"%.1f\"", getTemperature(0));
  *p++ = '}';
  *p++ = 0;

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json_response);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
void temph_handler(AsyncWebServerRequest *request) {
  String json_response;
  fi = SD.open("VALUES.TXT");
  if (fi) {
    json_response = "{\"values\":[";
    while (fi.available()) {
      String v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      json_response += v ;
      if (fi.available())json_response += ",";
    }
    fi.close();
    json_response += "]";
  }
  fi = SD.open("LABELS.TXT");
  if (fi) {
    json_response += ",\"labels\":[";
    while (fi.available()) {
      String v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      json_response += "\"" + v + "\"" ;
      if (fi.available())json_response += ",";
    }
    fi.close();
    json_response += "]}";
  }

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json_response);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void index_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_gz, index_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void chartjs_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", chartjs_gz, chartjs_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void renderjs_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", renderjs_gz, renderjs_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void utilsjs_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", utils_gz, utils_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void jquerymin_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", jquerymin_gz, jquerymin_len);
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

void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  //WiFi.mode(WIFI_STA);
  //WiFi.begin(ssid, password);

  //  WiFi.mode(WIFI_AP);
  //  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  //  WiFi.softAP("EspServer", NULL);

  WiFi.mode(WIFI_AP_STA );
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("EspServer", NULL);
  WiFi.begin(ssid, password);

  dnsServer.start(53, "*", apIP);

  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);
  if (!SD.begin(10)) {
    DBG_OUTPUT_PORT.println("initialization failed!");
  }
  timeClient.begin();
  timeClient.setTimeOffset(-10800);

  // Wait for connection
  uint8_t i = 0;
  //  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
  //    delay(500);
  //  }
  //  if (i == 21) {
  //    DBG_OUTPUT_PORT.print("Could not connect to");
  //    DBG_OUTPUT_PORT.println(ssid);
  //    while (1) delay(500);
  //  }
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  //  if (MDNS.begin(host)) {
  //    MDNS.addService("http", "tcp", 80);
  //    DBG_OUTPUT_PORT.println("MDNS responder started");
  //    DBG_OUTPUT_PORT.print("You can now connect to http://");
  //    DBG_OUTPUT_PORT.print(host);
  //    DBG_OUTPUT_PORT.println(".local");
  //  }


  server.on("/", HTTP_GET, index_handler);
  server.on("/temp", HTTP_GET, temp_handler);
  server.on("/getTempJson", HTTP_GET, temph_handler);
  server.on("/bootstrap.bundle.min.js", HTTP_GET, bootstrapjs_handler);
  server.on("/bootstrap.css", HTTP_GET, bootstrapcss_handler);
  server.on("/jquery-3.js", HTTP_GET, jquerymin_handler);
  server.on("/chart.min.js", HTTP_GET, chartjs_handler);
  server.on("/render.js", HTTP_GET, renderjs_handler);
  server.on("/utils.js", HTTP_GET, utilsjs_handler);
  server.onNotFound(index_handler);

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

}

void loop(void) {
  dnsServer.processNextRequest();
  //aca graba la temp en una funcioncon timer
  if (((micros() - timerLoop) / 1000000) > 60)//(5 * 60))
  {
    fi = SD.open("VALUES.TXT", FILE_WRITE);
    if (fi) {
      float temperature1 = getTemperature(0);
      fi.println( String(temperature1) );
      fi.close();
    }
    fi = SD.open("LABELS.TXT", FILE_WRITE);
    if (fi) {
      timeClient.update();
      DBG_OUTPUT_PORT.println(timeClient.getFormattedTime());
      fi.println(timeClient.getFormattedTime());
      fi.close();
    }
    timerLoop = micros();
  }
}
