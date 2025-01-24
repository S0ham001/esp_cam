#include <WiFi.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>

// Hotspot Credentials
const char* ssid = "ESP32_Hotspot";
const char* password = "12345678";

// ESP32-CAM Stream URL
const char* streamURL = "http://<ESP32-CAM-IP>/mjpeg/1"; // Replace <ESP32-CAM-IP> with the ESP32-CAM's IP

// TFT Display Initialization
TFT_eSPI tft = TFT_eSPI();

WiFiClient client;

void fetchAndDisplayLiveStream() {
  if (!client.connected()) {
    // Reconnect to the stream
    if (client.connect(WiFi.localIP(), 80)) {
      client.print(String("GET ") + streamURL + " HTTP/1.1\r\n" +
                   "Host: " + WiFi.localIP().toString() + "\r\n" +
                   "Connection: keep-alive\r\n\r\n");
    } else {
      Serial.println("Failed to connect to stream");
      return;
    }
  }

  // Process MJPEG stream
  while (client.available()) {
    if (client.read() == 0xFF) {
      if (client.read() == 0xD8) { // Start of JPEG
        uint8_t buf[1024];
        int len = 0;

        // Read JPEG into buffer
        while (client.available()) {
          buf[len++] = client.read();
          if (len >= sizeof(buf)) break;
          if (buf[len - 2] == 0xFF && buf[len - 1] == 0xD9) break; // End of JPEG
        }

        // Decode and display the JPEG
        if (JpegDec.decodeArray(buf, len)) {
          tft.fillScreen(TFT_BLACK);
          tft.setAddrWindow(0, 0, JpegDec.width, JpegDec.height);

          for (int y = 0; y < JpegDec.height; y++) {
            for (int x = 0; x < JpegDec.width; x++) {
              uint16_t color = JpegDec.read();
              tft.pushColor(color);
            }
          }

          JpegDec.abort();
        } else {
          Serial.println("Failed to decode JPEG");
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize the TFT display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Starting...", 10, 10);

  // Set up the ESP32 as a Wi-Fi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("ESP32 Hotspot started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  tft.fillScreen(TFT_BLACK);
  tft.drawString("Waiting for stream...", 10, 10);

  // Connect to the MJPEG stream
  delay(5000); // Wait for ESP32-CAM to connect
}

void loop() {
  // Continuously fetch and display live footage
  fetchAndDisplayLiveStream();
}
