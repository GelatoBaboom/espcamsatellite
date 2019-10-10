
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
//#include "FS.h"                // SD Card ESP32
//#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "dl_lib.h"
#include "driver/rtc_io.h"
//#include <EEPROM.h>            // read and write from flash memory
#include <ESP32_Servo.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 15

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
Servo servo1;

// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
//#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
framesize_t frame_size = FRAMESIZE_VGA;
int pictureNumber = 0;

bool camInitialized = false;
bool flashLed = false;
void setup() {
  //Servo init
  servo1.attach(12, 550, 2400);
  // tilt the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  pinMode(4, OUTPUT);//Flash Led
  pinMode(14, OUTPUT);//Probably hard reset
  pinMode(2, OUTPUT);//HC-12 AT Command control

  //Serial.begin(115200);
  Serial.begin(9600);
  //Serial.setDebugOutput(true);
  //Dallas temp sensor
  DS18B20.begin();

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  frame_size = FRAMESIZE_VGA;
  for (uint8_t t = 20; t > 0; t--) {
    digitalWrite(4, flashLed ? HIGH : LOW);
    delay(10);
    digitalWrite(4, LOW);
    delay(50);
  }

  Serial.println("");
  Serial.println("Im awake again!");

}
void loop() {

  String inData = readSerialData();
  if (inData.startsWith("getImage"))
  {
    getImage();
  }
  if (inData.startsWith("setCam"))
  {
    setCam();
  }
  if (inData.startsWith("getTemp"))
  {
    getTemp();
  }
  if (inData.startsWith("restart"))
  {
    restart();
  }
  if (inData.startsWith("reset"))
  {
    resetESP();
  }
  if (inData.startsWith("gotoSleep"))
  {
    goToSleep();
  }
  if (inData.startsWith("move"))
  {
    moveCam();
  }
  if (inData.startsWith("areYouThere"))
  {
    Serial.println("Im here");
  }
  delay(500);
}
//put parameter with timer to time out this func
String readSerialData() {
  if (Serial.available() > 0) {
    String inString = "";
    while (Serial.available() > 0) {
      int inChar = Serial.read();
      inString += (char)inChar;
      if (inChar == '\n') {
        return inString;
      }
    }
  }
  return "";
}
int readSerialDataInt() {
  if (Serial.available() > 0) {
    String inString = "";
    while (Serial.available() > 0) {
      int inChar = Serial.read();
      if (isDigit(inChar)) {

        inString += (char)inChar;
      }
      if (inChar == '\n') {
        return inString.toInt();
      }
    }
  }
  return -1;
}
void getImage()
{
  if (!camInitialized) {
    //Camera configs
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;


    config.frame_size = frame_size; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
    // Init Camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x", err);
      return;
    }
    camInitialized = true;
  }
  digitalWrite(4, flashLed ? HIGH : LOW);
  camera_fb_t * fb = NULL;
  // Take Picture with Camera
  delay(700);
  digitalWrite(4, LOW);
  //sin flash
  fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    digitalWrite(4, flashLed ? HIGH : LOW);
    delay(500);
    digitalWrite(4, LOW);
    delay(500);
    digitalWrite(4, flashLed ? HIGH : LOW);
    delay(500);
    digitalWrite(4, LOW);
    return;
  }
  Serial.println(String(fb->len));
  delay(100);
  String inData = readSerialData();
  while (inData == "") {
    delay(100);
    inData = readSerialData();
  }
  if (inData.startsWith("ok"))
  {
    Serial.write(fb->buf, fb->len);
    digitalWrite(4, flashLed ? HIGH : LOW);
    delay(100);
    digitalWrite(4, LOW);
    delay(100);
    digitalWrite(4, flashLed ? HIGH : LOW);
    delay(100);
    digitalWrite(4, LOW);
    inData = readSerialData();
    while (inData == "") {
      delay(100);
      inData = readSerialData();
    }
    if (inData.startsWith("received"))
    {

      digitalWrite(4, flashLed ? HIGH : LOW);
      delay(100);
      digitalWrite(4, LOW);

      //ESP.restart();
    }

  }
  esp_camera_fb_return(fb);
}
void setCam() {
  Serial.println("Send config");
  String inData = readSerialData();
  while (inData == "") {
    delay(100);
    inData = readSerialData();
  }
  if (inData.startsWith("flash on"))
  {
    flashLed = true;
    Serial.println("Ok flash on");
  }
  if (inData.startsWith("flash off"))
  {
    flashLed = false;
    Serial.println("Ok flash off");
  }
  if (inData.startsWith("frame size UXGA"))
  {
    frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    Serial.println("Ok UXGA");
  }
  if (inData.startsWith("frame size VGA"))
  {
    //config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    Serial.println("Ok VGA");
  }
  Serial.println("Exit setCam");
}
void getTemp()
{
  float temp;
  //Serial.println("Check temp..");
  //do {
  DS18B20.requestTemperaturesByIndex(0);
  temp = DS18B20.getTempCByIndex(0);
  delay(100);
  // } while (temp == 85.0 || temp == (-127.0));
  Serial.println("temp: " + String(temp));

  //delay(2000);
  //  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  //  Serial.println("Going to sleep now");
  //  Serial.flush();
  //  esp_deep_sleep_start();

}
void restart()
{
  Serial.println("Restarting...");
  Serial.flush();
  digitalWrite(2, HIGH);
  delay(2000);
  ESP.restart();

}
void resetESP()
{
  Serial.println("Reseting...");
  digitalWrite(14, HIGH);
  esp_sleep_enable_timer_wakeup(500000);
  delay(1000);
  esp_deep_sleep_start();

}
void goToSleep() {
  Serial.println("Set time to sleep(in seconds)");
  int timeToSleep = readSerialDataInt();
  int tries = 200;
  while (timeToSleep == -1 && tries > 0) {
    delay(100);
    timeToSleep = readSerialDataInt();
    tries--;
  }
  if (timeToSleep != -1) {
    esp_sleep_enable_timer_wakeup(timeToSleep * uS_TO_S_FACTOR);
    Serial.println("Going to sleep now");
    Serial.flush();
    delay(2000);
    esp_deep_sleep_start();
  }
  else
  {
    Serial.println("Invalid value");
  }
}

void moveCam()
{
  Serial.println("Set angle(from 0 to 180)");
  int angle = readSerialDataInt();
  int tries = 200;
  while (angle == -1 && tries > 0) {
    delay(100);
    angle = readSerialDataInt();
    tries--;
  }
  if (angle != -1) {
    Serial.println("Go to angle: " + String(angle));
    Serial.flush();
    delay(500);
    int pos = servo1.read();
    int p = pos;
    if (angle < pos) {

      for (uint8_t t = 0; t < (pos - angle ); t++) {
        p = p - 1;
        servo1.write(p);
        delay(10);
      }
    } else {
      int p = pos;
      for (uint8_t t = 0; t < (angle - pos); t++) {
        p = p + 1;
        servo1.write(p);
        delay(10);
      }
    }

    Serial.println("Angle: " + String(servo1.read()));
  }
  else
  {
    Serial.println("Invalid value");
  }
}


