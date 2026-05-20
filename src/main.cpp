#include "Arduino.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"
#include "ca_cert.h"
#include <ArduinoJson.h>

#define BIT(x) (1ULL << x)

// ===================== BUTTONS =====================
#define BTNRED    GPIO_NUM_25 
#define BTNGREEN  GPIO_NUM_32 
#define BTNYELLOW GPIO_NUM_33 
#define BTNBLUE   GPIO_NUM_35  

// ===================== LEDS =====================
#define LEDRED    2
#define LEDYELLOW 4
#define LEDGREEN  18
#define LEDBLUE   19

// ===================== MQTT & WIFI & tls=====================
static WiFiClientSecure tlsClient;
static PubSubClient     mqttClient(tlsClient);

uint64_t bitmask =
  BIT(BTNBLUE)   |
  BIT(BTNGREEN)  |
  BIT(BTNYELLOW) |
  BIT(BTNRED);

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int countRed    = 0;
RTC_DATA_ATTR int countYellow = 0;
RTC_DATA_ATTR int countGreen  = 0;
RTC_DATA_ATTR int countBlue   = 0;

// ===================== WIFI =====================
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, WIFIPASSWORD);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
}

// ===================== MQTT RECONNECT =====================
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry in 5 sec");
      delay(5000);
    }
  }
}

void print_GPIO_wake_up(uint64_t wakePins) {
  const char* smiley = "";
  if (wakePins & BIT(BTNRED))    { smiley = "Meget sur 😠";  countRed++;    }
  if (wakePins & BIT(BTNYELLOW)) { smiley = "Sur 🙁";        countYellow++; }
  if (wakePins & BIT(BTNGREEN))  { smiley = "Glad 🙂";       countGreen++;  }
  if (wakePins & BIT(BTNBLUE))   { smiley = "Meget glad 😄"; countBlue++;   }

  struct tm timeinfo;
  char timestamp[30] = "unknown";
  if (getLocalTime(&timeinfo))
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);

  JsonDocument doc;
  doc["button"]      = smiley;
  doc["timestamp"]   = timestamp;
  doc["bootCount"]   = bootCount;
  doc["countRed"]    = countRed;
  doc["countYellow"] = countYellow;
  doc["countGreen"]  = countGreen;
  doc["countBlue"]   = countBlue;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  mqttClient.publish("/devices/device03/GroupKMT", jsonBuffer);
  Serial.println(jsonBuffer);
}

// ===================== TIME =====================
void setTimezone(String timezone) {
  setenv("TZ", timezone.c_str(), 1);
  tzset();
}

void initTime(String timezone) {
  struct tm timeinfo;
  Serial.println("Setting up NTP time...");
  configTime(0, 0, "pool.ntp.org");
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time");
    return;
  }
  Serial.println("Time synced");
  setTimezone(timezone);
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(200);

  // LEDs
  pinMode(LEDRED,    OUTPUT);
  pinMode(LEDYELLOW, OUTPUT);
  pinMode(LEDGREEN,  OUTPUT);
  pinMode(LEDBLUE,   OUTPUT);

  digitalWrite(LEDRED,    LOW);
  digitalWrite(LEDYELLOW, LOW);
  digitalWrite(LEDGREEN,  LOW);
  digitalWrite(LEDBLUE,   LOW);

  bootCount++;
  Serial.println("Boot: " + String(bootCount));

  initWiFi();

  tlsClient.setCACert(MQTT_CA_CERT);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  reconnectMQTT();

  initTime("CET-1CEST,M3.5.0,M10.5.0/3");
  printLocalTime();

  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();

  if (reason == ESP_SLEEP_WAKEUP_EXT1) {
    uint64_t wakePins = esp_sleep_get_ext1_wakeup_status();

    // Tænd LED
    if (wakePins & BIT(BTNRED))    digitalWrite(LEDRED,    HIGH);
    if (wakePins & BIT(BTNGREEN))  digitalWrite(LEDGREEN,  HIGH);
    if (wakePins & BIT(BTNYELLOW)) digitalWrite(LEDYELLOW, HIGH);
    if (wakePins & BIT(BTNBLUE))   digitalWrite(LEDBLUE,   HIGH);

    // Publish til MQTT
    print_GPIO_wake_up(wakePins);
    delay(200);  // Giv MQTT tid til at sende inden sleep
  }

  // Pull-down (vigtigt for EXT1)
  rtc_gpio_pulldown_en(BTNRED);
  rtc_gpio_pulldown_en(BTNGREEN);
  rtc_gpio_pulldown_en(BTNYELLOW);
  rtc_gpio_pulldown_en(BTNBLUE);

  rtc_gpio_pullup_dis(BTNRED);
  rtc_gpio_pullup_dis(BTNGREEN);
  rtc_gpio_pullup_dis(BTNYELLOW);
  rtc_gpio_pullup_dis(BTNBLUE);

  esp_sleep_enable_ext1_wakeup(bitmask, ESP_EXT1_WAKEUP_ANY_HIGH);

  Serial.println("Going to sleep...");
  delay(200);
  esp_deep_sleep_start();
}

void loop() {}