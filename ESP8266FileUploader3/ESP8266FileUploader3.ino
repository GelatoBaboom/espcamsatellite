/**
   BasicHTTPClientPostUpload.ino

   See http://posttestserver.com/ for the HTTP server details.

   Use HTTP multipart form-data to upload a 256 byte file.

   References:
    https://tools.ietf.org/html/rfc7230
    https://tools.ietf.org/html/rfc7231
*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
//#define Serial Serial
ESP8266WiFiMulti WiFiMulti;

void blinkLed(uint8_t maxCycles, unsigned long  delayCycle) {
  for (uint8_t cycles = 0; cycles < maxCycles; cycles++ ) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayCycle);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delayCycle);
  }
}
void setup() {
  //Serial.begin(115200);
  Serial.begin(9600);
  // Serial.setDebugOutput(true);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println();
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    blinkLed(5, 100);
    delay(500);
  }

  WiFiMulti.addAP("GelatoBaboom", "friofrio");
}
void loop() {
  // wait for WiFi connection
  while (WiFiMulti.run() != WL_CONNECTED) {
    blinkLed(2, 1000);
  }

  int tries = 10;
  String inData = "";
  while (inData == "" && tries > 0) {
    blinkLed(2, 100);
    cleanSerialBuffer();
    Serial.println("areYouThere");
    delay(1000);
    inData = readSerialData();
    tries--;
  }
  if (inData.startsWith("Im here")) //Camera capture failed
  {
    cleanSerialBuffer();
    Serial.println("setCam");
    delay(700);
    Serial.println("frame size UXGA");//FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    delay(700);
    Serial.println("setCam");
    delay(700);
    Serial.println("bright");
    delay(700);
    Serial.println("-2");
    delay(700);
    getImage2();
    delay(5000);
    //Serial.println("reset");
    delay(15000);
  } else
  {
    delay(5000);
  }

}
void getImage2()
{
  HTTPClient http;
  int httpCode;

  int total_read = 0;
  cleanSerialBuffer();
  Serial.println("getImage");
  int length = -1;
  String inData = "";
  while (inData == "") {
    blinkLed(2, 100);
    inData = readSerialData();
  }
  if (inData.startsWith("Camera") || inData.startsWith("[E]")) //Camera capture failed
  {
    Serial.println("Error: " + inData);
    delay(700);
    return;
  }
  else
  {
    length = inData.toInt();
  }
  if (length == 0) {
    Serial.println("Error: " + inData);
    return;
  }
  cleanSerialBuffer();
  Serial.println("ok " + inData);


  //Serial.write(buffer, chunk);
  digitalWrite(LED_BUILTIN, LOW);
  int bytesReaded = 0;
  while (bytesReaded < length ) {
    int chunkLength =  Serial.available() ;
    if (chunkLength > 0) {
      uint8_t data[chunkLength];
      Serial.readBytes(data, chunkLength);
      bytesReaded += chunkLength;
      http.begin("http://192.168.1.45:14693/file.ashx");
      http.addHeader("Content-Type", "image/jpg", false, true);
      httpCode = http.sendRequest("POST", &data[0], chunkLength );
      if (httpCode == HTTP_CODE_OK) {
        //Serial.println("ok http");
      } else {
        //Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  }
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("received");
  endFile();
}
void endFile()
{
  HTTPClient http;
  int httpCode;
  http.begin("http://192.168.1.45:14693/endFile.ashx");
  http.addHeader("Content-Type", "text/plain");
  httpCode = http.POST("Message from ESP8266");
  if (httpCode == HTTP_CODE_OK) {
    //Serial.println("ok http");
  } else {
    //Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
void getImage()
{
  HTTPClient http;
  int httpCode;

  int total_read = 0;
  cleanSerialBuffer();
  Serial.println("getImage");
  int length = -1;
  String inData = "";
  while (inData == "") {
    blinkLed(2, 100);
    inData = readSerialData();
  }
  if (inData.startsWith("Camera") || inData.startsWith("[E]")) //Camera capture failed
  {
    Serial.println("Error: " + inData);
    delay(700);
    return;
  }
  else
  {
    length = inData.toInt();
  }
  if (length == 0) {
    Serial.println("Error: " + inData);
    return;
  }
  cleanSerialBuffer();
  Serial.println("ok " + inData);
  //Serial.write(buffer, chunk);
  digitalWrite(LED_BUILTIN, LOW);
  //http.begin("http://192.168.1.45:14693/fileStreamEnterly.ashx");
  http.begin("http://img.monodev.tk/fileStreamEnterly.ashx");
  http.addHeader("Content-Type", "image/jpg", false, true);
  httpCode = http.sendRequest("POST", &Serial, length );
  if (httpCode == HTTP_CODE_OK) {
    //Serial.println("ok http");
  } else {
    //Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("received");
}


void cleanSerialBuffer() {
  while (Serial.available() > 0) {
    int inChar = Serial.read();
  }
}
String readSerialData() {
  if (Serial.available() > 0) {
    //Serial.println("data available: " + String(Serial.available()));
    String inString = "";
    while (Serial.available() > 0) {
      int inChar = Serial.read();
      inString += (char)inChar;
      //Serial.print((char)inChar);
      // if you get a newline, print the string, then the string's value:
      if (inChar == '\n') {
        //Serial.println(inString.toInt());
        return inString;
      }
    }
  }
  return "";
}
int readSerialDataInt() {
  if (Serial.available() > 0) {
    //Serial.println("data available: " + String(Serial.available()));
    String inString = "";
    while (Serial.available() > 0) {
      int inChar = Serial.read();
      if (isDigit(inChar)) {
        // convert the incoming byte to a char and add it to the string:
        inString += (char)inChar;
      } else
      {
        return -2;
      }
      //Serial.print((char)inChar);
      // if you get a newline, print the string, then the string's value:
      if (inChar == '\n') {
        //Serial.println(inString.toInt());
        return inString.toInt();
      }
    }
  }
  return -1;
}
char * copySerial()
{
  int total_read = 0;
  Serial.println("getImage");
  int length = -1;
  while (length == -1) {
    delay(100);
    length = readSerialDataInt();
  }

  Serial.flush();
  Serial.println("ok");
  while (Serial.available() == 0) {
    delay(100);
  }
  char buf [length];//= malloc(length);
  //realloc(buf, length);
  total_read = Serial.readBytes(buf, length);
  Serial.printf("File content: %s\n", buf);
  if (length == total_read)
  {
    Serial.println("received");
  }
  return buf;
}


