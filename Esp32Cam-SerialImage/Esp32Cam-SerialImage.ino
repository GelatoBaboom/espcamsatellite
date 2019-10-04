/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-take-photo-save-microsd-card

  IMPORTANT!!!
   - Select Board "ESP32 Wrover Module"
   - Select the Partion Scheme "Huge APP (3MB No OTA)
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

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

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 15

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);


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
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

int pictureNumber = 0;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  // Connect to Wi-Fi network with SSID and password
  //Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  Serial.begin(115200);
  //Serial.setDebugOutput(true);

  DS18B20.begin();

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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  pinMode(4, OUTPUT);
  for (uint8_t t = 20; t > 0; t--) {
    digitalWrite(4, HIGH);
    delay(10);
    digitalWrite(4, LOW);
    delay(100);
  }

  //digitalWrite(4, LOW);
  //rtc_gpio_hold_en(GPIO_NUM_4);
  //  pinMode(12, OUTPUT);
  //  digitalWrite(12, HIGH);
  //  delay(1000);
  //  digitalWrite(12, LOW);


}
void loop() {

  String inData = readSerialData();
  if (inData.startsWith("getImage"))
  {
    digitalWrite(4, HIGH);
    camera_fb_t * fb = NULL;
    // Take Picture with Camera
    delay(700);
    digitalWrite(4, LOW);
    //sin flash
    fb = esp_camera_fb_get();
       
    if (!fb) {
      Serial.println("Camera capture failed");
      digitalWrite(4, HIGH);
      delay(500);
      digitalWrite(4, LOW);
      delay(500);
      digitalWrite(4, HIGH);
      delay(500);
      digitalWrite(4, LOW);
      return;
    }
    Serial.println(String(fb->len));
    delay(100);
    inData = readSerialData();
    while (inData == "") {
      delay(100);
      inData = readSerialData();
    }
    if (inData.startsWith("ok"))
    {

      //const char *data = (const char *)fb->buf;
      //Serial.write(data, fb->len);

      Serial.write(fb->buf, fb->len);
      //      int position = 0 ;
      //      int length  = fb->len;
      //      const uint8_t *data = fb->buf;
      //      while (position < length) {
      //
      //        int bufLength = ((position + 4096) > length) ? length - position : 4096;
      //        Serial.write(&data[position], bufLength);
      //        //position = ((position + bufLength) >= length) ? position + bufLength :  position;
      //        position +=bufLength;
      //        Serial.flush();
      //      }

      digitalWrite(4, HIGH);
      delay(100);
      digitalWrite(4, LOW);
      delay(100);
      digitalWrite(4, HIGH);
      delay(100);
      digitalWrite(4, LOW);
      inData = readSerialData();
      while (inData == "") {
        delay(100);
        inData = readSerialData();
      }
      if (inData.startsWith("received"))
      {
        esp_camera_fb_return(fb);
        digitalWrite(4, HIGH);
        delay(100);
        digitalWrite(4, LOW);
        ESP.restart();
      }

    }

  }
  delay(500);
}
//put parameter with timer to time out this func
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

//  float temp;
//  Serial.println("Check temp..");
//  //do {
//  DS18B20.requestTemperaturesByIndex(0);
//  temp = DS18B20.getTempCByIndex(0);
//  delay(100);
//  // } while (temp == 85.0 || temp == (-127.0));
//  Serial.println("temp: " + String(temp));
//
//  delay(2000);
//  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
//  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
//                 " Seconds");
//  Serial.println("Going to sleep now");
//  Serial.flush();
//  esp_deep_sleep_start();
//}

