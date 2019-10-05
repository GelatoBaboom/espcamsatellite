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
const char PROGMEM CONTENT_HEADERS[] =
  "\r\nContent-Disposition: form-data; name=\"testfile\"; filename=\"image.jpg\"\r\n"
  "Content-Type: image/jpeg\r\n\r\n";

void setup() {
  //Serial.begin(115200);
  Serial.begin(9600);
  // Serial.setDebugOutput(true);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP("GelatoBaboom", "friofrio");
}

void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    HTTPClient http;
    int httpCode;

    char body[2048];
    char boundary[32] = "----";

    randomSeed(micros() + analogRead(A0));
    for (int i = 0; i < 3; i++) {
      ltoa(random(0x7FFFFFF), boundary + strlen(boundary), 16);
    }

    Serial.print("[HTTP] begin...\n");
    // Use posttestserver.com
    http.begin("http://192.168.1.45:14693/file.ashx");
    strcpy(body, "multipart/form-data; boundary=");
    strcat(body, &boundary[2]);
    Serial.printf("[HTTP] Content-Type: %s\n", body);
    //http.addHeader("Content-Type", body, false, true);
    strcpy(body, boundary);
    strcat_P(body, CONTENT_HEADERS);
    int content_length = strlen(body);

    int total_read = 0;
    cleanSerialBuffer();
    Serial.println("getImage");
    int length = -1;
    while (length == -1) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      length = readSerialDataInt();
    }

    http.addHeader("Content-Type", "image/jpg", false, true);
    //httpCode = http.POST("title=foo&body=bar&userId=1");

    cleanSerialBuffer();
    Serial.println("ok");
//    while (Serial.available() == 0) {
//
//      digitalWrite(LED_BUILTIN, LOW);
//      delay(500);
//      digitalWrite(LED_BUILTIN, HIGH);
//      delay(500);
//    }
    //    int bufFill = 0;
    //    byte buffer[length];
    //    while (bufFill < length) {
    //      Serial.println(bufFill);
    //      int chunk = Serial.available();
    //      Serial.readBytes(buffer, chunk );
    //      bufFill += chunk ;
    //    }
    //    http.sendRequest("POST", buffer, length );
    http.sendRequest("POST", &Serial, length );
    //    while (length) {
    //      size_t will_copy = (length < bufferSize) ? length : bufferSize;
    //      //SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
    //      char fileChunk [bufferSize];
    //      will_copy = Serial.readBytes(fileChunk, bufferSize);
    //      if (!client.connected()) break;
    //      client.write(fileChunk, will_copy);
    //          http.writeToStream(&Serial);
    //      length -= will_copy;
    //    }


    Serial.println("received");








    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }

  delay(30000);
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

