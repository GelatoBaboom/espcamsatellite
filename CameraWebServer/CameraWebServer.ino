#include "esp_camera.h"
#include "camera_pins.h"



#define GPIO_EXT_LED 14
#define GPIO_FLASH 4


void startCameraServer();
void initComponents();
String getWifiIP();
void getTemp();

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
  //Serial.begin(115200);
  Serial.begin(9600);
  //Serial.setDebugOutput(true);
  //Serial.println();
  pinMode(GPIO_FLASH, OUTPUT);//Flash Led
  ledcAttachPin(GPIO_EXT_LED, 2);//test led
  ledcSetup(2, 5000, 8);

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
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);
  //
  //#if defined(CAMERA_MODEL_M5STACK_WIDE)
  //  s->set_vflip(s, 1);
  //  s->set_hmirror(s, 1);
  //#endif

  initComponents();

  startCameraServer();
  // tilt the LED
  tiltLed(30, 255, 10, 500);
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(getWifiIP());
  Serial.println("' to connect");
}

void loop() {
  // put your main code here, to run repeatedly:
  //getTemp();
  delay(10000);
}
