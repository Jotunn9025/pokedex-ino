#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define CAMERA_MODEL_AI_THINKER
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

#define CAPTURE_INTERVAL 60000

const char* ssid = "name";
const char* password = "password";
const char* server = "api url";
const int port = 5000;
const char* endpoint = "/predict";

String customBase64Encode(const uint8_t* data, size_t len) {
  const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  String encoded;
  encoded.reserve(((len + 2) / 3) * 4);

  for (size_t i = 0; i < len; i += 3) {
    uint32_t group = (data[i] << 16) | (i + 1 < len ? data[i + 1] << 8 : 0) | (i + 2 < len ? data[i + 2] : 0);

    encoded += base64_chars[(group >> 18) & 0x3F];
    encoded += base64_chars[(group >> 12) & 0x3F];
    encoded += i + 1 < len ? base64_chars[(group >> 6) & 0x3F] : '=';
    encoded += i + 2 < len ? base64_chars[group & 0x3F] : '=';
  }

  return encoded;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

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
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  captureLoop();
}

void captureLoop() {
  unsigned long previousMillis = 0;
  Serial.println("Inside captureLoop");
  while (1) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= CAPTURE_INTERVAL) {
      previousMillis = currentMillis;

      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        continue;
      }

      String encodedImage = customBase64Encode(fb->buf, fb->len);

      Serial.println("Base64 Encoded Image:");
      Serial.println(encodedImage);

      HTTPClient http;
      http.begin("http://" + String(server) + ":" + String(port) + endpoint);
      

      http.addHeader("Content-Type", "application/json");

      String payload = "{\"data\":\"";
      payload += encodedImage;
      payload += "\"}";

      int httpCode = http.POST(payload);

      if (httpCode > 0) {
        String response = http.getString();
        Serial.println("API Response:");
        Serial.println(response);
      } else {
        Serial.print("HTTP error code: ");
        Serial.println(httpCode);
      }

      http.end();
      esp_camera_fb_return(fb);
    }
  }
}

void loop() {}
