
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define PIR_PIN   27
#define SERVO_PIN 18
#define DHT_PIN   4

const char* WIFI_ADI   = "...";
const char* WIFI_SIFRE = "asafdemir";

WebServer server(80);
Servo arm;
DHT dht(DHT_PIN, DHT11);

float nem = 0, sic = 0;
int sayac = 0;
unsigned long sonKovma = 0;

void kov() {
  Serial.println("Servo calisiyor");
  arm.attach(SERVO_PIN);
  for (int tur = 0; tur < 3; tur++) {
    for (int a = 10; a <= 150; a += 2) { arm.write(a); delay(10); }
    for (int a = 150; a >= 10; a -= 2) { arm.write(a); delay(10); }
  }
  arm.detach();
  Serial.println(">> Servo bitti");
}

// Panel başka yerden açıldığı için tarayıcıya izin başlığı ekliyoruz
void gonder(int kod, const char* tip, String icerik) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(kod, tip, icerik);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== Kus Kovucu basliyor ===");

  pinMode(PIR_PIN, INPUT);
  dht.begin();
  arm.attach(SERVO_PIN); arm.write(10); delay(500); arm.detach();

  // Önce kendi Wi-Fi ağına bağlanmayı dene (en fazla 15 sn)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_ADI, WIFI_SIFRE);
  Serial.print("Wi-Fi'ye baglaniyor");
  int deneme = 0;
  while (WiFi.status() != WL_CONNECTED && deneme < 30) {
    delay(500);
    Serial.print(".");
    deneme++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi baglandi");
    Serial.print("Panel adresi (IP): ");
    Serial.println(WiFi.localIP());          // panele bu IP'yi yazin
  } 

  server.on("/", []() { gonder(200, "text/plain", "Kus Kovucu calisiyor"); });
  server.on("/data", []() {
    gonder(200, "application/json",
      "{\"h\":" + String(nem, 0) + ",\"t\":" + String(sic, 1) +
      ",\"n\":" + String(sayac) + "}");
  });
  server.on("/test", []() {
    Serial.println("Panelden test komutu geldi");
    kov();
    gonder(200, "text/plain", "ok");
  });
  server.begin();
  Serial.println("Web sunucu basladi");
}

void loop() {
  server.handleClient();

  // nem ve sıcaklık (2 sn'de bir)
  static unsigned long sonDht = 0;
  if (millis() - sonDht > 2000) {
    float h = dht.readHumidity(), t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("DHT11 okunamadi! Baglantiyi kontrol edin");
    } else {
      nem = h;
      sic = t;
      Serial.print("Nem: %");
      Serial.print(nem, 0);
      Serial.print("  Sicaklik: ");
      Serial.print(sic, 1);
      Serial.print(" C  PIR: ");
      Serial.print(digitalRead(PIR_PIN) ? "HAREKET" : "yok");
      Serial.print("  Kus sayisi: ");
      Serial.println(sayac);
    }
    sonDht = millis();
  }

  // kuş algılama (30 sn bekleme)
  if (digitalRead(PIR_PIN) == HIGH && (sonKovma == 0 || millis() - sonKovma > 30000)) {
    sayac++;
    sonKovma = millis();
    Serial.print("KUS ALGILANDI! Toplam: ");
    Serial.println(sayac);
    kov();
  }
}
