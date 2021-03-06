
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

#define GPIO_SERVO 12
#define GPIO_FLASH 4
#define GPIO_RESET 14
#define GPIO_HC12 2

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
//#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
framesize_t frame_size = FRAMESIZE_VGA;
int pictureNumber = 0;

bool camInitialized = false;
bool flashOn = false;
bool witnessOn = true;
int currentBaudRate = 9600;
hw_timer_t * timer = NULL;

// Tilt the ESP32-CAM external LED Function
void tiltLed(int minRiseVal, uint8_t maxRiseVal, uint8_t maxCycles, unsigned long  delayCycle) {
  bool rise = true;
  uint8_t cycles = 0;

  for (int dutyCycle = (minRiseVal + 1); rise == (dutyCycle <= (rise ? (maxRiseVal + 1) : (minRiseVal - 1))); (rise ? dutyCycle++ : dutyCycle--) ) {
    ledcWrite(2, dutyCycle);
    if ((dutyCycle == maxRiseVal || dutyCycle == minRiseVal) && cycles < ((maxCycles * 2) - 1)) {
      rise = !rise ;
      cycles++;
    }
    delayMicroseconds(delayCycle);
  }
  ledcWrite(2, 0);
}

void setup() {
  //Servo init
  servo1.attach(GPIO_SERVO, 550, 2400);
  pinMode(GPIO_FLASH, OUTPUT);//Flash Led
  //pinMode(GPIO_RESET, OUTPUT);//Probably hard reset
  ledcAttachPin(GPIO_RESET, 2);//test led
  ledcSetup(2, 5000, 8);
  //pinMode(GPIO_HC12, OUTPUT);//HC-12 AT Command control
  ledcAttachPin(13, 4);//test led
  ledcSetup(4, 5000, 8);

  //Serial Init
  //Serial.begin(115200);
  //Serial.begin(9600);
  Serial.begin(2400);
  //Serial.setDebugOutput(true);
  //digitalWrite(GPIO_HC12, HIGH);
  ledcWrite(4, 255);

  //Dallas temp sensor
  DS18B20.begin();

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  //Set fram config
  frame_size = FRAMESIZE_UXGA;

  // tilt the LED
  tiltLed(30, 255, 10, 500);

  //setBaudRate(9600, false);
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
  if (inData.startsWith("setAnt"))
  {
    setAnt();
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
  if (inData.startsWith("antDown"))
  {
    antennaDown();
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
      if (isDigit(inChar) || inChar == '-') {
        inString += (char)inChar;
      }
      if (inChar == '\n') {
        return inString.toInt();
      }
    }
  }
  return -1000;
}
void initCam()
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
    //    sensor_t * s = esp_camera_sensor_get();
    //    s->set_vflip(s, 1);
    //    s->set_hmirror(s, 1);
  }
}
void getImage()
{
  initCam();

  digitalWrite(GPIO_FLASH, flashOn ? HIGH : LOW);
  camera_fb_t * fb = NULL;
  // Take Picture with Camera
  delay(100);
  fb = esp_camera_fb_get();
  digitalWrite(GPIO_FLASH, LOW);
  if (!fb) {
    Serial.println("Camera capture failed");
    if (witnessOn) tiltLed(0, 255, 4, 1000);
    return;
  }
  if (witnessOn) tiltLed(0, 255, 1, 1000);
  delay(500);

  Serial.println(String(fb->len));

  delay(100);
  String inData = readSerialData();
  while (inData == "") {
    delay(100);
    inData = readSerialData();
  }
  if (inData.startsWith("ok"))
  {
    ledcWrite(2, 255);
    //Serial.write(fb->buf, fb->len);
    int position = 0 ;
    int length  = fb->len;
    const uint8_t *data = fb->buf;
    int buffChunkLength = 128;
    while (position < length /*&& inData.startsWith("ok")*/) {
      cleanSerialBuffer();
      ledcWrite(2, (255 - ((position * 255) / length)));
      int bufLength = ((position + buffChunkLength) > length) ? length - position : buffChunkLength;
      Serial.write(&data[position], bufLength);
      //position = ((position + bufLength) >= length) ? position + bufLength :  position;
      Serial.flush();
      int recTries = 1000;
      while (Serial.available() < bufLength && recTries > 0) {
        delay(10);
        recTries--;
      }
      bool done = true;
      if (Serial.available() >= bufLength) {
        uint8_t dataRet[bufLength];
        Serial.readBytes(dataRet, bufLength);

        for (int i = 0; i < bufLength; i++)
        {
          if (dataRet[i] != (&data[position])[i]) {
            done = false;
          }
        }
      } else {
        done = false;
      }
      if (done) {
        Serial.println("ok");
        position += bufLength;
      } else
      {
        Serial.println("wrong");
      }
      //check synchro
      cleanSerialBuffer();
      recTries = 1000;
      inData == "";
      inData = readSerialData();
      while (inData == "" && recTries > 0) {
        delay(10);
        inData = readSerialData();
        recTries--;
      }
      if (inData.startsWith("snd")) {

      }
      cleanSerialBuffer();
      //end synchro

    }
    ledcWrite(2, 0);
    if (witnessOn) tiltLed(50, 255, 2, 1000);
    inData == "";
    inData = readSerialData();
    while (inData == "") {
      delay(100);
      inData = readSerialData();
    }
    if (inData.startsWith("received"))
    {
      if (witnessOn) tiltLed(0, 255, 1, 1000);
      //ESP.restart();
    }

  }
  esp_camera_fb_return(fb);
}
void setCam() {
  initCam();
  sensor_t * s = esp_camera_sensor_get();
  Serial.println("Send config");
  String inData = readSerialData();
  while (inData == "") {
    delay(100);
    inData = readSerialData();
  }
  if (inData.startsWith("flash on"))
  {
    flashOn = true;
    digitalWrite(GPIO_FLASH, HIGH);
    delay(500);
    digitalWrite(GPIO_FLASH, LOW);
    Serial.println("Ok flash on");
  }
  if (inData.startsWith("flash off"))
  {
    flashOn = false;
    Serial.println("Ok flash off");
  }
  if (inData.startsWith("frame size UXGA"))
  {
    s->set_framesize(s, FRAMESIZE_UXGA);
    Serial.println("Ok UXGA");
  }
  if (inData.startsWith("frame size SXGA"))
  {
    s->set_framesize(s, FRAMESIZE_SXGA);
    Serial.println("Ok SXGA");
  }
  if (inData.startsWith("frame size XGA"))
  {
    s->set_framesize(s, FRAMESIZE_XGA);
    Serial.println("Ok XGA");
  }
  if (inData.startsWith("frame size SVGA"))
  {
    s->set_framesize(s, FRAMESIZE_SVGA);
    Serial.println("Ok SVGA");
  }
  if (inData.startsWith("frame size VGA"))
  {
    s->set_framesize(s, FRAMESIZE_VGA);
    Serial.println("Ok VGA");
  }
  if (inData.startsWith("bright"))
  {
    Serial.println("Send value");
    int v = readSerialDataInt();
    int tries = 200;
    while (v == -1000 && tries > 0) {
      delay(100);
      v = readSerialDataInt();
      tries--;
    }

    s->set_brightness(s, v);
    Serial.println("Brightness set " + String(v));

  }
  if (inData.startsWith("exposure"))
  {
    Serial.println("Send value");
    int v = readSerialDataInt();
    int tries = 200;
    while (v == -1000 && tries > 0) {
      delay(100);
      v = readSerialDataInt();
      tries--;
    }

    s->set_exposure_ctrl(s, v);
    Serial.println("Exposure set " + String(v));

  }

  if (inData.startsWith("flip"))
  {
    Serial.println("Send value");
    int v = readSerialDataInt();
    int tries = 200;
    while (v == -1000 && tries > 0) {
      delay(100);
      v = readSerialDataInt();
      tries--;
    }

    s->set_vflip(s, v);
    Serial.println("Flip set " + String(v));
  }

  //End configs
  Serial.println("Exit setCam");
}
void setAnt() {
  Serial.println("Send config");
  String inData = readSerialData();
  while (inData == "") {
    delay(100);
    inData = readSerialData();
  }
  if (inData.startsWith("baudrate"))
  {
    Serial.println("Send value 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200");
    int v = readSerialDataInt();
    int tries = 200;
    while (v == -1000 && tries > 0) {
      delay(100);
      v = readSerialDataInt();
      tries--;
    }
    Serial.println("Setting baud rate at " + String(v));
    setBaudRate(v, true);
  }


}
void setBaudRate(int v, bool logit)
{

  delay(500);
  //(GPIO_HC12, LOW);
  ledcWrite(4, 0);
  if (currentBaudRate != v) {
    Serial.end();
    delay(1000);
    Serial.begin(9600);
  }
  while (!Serial) {
    delay(10);
  }
  Serial.println("");
  Serial.println("AT+B" + String(v));
  int tries = 200;
  String atData = readSerialData();
  while (atData == "" && tries > 0) {
    delay(100);
    tries--;
    atData = readSerialData();
  }
  //digitalWrite(GPIO_HC12, HIGH);
  ledcWrite(4, 255);
  if (atData.startsWith("OK+B"))
  {
    if (currentBaudRate != v) {
      Serial.end();
      delay(1000);
      Serial.begin(v);
      while (!Serial) {
        delay(10);
      }
      currentBaudRate = v;
    }
    delay(1000);
    if (logit)  Serial.println("Done setting baud rate");
  }
  else
  {
    Serial.end();
    delay(1000);
    Serial.begin(currentBaudRate);
    while (!Serial) {
      delay(10);
    }
    if (logit)Serial.println("Fail setting baud rate");
  }

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
}
void restart()
{
  Serial.println("Restarting...");
  Serial.flush();
  delay(2000);
  ESP.restart();

}
void resetESP()
{
  Serial.println("Reseting...");
  digitalWrite(GPIO_RESET, HIGH);
  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR / 2);
  delay(1000);
  esp_deep_sleep_start();

}
void IRAM_ATTR wakeAntenna()
{
  resetESP();
  //  digitalWrite(GPIO_HC12, HIGH);
  //  if (timer != NULL) {
  //    timerAlarmDisable(timer);
  //    timerDetachInterrupt(timer);
  //    timerEnd(timer);
  //    timer = NULL;
  //  }
  //  delay(500);
  //  Serial.println("Serial module it's up again");
}
void antennaDown()
{
  Serial.println("Cutting transmition...");
  Serial.flush();
  delay(1000);
  digitalWrite(GPIO_HC12, LOW);
  Serial.println("AT+SLEEP");
  int tries = 200;
  String atData = readSerialData();
  while (atData == "" && tries > 0) {
    delay(100);
    tries--;
    atData = readSerialData();
  }
  if (atData.startsWith("OK+SLEEP"))
  {
    delay(1000);

    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &wakeAntenna, true);
    timerAlarmWrite(timer, uS_TO_S_FACTOR * 10, true);
    timerAlarmEnable(timer);

  }
  else
  {
    Serial.println("Serial module can�t sleep");
    delay(1000);
    resetESP();
  }
  Serial.println("This message must not being seeing");
}



void goToSleep() {
  Serial.println("Set time to sleep(in seconds)");
  int timeToSleep = readSerialDataInt();
  int tries = 200;
  while (timeToSleep == -1000 && tries > 0) {
    delay(100);
    timeToSleep = readSerialDataInt();
    tries--;
  }
  if (timeToSleep != -1000) {
    esp_sleep_enable_timer_wakeup(timeToSleep * uS_TO_S_FACTOR);
    Serial.println("Going to sleep now");
    Serial.flush();
    delay(1000);
    digitalWrite(GPIO_HC12, LOW);
    delay(200);
    Serial.println("AT+SLEEP");
    int tries = 200;
    String atData = readSerialData();
    while (atData == "" && tries > 0) {
      delay(100);
      tries--;
      atData = readSerialData();
    }
    if (atData.startsWith("OK+SLEEP"))
    {
      esp_deep_sleep_start();
    }
    else
    {
      Serial.println("Serial module can�t sleep");
    }

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
  while (angle == -1000 && tries > 0) {
    delay(100);
    angle = readSerialDataInt();
    tries--;
  }
  if (angle != -1000) {
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
void cleanSerialBuffer() {
  while (Serial.available() > 0) {
    int inChar = Serial.read();
  }
}



