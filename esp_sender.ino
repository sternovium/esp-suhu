#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// --- KONFIGURASI WI‑FI ---
const char* WIFI_SSID      = "GRIYA RARA";
const char* WIFI_PASSWORD  = "10042025";

// --- KONFIGURASI DHT22 ---
#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- KONFIGURASI LCD I2C ---
// Ganti alamat I2C jika lain (cek dengan I2C scanner; umumnya 0x27 atau 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2); // lcd(alamat, kolom, baris)

// --- KONFIGURASI SUPABASE ---
const char* SUPABASE_URL = "https://kgarcjpjtygipoaxaoar.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtnYXJjanBqdHlnaXBvYXhhb2FyIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzU1MzM5NDcsImV4cCI6MjA5MTEwOTk0N30.DBLxZWZFtI6uJeJDukU71wM5S4PQb4syaQrLINr1m8k";
const char* TABLE_NAME   = "sensors";

// --- fungsi bantu: konversi float ke string tanpa float print glitch ---
String floatToString(float value, int decimalPlaces = 1) {
  return String(value, decimalPlaces);
}

void setup() {
  Serial.begin(115200);

  // --- 1. Inisialisasi LCD I2C ---
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  // --- 2. Inisialisasi DHT22 ---
  dht.begin();

  // --- 3. Konek WiFi ---
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi OK");
}

void loop() {
  // --- 1. Baca DHT22 ---
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Celsius

  if (isnan(h) || isnan(t)) {
    Serial.println("Gagal baca DHT22");
    delay(5000);
    return;
  }

  // --- 2. Tampilkan ke LCD I2C ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(floatToString(t) + "C");

  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(floatToString(h) + "%");

  // --- 3. Kirim ke Supabase via REST API ---
  HTTPClient http;
  String url = String(SUPABASE_URL) + "/rest/v1/" + TABLE_NAME;

  http.begin(url);
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");

  // JSON payload (sesuaikan dengan nama kolom di tabel)
  String payload = "{\"temperature\":" + String(t) +
                   ",\"humidity\":" + String(h) +
                   ",\"recorded_at\":\"now()\"}";

  Serial.println("Mengirim ke Supabase:");
  Serial.println(payload);

  int httpCode = http.POST(payload);

  if (httpCode == 201) {
    Serial.println("Sukses kirim data ke Supabase");
  } else {
    Serial.printf("Gagal, HTTP code: %d\n", httpCode);
    Serial.println(http.getString());
  }

  http.end();

  // --- 4. Delay sebelum loop berikutnya ---
  delay(15000); // 15 detik, bisa diubah sesuai kebutuhan
}