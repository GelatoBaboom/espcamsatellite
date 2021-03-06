#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RTClib.h>
#include <DHTesp.h>

#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <DNSServer.h>

#include <SPI.h>
#include <SD.h>
#include "index.h"
#include "jsgzip.h"
IPAddress apIP(192, 168, 4, 1);
#define DBG_OUTPUT_PORT Serial
#define CS_PIN  10
#define TRELAY_PIN  D0
#define HRELAY_PIN  3

#define LEDPIN D4
#define DHTTYPE DHT22

uint8_t DHTPin = D3;

char* wifissid = "";
char* wifipass = "";
const char*  apssid = "FungoServer";
const char* appass = NULL;
const char* host = "fungoserver";
char* localIp = "";
uint32_t timerLedLoop;
uint32_t timerLoop;
uint32_t timerTempLoop = 30 * 1000;

int regTime = 60 * 5;
String filecont = "";
bool configHasChanges = false;
bool regEnable = true;
bool rebootRequest = false;
bool ledStatus = false;

bool humInited = false;
bool calInited = false;


DHTesp dht;
float currentTemp = -127;
short tAdj = 0;
float currentHum = 0;
short hAdj = 0;
int bottomThresholdT = 0;
int topThresholdT = 0;
int bottomThresholdH = 0;
int topThresholdH = 0;

DNSServer dnsServer;
File fi;

WiFiUDP ntpUDP;

RTC_DS3231 rtc;

AsyncWebServer  server = AsyncWebServer(80);
float getDHTTemperature() {
  //Serial.println("index: " + String(idx));
  int cts = 10;
  float temp;
  float hum;
  do {
    temp = dht.getTemperature();
    delay(300);
    cts--;
  } while (isnan(temp) && cts > 1);
  if (!isnan(temp)) {
    temp = temp + tAdj;
    currentTemp = temp;
  }
  return temp;
}
float getDHTHumidity() {
  int cts = 10;
  float hum;
  do {
    hum = dht.getHumidity();
    delay(300);
    cts--;
  } while (isnan(hum) && cts > 1);
  if (!isnan(hum)) {
    hum = hum + hAdj;
    currentHum = hum;
  }
  return hum;
}
String getConfigsToJSON()
{
  String json_response = "[";
  fi = SD.open("/configs/configs.ini");
  if (fi) {
    while (fi.available()) {
      String v = fi.readStringUntil(',');
      json_response += "{\"key\":\"";
      json_response += v;
      json_response += "\",\"value\":\"";
      v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      json_response += v;
      json_response += "\"}";
      if (fi.available())json_response += ",";

    }
    json_response += "]";
  }
  fi.close();
  return json_response;
}
void configsjson_handler(AsyncWebServerRequest *request) {
  String json_response = getConfigsToJSON();
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json_response);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
void rtcconfigsjson_handler(AsyncWebServerRequest *request) {
  DateTime now = rtc.now();
  String json_response = "[";

  json_response += "{\"key\":\"inpdate\",\"value\":\"" + String(now.year()) + "-" +  String(now.month() < 10 ? "0" : "") + String(now.month()) + "-" +  String(now.day() < 10 ? "0" : "") + String(now.day())  +   "\"},";
  json_response += "{\"key\":\"inphour\",\"value\":\"" + String(now.hour() < 10 ? "0" : "") + String(now.hour()) + "\"},";
  json_response += "{\"key\":\"inpminute\",\"value\":\"" + String(now.minute() < 10 ? "0" : "") + String(now.minute()) + "\"}";
  json_response += "]";

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json_response);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
bool setConfigs(String key, String val)
{
  if (!configHasChanges) {
    filecont = "";
    DBG_OUTPUT_PORT.println("open file");
    fi = SD.open("/configs/configs.ini");
    String v = "";
    if (fi) {
      while (fi.available()) {
        v = fi.readStringUntil(',');
        filecont += v ;
        filecont += ",";
        if (v == key)
        {
          DBG_OUTPUT_PORT.println("key found");
          DBG_OUTPUT_PORT.println("value: " + val);
          filecont += val;
          filecont += "\r\n";
          fi.readStringUntil('\r');
          fi.readStringUntil('\n');
          DBG_OUTPUT_PORT.println("val put");
        } else {
          v = fi.readStringUntil('\r');
          fi.readStringUntil('\n');
          filecont += v;
          filecont += "\r\n";
        }
      }
      fi.close();
      DBG_OUTPUT_PORT.println("file closed");
      configHasChanges = true;
    }
    return true;
  }
  return false;

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
  bool done = setConfigs(k, v);

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"result\":" + String(done ? "true" : "false") + "}");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
void setrtcconfig_handler(AsyncWebServerRequest *request) {
  String k = "";
  String v = "";
  int params = request->params();
  DateTime now = rtc.now();
  for (int i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if ((p->name()) == "k") {
      k = (p->value());
    }
    if ((p->name()) == "v") {
      v = (p->value());
    }
  }
  DBG_OUTPUT_PORT.println("k: " + k + " v: " + v);
  if (k == "inpdate")
  {
    rtc.adjust(DateTime(v.substring(0, 4).toInt(), v.substring(5, 7).toInt(), v.substring(8).toInt(), now.hour(), now.minute(), now.second()));
  }
  if (k == "inphour")
  {
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), v.toInt(), now.minute() ));
  }
  if (k == "inpminute")
  {
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), v.toInt() ));
  }

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"result\":true}");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void temp_handler(AsyncWebServerRequest *request) {
  DateTime now = rtc.now();

  int currentMonth = now.month();
  int currentDay = now.day();

  int currentHour = now.hour();
  int currentMinute = now.minute();

  static char json_response[1024];

  char * p = json_response;
  *p++ = '{';
  p += sprintf(p, "\"temp\":\"%.1f\",", currentTemp);
  p += sprintf(p, "\"hum\":\"%.1f\",", currentHum);
  p += sprintf(p, "\"ip\":\"%s\",", localIp);
  p += sprintf(p, "\"devtemp\":\"%.1f\",", rtc.getTemperature());
  p += sprintf(p, "\"rtclostpower\":%s,", (rtc.lostPower() ? "true" : "false") );
  p += sprintf(p, "\"regenable\":%s,", (regEnable ? "true" : "false") );
  p += sprintf(p, "\"huminited\":%s,", (humInited ? "true" : "false") );
  p += sprintf(p, "\"calinited\":%s,", (calInited ? "true" : "false") );
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
void reboot_handler(AsyncWebServerRequest * request) {
  DBG_OUTPUT_PORT.println("Restart request");
  rebootRequest = true;
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"result\":true}");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
void cmdreg_handler(AsyncWebServerRequest * request) {
  regEnable = !regEnable;
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"result\":true}");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
void cmdregdel_handler(AsyncWebServerRequest * request) {
  int currentYear = 0;
  int currentMonth = 0;
  int currentDay = 0;
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
  }
  bool removed = false;
  if (currentDay != 0) {
    SD.remove("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/LABELS.TXT");
    SD.remove("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/TVALUES.TXT");
    SD.remove("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/HVALUES.TXT");
    bool removed = SD.rmdir("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay));
  }
  String json = "{\"result\":" + String(removed ? "true" : "false") + "}";
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

String getDaily(int currentYear, int currentMonth, int currentDay, int currentResolution )
{
  int res = 0;
  float minT = 500;
  float maxT = -127;
  float minH = 101;
  float maxH = -1;
  float current = 0;
  String json_response;
  //DBG_OUTPUT_PORT.println("year: " + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay));
  //temperatura
  fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/TVALUES.TXT");
  if (fi) {
    json_response = "{\"tvalues\":[";
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
    json_response += "],";
  }
  //humedad
  res = 0;
  current = 0;
  fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/HVALUES.TXT");
  if (fi) {
    json_response += "\"hvalues\":[";
    float promV = 0;
    while (fi.available()) {
      String v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      current = v.toFloat();
      promV = promV + current ;
      minH = current < minH ? current : minH;
      maxH = current > maxH ? current : maxH;
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
    json_response += "],";
  }
  //Labels
  res = 0;
  fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/LABELS.TXT");
  if (fi) {
    json_response += "\"labels\":[";
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
  json_response += "\"maxt\":\"" + String(maxT) + "\",";
  json_response += "\"mint\":\"" + String(minT) + "\",";
  json_response += "\"maxh\":\"" + String(maxH) + "\",";
  json_response += "\"minh\":\"" + String(minH) + "\"";
  json_response += "}}";
  return json_response;
}
String getMonthly(int currentYear, int currentMonth, int currentResolution )
{
  int res = 0;
  String tjson_response;
  String hjson_response;
  String monthsLabels ;
  //DBG_OUTPUT_PORT.println("year: " + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay));
  File dir = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) );
  tjson_response = "\"tvalues\":[";
  hjson_response = "\"hvalues\":[";
  bool firstEntry = true;
  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      break;
    }
    if (!firstEntry) {
      tjson_response += ",";
      hjson_response += ",";
      monthsLabels += + ",";

    }
    firstEntry = false;
    res = 0;
    //temperatura
    fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + entry.name() + "/TVALUES.TXT");
    if (fi) {
      float promV = 0;
      while (fi.available()) {
        String v = fi.readStringUntil('\r');
        fi.readStringUntil('\n');
        promV = promV + v.toFloat();
        res++;
      }
      tjson_response += String(promV / res) ;
    }
    fi.close();
    //humedad
    fi = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + entry.name() + "/HVALUES.TXT");
    if (fi) {
      float promV = 0;
      while (fi.available()) {
        String v = fi.readStringUntil('\r');
        fi.readStringUntil('\n');
        promV = promV + v.toFloat();
        res++;
      }
      hjson_response += String(promV / res) ;


    }
    fi.close();
    monthsLabels += "\"" ;
    monthsLabels += + entry.name();
    monthsLabels += + "\"";
    entry.close();
  }
  tjson_response += "]";
  hjson_response += "]";
  monthsLabels = "\"labels\":[" + monthsLabels + "]";

  return "{" + tjson_response + "," + hjson_response + "," + monthsLabels + "}" ;
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
void  configpage_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", configpage_gz, configpage_len);
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
void configsjs_handler(AsyncWebServerRequest * request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", configsjs_gz, configsjs_len);
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
void registerData()
{
  if (regEnable) {
    File fiReg;
    DateTime now = rtc.now();
    int currentMonth = now.month();
    int currentDay = now.day();
    int currentYear = now.year();

    if (!SD.exists("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay)))
    {
      SD.mkdir("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay));
    }
    float temperature = getDHTTemperature();
    float humidity = getDHTHumidity();
    if (!isnan(temperature) && !isnan(humidity)) {
      fiReg = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/TVALUES.TXT", FILE_WRITE);
      if (fiReg) {
        fiReg.println( String(temperature) );
        fiReg.close();
      }
      fiReg = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/HVALUES.TXT", FILE_WRITE);
      if (fiReg) {
        fiReg.println( String(humidity) );
        fiReg.close();
      }
      fiReg = SD.open("/regs/" + String(currentYear) + "/" + String(currentMonth) + "/" + String(currentDay) + "/LABELS.TXT", FILE_WRITE);
      if (fiReg) {
        int  hh = now.hour();
        int  mm = now.minute();
        fiReg.println( (hh < 10 ? "0" + String(hh) : String(hh)) + ":" + (mm < 10 ? "0" + String(mm) : String(mm)) );
        fiReg.close();
      }
      timerLoop = millis();
    }
    else
    {
      delay(1000);
    }
  }
}
String getConfigs(String key)
{
  String k = "";
  String v = "";
  fi = SD.open("/configs/configs.ini");
  DBG_OUTPUT_PORT.println("Open config");
  if (fi) {
    while (fi.available()) {
      k = fi.readStringUntil(',');
      v = fi.readStringUntil('\r');
      fi.readStringUntil('\n');
      if (k == key)
      {
        DBG_OUTPUT_PORT.println("Key:" + k + ", Val: '" + v + "'");
        fi.close();
        //if (v == "")return NULL;
        return v;
      }
    }
  }
  fi.close();
  return "";

}
void updateConfig() {
  if (configHasChanges)
  {
    File fiC;
    SD.remove("/configs/configs.ini");
    DBG_OUTPUT_PORT.println("file deleted");
    fiC = SD.open("/configs/configs.ini", FILE_WRITE);
    DBG_OUTPUT_PORT.println("data: \r\n" + filecont);
    fiC.print(filecont);
    DBG_OUTPUT_PORT.println("file printed again");
    fiC.close();
    configHasChanges = false;
    //loadConfigs();
  } else {
    if (rebootRequest)
    {
      DBG_OUTPUT_PORT.println("Restart");
      ESP.restart();
    }
  }
}
void checkThresholds()
{
  if (regEnable) {
    if (currentTemp < bottomThresholdT )
    {
      digitalWrite(TRELAY_PIN, LOW);
      calInited = true;
    }
    if (currentTemp > topThresholdT ) {
      digitalWrite(TRELAY_PIN, HIGH);
      calInited = false;
    }
    if ( currentHum < bottomThresholdH ) {
      digitalWrite(HRELAY_PIN, LOW);
      humInited = true;
    }
    if ( currentHum > topThresholdH ) {
      digitalWrite(HRELAY_PIN, HIGH);
      humInited = false;
    }
  }
  else
  {
    digitalWrite(TRELAY_PIN, HIGH);
    calInited = false;
    digitalWrite(HRELAY_PIN, HIGH);
    humInited = false;
  }
}

char* ip2CharArray(IPAddress ip) {
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}
void setup(void) {
  pinMode(LEDPIN, OUTPUT);
  pinMode(HRELAY_PIN, OUTPUT);
  pinMode(TRELAY_PIN, OUTPUT);
  digitalWrite(HRELAY_PIN, HIGH);
  digitalWrite(TRELAY_PIN, HIGH);
  //digitalWrite(LEDPIN, HIGH);
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  pinMode(HRELAY_PIN, OUTPUT);


  if (!SD.begin(CS_PIN)) {
    DBG_OUTPUT_PORT.println("initialization failed!");
    bool error = false;
    while (1) {
      digitalWrite(LEDPIN, (error = !error));
      delay(80);
    }
  }
  //  loadConfigs();
  //WiFi.mode(WIFI_STA);
  //WiFi.begin(ssid, password);

  //  WiFi.mode(WIFI_AP);
  //  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  //  WiFi.softAP("EspServer", NULL);
  WiFi.mode(WIFI_AP_STA );
  //WiFi.begin(getConfigs("wifissid"), getConfigs("wifipass"));
  String wifissidStr = getConfigs("wifissid");
  String wifipassStr = getConfigs("wifipass");
  if (wifissidStr.length() > 0) {
    WiFi.begin(wifissidStr.c_str(), (wifipassStr.length() == 0 ? NULL : wifipassStr.c_str() ));
    //quizas aca chequear....
    DBG_OUTPUT_PORT.print("Connecting to ");
    DBG_OUTPUT_PORT.println(wifissidStr);
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
      DBG_OUTPUT_PORT.print("Connected! IP address: ");
      DBG_OUTPUT_PORT.println( WiFi.localIP().toString());
      DBG_OUTPUT_PORT.println(localIp);
    } else
    {
      DBG_OUTPUT_PORT.print("Not connected to LAN");
    }
  } else
  {
    WiFi.mode(WIFI_AP);
  }


  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  if (rtc.lostPower()) {
    regEnable = false;
    //don't let register if clock is unset
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // July 27, 2020 at 12am you would call:
    //rtc.adjust(DateTime(2020, 1, 1, 12, 0, 0));
  }

  //dht.begin();
  dht.setup(DHTPin, DHTesp::DHT22);
  //checkSleepMode();
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  //WiFi.softAP(apssid, appass);
  String ssidStr = getConfigs("apssid");
  String appassStr = getConfigs("appass");
  WiFi.softAP(ssidStr.c_str(), ((appassStr.length() == 0) ? NULL : appassStr.c_str() ));

  regTime =  getConfigs("regtime").toInt() * 60;
  tAdj =  getConfigs("tadj").toInt();
  hAdj =  getConfigs("hadj").toInt();

  bottomThresholdT = getConfigs("bottomthresholdt").toInt();
  topThresholdT = getConfigs("topthresholdt").toInt();
  bottomThresholdH = getConfigs("bottomthresholdh").toInt();
  topThresholdH = getConfigs("topthresholdh").toInt();

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

  //  if (MDNS.begin(host)) {
  //    MDNS.addService("http", "tcp", 80);
  //    DBG_OUTPUT_PORT.println("MDNS responder started");
  //    DBG_OUTPUT_PORT.print("You can now connect to http://");
  //    DBG_OUTPUT_PORT.print(host);
  //    DBG_OUTPUT_PORT.println(".local");
  //  }

  //Pages
  server.on("/", HTTP_GET, index_handler);
  server.on("/config", HTTP_GET, configpage_handler);
  server.onNotFound(index_handler);
  //statics
  server.on("/bootstrap.bundle.min.js", HTTP_GET, bootstrapjs_handler);
  server.on("/bootstrap.css", HTTP_GET, bootstrapcss_handler);
  server.on("/jquery-3.js", HTTP_GET, jquerymin_handler);
  server.on("/chart.min.js", HTTP_GET, chartjs_handler);
  server.on("/render.js", HTTP_GET, renderjs_handler);
  server.on("/utils.js", HTTP_GET, utilsjs_handler);
  server.on("/configs.js", HTTP_GET, configsjs_handler);
  server.on("/logo.svg", HTTP_GET, logosvg_handler);
  server.on("/monodev.svg", HTTP_GET, monodevsvg_handler);
  //API
  server.on("/api/temp", HTTP_GET, temp_handler);
  server.on("/api/getRegs", HTTP_GET, getRegisters_handler);
  server.on("/api/getTempJson", HTTP_GET, temph_handler);
  server.on("/api/getConfigsJson", HTTP_GET, configsjson_handler);
  server.on("/api/getRTCConfigsJson", HTTP_GET, rtcconfigsjson_handler);
  server.on("/api/setConfig", HTTP_GET, setconfig_handler);
  server.on("/api/setRTCConfig", HTTP_GET, setrtcconfig_handler);
  server.on("/api/reboot", HTTP_GET, reboot_handler);
  server.on("/api/initReg", HTTP_GET, cmdreg_handler);
  server.on("/api/stopReg", HTTP_GET, cmdreg_handler);
  server.on("/api/delReg", HTTP_GET, cmdregdel_handler);
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

}


void loop(void) {

  dnsServer.processNextRequest();
  if (((millis() - timerLoop) / 1000) > (regTime))
  {
    DBG_OUTPUT_PORT.println("Register data");
    registerData();
  }
  if (((millis() - timerTempLoop) / 1000) > (10))
  {
    getDHTTemperature();
    getDHTHumidity();
    checkThresholds();
    DBG_OUTPUT_PORT.println("Temp: " + String(currentTemp));
    DBG_OUTPUT_PORT.println("Hum: " + String(currentHum));
    timerTempLoop = millis();

  }
  if (((millis() - timerLedLoop) ) > 1000)
  {
    digitalWrite(LEDPIN, ledStatus = (regEnable ? !ledStatus : !regEnable));
    timerLedLoop = millis();
  }
  updateConfig();




}
