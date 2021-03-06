#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
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
#define LEDPIN 2

const char* ssid = "GelatoBaboom";
const char* password = "friofrio";
const char* host = "esp8266sd";
uint32_t timerLoop;
bool justWakedUp = true;
int regTime = 60 * 5;

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
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int currentMonth = ptm->tm_mon + 1;
  int currentDay = ptm->tm_mday;

  int currentHour = ptm->tm_hour;
  int currentMinute = ptm->tm_min;

  static char json_response[1024];

  char * p = json_response;
  *p++ = '{';
  p += sprintf(p, "\"temp\":\"%.1f\",", getTemperature(0));
  p += sprintf(p, "\"month\":%i,",  currentMonth);
  p += sprintf(p, "\"day\":%i,", currentDay);
  p += sprintf(p, "\"hour\":%i,", currentHour);
  p += sprintf(p, "\"minute\":%i", currentMinute);
  *p++ = '}';
  *p++ = 0;

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json_response);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
void getDirectory(File dir, int level, String *json) {
  bool newEntry = true;
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      break;
    }
    if (!newEntry) {
      *json += ",";
    } else {
      newEntry = false;
    }
    if (entry.isDirectory()) {
      if (level == 1) {
        *json += "{\"year\":";
        *json +=  entry.name();
        *json += ", \"months\":[";
        getDirectory(entry, 2, json);
        *json += "]}";
      }
      if (level == 2) {
        *json += "{\"month\":";
        *json +=  entry.name();
        *json += ", \"days\":[";
        getDirectory(entry, 3, json);
        *json += "]}";
      } if (level == 3) {
        *json += "\"";
        *json += entry.name();
        *json += "\"";
      }
    }
    entry.close();
  }
}
void getRegisters_handler(AsyncWebServerRequest * request) {
  String json_response;
  File root = SD.open("/regs/");
  json_response = "{\"registers\":[";
  getDirectory(root, 1, &json_response);
  json_response += "]}";


  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json_response);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
String getDaily(int currentYear, int currentMonth, int currentDay, int currentResolution )
{
  int res = 0;
  float minT = 500;
  float maxT = -127;
  float current = 0;
  String json_response;
  //DBG_OUTPUT_PORT.println("year: " + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay));
  fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/VALUES.TXT");
  if (fi) {
    json_response = "{\"values\":[";
    float promV = 0;
    while (fi.available()) {
      String v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      current = v.toFloat();
      promV = promV + current ;
      minT = current < minT ? current : minT;
      maxT = current > maxT ? current : maxT;
      res++;
      if (res >= currentResolution) {
        json_response += String(promV / res) ;
        res = 0;
        promV = 0;
        if (fi.available())json_response += ",";
      }
    }
    if (res > 0)
    {
      json_response += String(promV / res) ;
    }
    fi.close();
    json_response += "]";
  }
  res = 0;
  fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/LABELS.TXT");
  if (fi) {
    json_response += ",\"labels\":[";
    String v = "";
    while (fi.available()) {
      v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      res++;
      if (res >= currentResolution) {
        json_response += "\"" + v + "\"" ;
        res = 0;
        if (fi.available())json_response += ",";
      }
    }
    if (res > 0)
    {
      json_response += "\"" + v + "\"" ;
    }
    fi.close();
    json_response += "],";

  }
  json_response += "\"stats\":{";
  json_response += "\"max\":" + String(maxT);
  json_response += ",\"min\":" + String(minT);
  json_response += "}}";
  return json_response;
}
String getMonthly(int currentYear, int currentMonth, int currentResolution )
{
  int res = 0;
  String json_response;
  String monthsLabels ;
  //DBG_OUTPUT_PORT.println("year: " + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay));
  File dir = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) );
  json_response = "{\"values\":[";
  bool firstEntry = true;
  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      break;
    }
    if (!firstEntry) {
      json_response += ",";
      monthsLabels += + ",";

    }
    firstEntry = false;
    res = 0;
    File fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + entry.name() + "/VALUES.TXT");
    if (fi) {
      float promV = 0;
      while (fi.available()) {
        String v = fi.readStringUntil('\r');
        fi.readStringUntil('\n');
        promV = promV + v.toFloat();
        res++;
      }
      json_response += String(promV / res) ;

      monthsLabels += "\"" ;
      monthsLabels += + entry.name();
      monthsLabels += + "\"";
    }
    fi.close();

    entry.close();
  }
  json_response += "]";
  json_response += ",\"labels\":[";
  json_response += monthsLabels;
  json_response += "]}";
  return json_response;
}
void temph_handler(AsyncWebServerRequest *request) {
  int currentYear = 0;
  int currentMonth = 0;
  int currentDay = 0;
  int currentResolution = 0;


  int params = request->params();
  DBG_OUTPUT_PORT.println("Param count: " + String(params));
  for (int i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if ((p->name()) == "y") {
      currentYear = (p->value()).toInt();
    }
    if ((p->name()) == "m") {
      currentMonth = (p->value()).toInt();
    }
    if ((p->name()) == "d") {
      currentDay = (p->value()).toInt();
    }
    if ((p->name()) == "s") {
      currentResolution = (p->value()).toInt();
    }
  }
  //that block has to be replaced whit month and day from params
  String json_response;
  if (currentDay != 0) {
    json_response = getDaily(currentYear, currentMonth, currentDay, currentResolution);
  } else {
    json_response = getMonthly(currentYear, currentMonth, currentResolution);
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
void registerData()
{

  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int currentMonth = ptm->tm_mon + 1;
  int currentDay = ptm->tm_mday;
  int currentYear = ptm->tm_year + 1900;
  if (currentYear > 1969) {
    if (!SD.exists("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay)))
    {
      SD.mkdir("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay));
    }
    fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/VALUES.TXT", FILE_WRITE);
    if (fi) {
      float temperature1 = getTemperature(0);
      fi.println( String(temperature1) );
      fi.close();
    }
    fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/LABELS.TXT", FILE_WRITE);
    if (fi) {
      timeClient.update();
      fi.println(timeClient.getFormattedTime());
      fi.close();
    }
    timerLoop = micros();
  }


}
void checkSleepMode()
{
  DBG_OUTPUT_PORT.println("checking config" );
  fi = SD.open("/configs/sleepmode.txt");
  if (fi) {
    DBG_OUTPUT_PORT.println("config file found" );
    int topH = 0;
    int bottomH = 0;
    if (fi.available()) {
      DBG_OUTPUT_PORT.println("config file available" );
      String v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      topH = v.toInt();
      v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      bottomH = v.toInt();
      timeClient.update();
      unsigned long epochTime = timeClient.getEpochTime();
      struct tm *ptm = gmtime ((time_t *)&epochTime);
      int currentHour = ptm->tm_hour;
      int currentMinute = ptm->tm_min;
      DBG_OUTPUT_PORT.println("current H " + String(currentHour) + " TopH: " + String(topH) + " BottomH: " + String(bottomH) );
      if ((topH < bottomH && currentHour >= topH && currentHour < bottomH) || (topH > bottomH && (currentHour >= topH || currentHour < bottomH)))
      {
        for (int i = 0 ; i < 10; i++)
        {
          digitalWrite(LEDPIN, LOW);
          delay(100);
          digitalWrite(LEDPIN, HIGH);
          delay(100);

        }
        if (justWakedUp) {
          registerData();
        }
        DBG_OUTPUT_PORT.println("imma gonna sleep");
        ESP.deepSleep(1000000 * (regTime));
      }
    }
  }
}
void setup(void) {
  pinMode(LEDPIN, OUTPUT);
  //digitalWrite(LEDPIN, HIGH);
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  //WiFi.mode(WIFI_STA);
  //WiFi.begin(ssid, password);

  //  WiFi.mode(WIFI_AP);
  //  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  //  WiFi.softAP("EspServer", NULL);

  WiFi.mode(WIFI_AP_STA );
  WiFi.begin(ssid, password);
  //quizas aca chequear....
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);
  uint8_t wtries = 0;
  while (WiFi.status() != WL_CONNECTED && wtries++ < 10) {//wait 10 seconds
    digitalWrite(LEDPIN, LOW);
    delay(500);
    digitalWrite(LEDPIN, HIGH);
    delay(500);
  }
  if (!SD.begin(10)) {
    DBG_OUTPUT_PORT.println("initialization failed!");
  }
  timeClient.begin();
  timeClient.setTimeOffset(-10800);
  timeClient.update();
  checkSleepMode();
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("EspServer", NULL);


  dnsServer.start(53, "*", apIP);


  // Wait for connection
  //uint8_t i = 0;
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
  server.on("/getRegs", HTTP_GET, getRegisters_handler);
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
  justWakedUp = false;
  dnsServer.processNextRequest();
  if (((micros() - timerLoop) / 1000000) > (regTime))
  {
    registerData();
    checkSleepMode();
  }
}
