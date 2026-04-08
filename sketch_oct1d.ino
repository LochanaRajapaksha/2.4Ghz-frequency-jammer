#include <SPI.h>
#include <RF24.h>
#include <ezButton.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string>
#include "images1-MSIThinA15.h"

#define NRF_CE 4
#define NRF_CSN 5
#define SCK 18
#define MISO 19
#define MOSI 23
#define BTN_PIN 13
#define SDA_PIN 21
#define SCL_PIN 22

RF24 radio(NRF_CE, NRF_CSN);
byte i = 45;
ezButton button(BTN_PIN);
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

const int wifiFrequencies[] = {
  2412, 2417, 2422, 2427, 2432,
  2437, 2442, 2447, 2452, 2457, 2462
};

const int LARGE_WIDTH = 1080;
const int LARGE_HEIGHT = 1080;
const int LARGE_BYTES_PER_ROW = LARGE_WIDTH / 8;

void drawScaledBitmap(int16_t dx, int16_t dy, int16_t dw, int16_t dh, const unsigned char* bitmap) {
  for (int16_t y = 0; y < dh; y++) {
    int32_t large_y = ((int32_t)y * LARGE_HEIGHT) / dh;
    for (int16_t x = 0; x < dw; x++) {
      int32_t large_x = ((int32_t)x * LARGE_WIDTH) / dw;
      int32_t byte_index = large_y * LARGE_BYTES_PER_ROW + (large_x / 8);
      uint8_t bit_pos = 7 - (large_x % 8);
      bool pixel = (bitmap[byte_index] & (1 << bit_pos)) != 0;
      if (pixel) {
        display.drawPixel(dx + x, dy + y, WHITE);
      }
    }
  }
}

void displayMessage(const char* line, uint8_t x = 65, uint8_t y = 22, const unsigned char* bitmap = nullptr, bool fullScreenImage = false) {
  radio.powerDown();
  SPI.end();
  delay(10);
  display.clearDisplay();
  if (bitmap != nullptr) {
    if (fullScreenImage) {
      drawScaledBitmap(0, 0, 128, 64, bitmap);
    } else {
      drawScaledBitmap(0, 0, 64, 64, bitmap);
    }
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  String text = String(line);
  int16_t cursor_y = y;
  int16_t maxWidth = 128 - x;
  while (text.length() > 0) {
    int16_t charCount = 0;
    int16_t lineWidth = 0;
    while (charCount < text.length() && lineWidth < maxWidth) {
      charCount++;
      lineWidth = 6 * charCount;
    }
    if (charCount < text.length()) {
      int16_t lastSpace = text.substring(0, charCount).lastIndexOf(' ');
      if (lastSpace > 0) {
        charCount = lastSpace + 1;
      }
    }
    display.setCursor(x, cursor_y);
    display.println(text.substring(0, charCount));
    text = text.substring(charCount);
    cursor_y += 10;
    if (cursor_y > 64) break;
  }
  display.display();
  SPI.begin();
  radio.powerUp();
  delay(5);
  radio.startConstCarrier(RF24_PA_MAX, i);
}

void displayFullScreenBitmap() {
  for (size_t i = 0; i < 3; i++) {
    radio.powerDown();
    SPI.end();
    delay(10);
    display.clearDisplay();
    drawScaledBitmap(0, 0, 128, 64, epd_bitmap_image02);
    display.display();
    Serial.println("Displaying full-screen bitmap");
    SPI.begin();
    radio.powerUp();
    delay(5);
    radio.startConstCarrier(RF24_PA_MAX, i);
    delay(310);
    radio.powerDown();
    SPI.end();
    delay(10);
    display.clearDisplay();
    display.display();
    Serial.println("Clearing display");
    SPI.begin();
    radio.powerUp();
    delay(5);
    radio.startConstCarrier(RF24_PA_MAX, i);
    delay(300);
  }
}

void addvertising() {
  for (size_t i = 0; i < 3; i++) {
    displayMessage("", 65, 22, epd_bitmap_image02);
    Serial.println("Advertising: Showing bitmap");
    delay(310);
    displayMessage("", 65, 22, nullptr);
    Serial.println("Advertising: Clearing bitmap");
    delay(300);
  }
  displayMessage("Click the button and change modes!", 65, 6, epd_bitmap_image02);
  Serial.println("Advertising: Showing 'Click the button' message");
}

void setup() {
  Serial.begin(9600);
  button.setDebounceTime(100);
  pinMode(BTN_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Initializing OLED...");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED screen not found!"));
    while (true);
  }
  Serial.println("OLED initialized successfully");
  // Blink full-screen bitmap three times
  displayFullScreenBitmap();
  Serial.println("Attempting to initialize radio...");
  if (radio.begin()) {
    Serial.println("Radio initialized successfully!");
    delay(200);
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPayloadSize(5);
    radio.setAddressWidth(3);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.printPrettyDetails();
    radio.startConstCarrier(RF24_PA_MAX, i);
    addvertising();
  } else {
    Serial.println("Radio initialization failed!");
    displayMessage("Jammer Error!", 65, 22, epd_bitmap_image02);
  }
}

void fullAttack() {
  for (size_t i = 0; i < 80; i++) {
    radio.setChannel(i);
  }
}

void wifiAttack() {
  for (int i = 0; i < sizeof(wifiFrequencies) / sizeof(wifiFrequencies[0]); i++) {
    radio.setChannel(wifiFrequencies[i] - 2400);
  }
}

const char* modes[] = {
  "All 2.4 GHz",
  "Wi-Fi only ",
  "Waiting "
};
uint8_t attack_type = 2;

void loop() {
  button.loop();
  if (button.isPressed()) {
    attack_type = (attack_type + 1) % 3;
    String modeText = String(modes[attack_type]) + " Mode";
    displayMessage(modeText.c_str(), 65, 22, epd_bitmap_image02);
    Serial.println("Mode changed to: " + modeText);
  }
  switch (attack_type) {
    case 0: fullAttack(); break;
    case 1: wifiAttack(); break;
    case 2: break;
  }
}