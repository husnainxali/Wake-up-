// Simple alarm clock - XIAO ESP32-C3
// Shows the time (from WiFi), has one alarm, beeps when it's time.
//
// KEYS:  SW1 = alarm hour +      SW2 = alarm minute +
//        SW5 = alarm on/off      SW6 = stop the beeping

#include <WiFi.h>
#include <SPI.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ---------- 1. YOUR SETTINGS ----------
const char* WIFI_NAME = "YOUR_WIFI_NAME";      // must be 2.4 GHz
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ---------- 2. PINS (from your schematic) ----------
#define TFT_CS   D6
#define TFT_DC   D5
#define TFT_RST  D8
#define TFT_BL   D7
#define TFT_SCK  D9
#define TFT_MOSI D10
#define BUZZER   D3

const int rowPins[2] = {D0, D1};   // R1, R2
const int colPins[2] = {D2, D4};   // C1, C3  (C2 is skipped: the buzzer uses that pin)

// Which key is at [row][column]:  R1/C1 = SW1, R1/C3 = SW5, R2/C1 = SW2, R2/C3 = SW6
const int keyMap[2][2] = { {1, 5},
                           {2, 6} };

Adafruit_ST7789 tft(&SPI, TFT_CS, TFT_DC, TFT_RST);

// ---------- 3. VARIABLES ----------
int  alarmHour = 7;
int  alarmMin  = 0;
bool alarmOn   = false;
bool ringing   = false;

int  lastKey = 0;
int  lastSecond = -1;
bool beepState = false;
unsigned long lastBeep = 0;

// ---------- 4. READ THE KEYS ----------
// Pull one column LOW at a time. If a key is pressed, its row reads LOW.
// Returns the key number (1, 2, 5 or 6), or 0 if nothing is pressed.
int readKey() {
  for (int c = 0; c < 2; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], LOW);
    delayMicroseconds(50);
    for (int r = 0; r < 2; r++) {
      if (digitalRead(rowPins[r]) == LOW) {
        pinMode(colPins[c], INPUT);
        return keyMap[r][c];
      }
    }
    pinMode(colPins[c], INPUT);
  }
  return 0;
}

// ---------- 5. DRAW ON THE SCREEN ----------
void drawTime(struct tm t) {
  char text[16];
  snprintf(text, sizeof(text), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  tft.setTextSize(4);
  tft.setCursor(24, 25);
  tft.print(text);
}

void drawAlarm() {
  char text[24];
  snprintf(text, sizeof(text), "ALARM %02d:%02d %s", alarmHour, alarmMin, alarmOn ? "ON " : "OFF");
  tft.setTextSize(2);
  tft.setCursor(24, 90);
  tft.print(text);
}

// ---------- 6. WHAT EACH KEY DOES ----------
void handleKey(int key) {
  if (key == 1) alarmHour = (alarmHour + 1) % 24;
  if (key == 2) alarmMin  = (alarmMin + 1) % 60;
  if (key == 5) alarmOn   = !alarmOn;
  if (key == 6) { ringing = false; noTone(BUZZER); }
  drawAlarm();
}

// ---------- 7. SETUP (runs once) ----------
void setup() {
  // keys
  pinMode(rowPins[0], INPUT_PULLUP);
  pinMode(rowPins[1], INPUT_PULLUP);
  pinMode(colPins[0], INPUT);
  pinMode(colPins[1], INPUT);

  // screen
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);                 // backlight on
  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.init(135, 240);                         // your display size
  tft.setRotation(1);                         // use 3 if it is upside down
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);   // green text, black background
  tft.setTextSize(2);
  tft.setCursor(10, 50);
  tft.print("Connecting WiFi...");

  // WiFi + time from the internet (Pakistan = UTC+5)
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(300);
  configTzTime("PKT-5", "pool.ntp.org");

  struct tm t;
  while (!getLocalTime(&t)) { }               // wait until the time arrives

  tft.fillScreen(ST77XX_BLACK);
  drawAlarm();
}

// ---------- 8. LOOP (runs forever) ----------
void loop() {
  // keys: act only once when a key goes down
  int key = readKey();
  if (key != lastKey) {
    lastKey = key;
    if (key != 0) handleKey(key);
  }

  // time: redraw once per second, and check the alarm
  struct tm t;
  if (getLocalTime(&t, 0)) {
    if (t.tm_sec != lastSecond) {
      lastSecond = t.tm_sec;
      drawTime(t);
    }
    if (alarmOn && !ringing && t.tm_hour == alarmHour && t.tm_min == alarmMin && t.tm_sec == 0) {
      ringing = true;
    }
  }

  // beeping: on for half a second, off for half a second
  if (ringing && millis() - lastBeep > 500) {
    lastBeep = millis();
    beepState = !beepState;
    if (beepState) tone(BUZZER, 2000);
    else           noTone(BUZZER);
  }

  delay(20);   // also works as key debounce
}
