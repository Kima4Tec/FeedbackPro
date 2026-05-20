# Smiley Feedback Konsol – IoT Projekt
## Logbog
### Dag 1
Opstart og genopfriskning af IoT. En knap, der tænder en ledlampe.

### Dag 2
Udvidelse af board til fire knapper og fire leds, der tænder ift det tilhørende knaptryk.

Kode til fire knapper
```
#include "Arduino.h"
#include "driver/rtc_io.h"

#define BIT(x) (1ULL << x)

// Buttons - using stabil RTC GPIOs
#define BTN1 GPIO_NUM_25
#define BTN2 GPIO_NUM_32
#define BTN3 GPIO_NUM_33
#define BTN4 GPIO_NUM_27

// LEDs
#define LED1 2
#define LED2 4
#define LED3 18
#define LED4 19

uint64_t bitmask =
  BIT(BTN1) |
  BIT(BTN2) |
  BIT(BTN3) |
  BIT(BTN4);

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  digitalWrite(LED4, LOW);

  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();

  if (reason == ESP_SLEEP_WAKEUP_EXT1) {

    uint64_t wakePins = esp_sleep_get_ext1_wakeup_status();

    if (wakePins & BIT(BTN1)) digitalWrite(LED1, HIGH);
    if (wakePins & BIT(BTN2)) digitalWrite(LED2, HIGH);
    if (wakePins & BIT(BTN3)) digitalWrite(LED3, HIGH);
    if (wakePins & BIT(BTN4)) digitalWrite(LED4, HIGH);
  }

  // Stabil input
  rtc_gpio_pulldown_en(BTN1);
  rtc_gpio_pulldown_en(BTN2);
  rtc_gpio_pulldown_en(BTN3);
  rtc_gpio_pulldown_en(BTN4);

  rtc_gpio_pullup_dis(BTN1);
  rtc_gpio_pullup_dis(BTN2);
  rtc_gpio_pullup_dis(BTN3);
  rtc_gpio_pullup_dis(BTN4);

  esp_sleep_enable_ext1_wakeup(bitmask, ESP_EXT1_WAKEUP_ANY_HIGH);

  Serial.println("Going to sleep...");
  delay(1000);

  esp_deep_sleep_start();
}

void loop() {}
```
### Dag 3
Vi arbejdede med MQTT, Wifi og sendte først beskeder, for dernæst at registrere knaptryk og sende trykinfo til mqtt. 
Vi startede med en wifi testkode, der blev udvidet med NTP og senere MQTT. Konstanter med ssid og pw er fjernet. De var hardcodede
i main.cpp, men senere lagde vi dem i en secrets.h, som vi tilføjede gitignore, så den ikke blev tilføjet github repo.
Vi installerede et program: MQTT explorer, så vi kunne læse vores beskeder. Programmet kan man bruge til at forbinde sig til MQTT-serveren
og sende beskeder i fx raw eller json. 
Her er første kode.
```
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

//const with ssid and pw are removed, but was here

// ===================== CLIENT =====================
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ===================== TIME =====================
void setTimezone(String timezone) {
  Serial.printf("Setting Timezone: %s\n", timezone.c_str());
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

// ===================== MQTT CALLBACK =====================
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message on topic: ");
  Serial.println(topic);

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message: ");
  Serial.println(message);
}

// ===================== WIFI =====================
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
}

// ===================== MQTT RECONNECT =====================
void reconnectMQTT() {
  while (!client.connected()) {

    Serial.print("Connecting MQTT...");

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");

      client.subscribe("iot/test");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 5 sec");

      delay(5000);
    }
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(1000);

  initWiFi();

  // 🔐 TLS TEST MODE (virker uden cert)
  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  initTime("CET-1CEST,M3.5.0,M10.5.0/3");
  printLocalTime();
}

// ===================== LOOP =====================
void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  static unsigned long lastMsg = 0;

  if (millis() - lastMsg > 5000) {
    lastMsg = millis();

    String message = "Hej fra Malthe, Theis og Kim på ESP32! Klokken er: " + String(millis() / 1000) + " sekunder siden opstart.";
    client.publish("/devices/device03/TopGroup", message.c_str());

    Serial.println("MQTT message sent");
  }
}
```

Vi flettede de to programmer sammen, så knaptryk tændte led og samtidig og sender besked til MQTT-serveren, hvilken knap er trykket. 
Sidst har vi tilføjet en registrering af antallet af de forskellige knaptryk, så de bliver talt og sendt til MQTT-serveren. 
Ved at gemme knap-tællerne i RTC_DATA_ATTR variabler overlever de deep sleep. 

Her er den afsluttede kode:
```
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
```






# Opgaven:
## Motivation

Smiley feedback konsollen er en sjov og dagligdags anordning, som vi genkender fra oplevelser i banken, borgerservice, varehuse og sundhedscentre.

Anordningen kan implementeres uden alt for store udfordringer og alligevel tilføjes kompleksitet i forbindelse med dataoverførsel og strømbesparelse. Derfor er det et passende intro-projekt til faget IoT og embeddede systemers anden del på H4.

---

# Produktkrav

- Anordningen har 4 knapper med tilhørende smileys.
- Der er respons på LED ved hver knap.
- På knapperne implementeres debounce-funktionalitet.
- Efter tryk på en knap er anordningen låst i 7 sekunder, før ny feedback modtages.
- Lysdioden ud for den trykkede knap lyser i alle 7 sekunder.
- Anordningen tilsluttes internet via Wi-Fi access point i undervisningslokalet.
- Anordningens ur synkroniseres løbende med NTP og korrekt tidszone.
- Knaptryk-data (værdi og timestamp) overføres via MQTT.
- Der bruges brugernavn og password, og hele kommunikationen sikres ved brug af TLS-kryptering.
- Anordningen designes strømbesparende ved brug af Deep Sleep og batteridrift.
- Anordningen skal spare strøm ved at skifte til Deep Sleep, når den er ubeskæftiget i et stykke tid.
- Eksperimenter og analyser skal vise de bedste tidsgrænser for:
  - hvor længe anordningen er ubeskæftiget, men aktiv
  - hvordan opvågning skal fungere, så anordningen opdager aktivitet og falder i søvn igen
- En god analyse og overvejelse danner grundlag for, hvor længe anordningen sover, inden den vågner rutinemæssigt.

---

# Proceskrav

- Opgaven løses i grupper på 2 personer.
- Eneste undtagelse er ved ulige antal elever, hvor der dannes en gruppe på 3 personer.
- Læreren tager initiativ og beslutter, hvem der deltager i gruppen på tre medlemmer.

## Versionsstyring

- Under hele projektforløbet skal versionsstyringssystemet Git bruges til at håndtere og registrere projektets filer.
- Alle gruppemedlemmer skal dagligt lave commits til repo’et.
- Det anbefales at repo’et pushes til GitHub, GitLab eller lignende.
- Læreren skal tilføjes til brugergruppen med adgang til:
  - hele repo’et
  - commit-historik
  - branches
  - øvrigt projektindhold

## Logbog

- Gruppen fører løbende en logbog.
- Alle commits skal dokumenteres mindst én gang om dagen.
- Repo’et skal derfor indeholde commit af dagbogen fra alle medlemmer hver dag.

## Arbejdsportfolio

Gruppen opbygger et arbejdsportfolio, hvor alle idéer og beslutninger beskrives:

- fra idé
- via skitse
- til færdig løsning

Alle bidrager til indholdet.

Arbejdsportfolien opdateres løbende, når der er relevant indhold at tilføje.

Indholdet kan blandt andet være:

- tekst
- skitser
- screenshots
- links
- noter
- inspiration

Både logbog og arbejdsportfolio skal committes til Git-repo’et, så progressionen dokumenteres.

## Dokumentation

Dokumentformatet Markdown er meget velegnet til detaljeret versionsstyring i Git.

## Samarbejde

Gruppen og læreren taler løbende sammen om:

- udfordringer
- fremdrift
- læring
- fremskridt

---

# Bedømmelse

Opgavebesvarelsen indgår som en del af den samlede bedømmelse for faget, proportionalt med den tid der er brugt til projektarbejdet.

Produktkravene og proceskravene prioriteres lige højt, fordi:

- produktet alene ikke afslører deltagerens viden om løsningen
- processen alene ikke nødvendigvis fører til en fungerende løsning

Begge dele skal mindst opnå karakteren **02** hver for sig for samlet at bestå.

En præstation over karakteren 12 i den ene del kan ikke kompensere for en præstation under 02 i den anden del.

## Vigtige vurderingspunkter

En løsning som:

- mangler TLS-kryptering
- mangler strømbesparelseselementer

kan ikke opnå karakteren **12** for produktkravene.

## Karakteren 12

Karakteren 12 gives, når:

- alle mål opfyldes
- løsningen kun har ubetydelige fejl eller mangler

Eksempler på mindre fejl kan være:

- periodisk usikker debounce
- usikker håndtering af tidszone
- mindre fejl i Deep Sleep-funktioner
- enkelte manglende begivenheder i logbogen
- mangelfulde beskrivelser i portfoliet

---

# Info

For information om den lokale MQTT-server, se:

- Lokal MQTT server

Her findes også eksempler på kode, som sender beskeder med MQTT.
