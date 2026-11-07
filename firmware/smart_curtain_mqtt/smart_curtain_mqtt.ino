#include <Stepper.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Konfigurasi Stepper Motor
const int stepsPerRevolution = 150;  // Jumlah langkah per revolusi motor
Stepper motor1(stepsPerRevolution, 13, 14, 12, 27); 
Stepper motor2(stepsPerRevolution, 25, 32, 26, 33);

int stepCount = 8000;     // Jumlah langkah untuk pergerakan motor
int LDRPin = 34;         // Pin untuk LDR sensor (GPIO34 pada ESP32)
bool movedRight = false; // Untuk melacak apakah motor sudah bergerak ke kanan
bool movedLeft = false;  // Untuk melacak apakah motor sudah bergerak ke kiri

// Konfigurasi DHT11 Sensor
#define DHTPIN 4         // Pin untuk DHT11 sensor (misalnya GPIO4)
#define DHTTYPE DHT11    // DHT 11
DHT dht(DHTPIN, DHTTYPE);  // Inisialisasi sensor DHT11

// Konfigurasi Relay
int relayPin = 5;        // Pin untuk relay (misalnya GPIO5)

// Konfigurasi Wi-Fi dan MQTT
const char* ssid = "YOUR_WIFI_SSID";           // SSID Wi-Fi Anda
const char* password = "YOUR_WIFI_PASSWORD";   // Password Wi-Fi Anda
const char* mqtt_server = "broker.mqtt-dashboard.com";    // IP Address atau URL broker MQTT (misal: Node-RED)

// Inisialisasi objek Wi-Fi dan MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Fungsi callback untuk menerima pesan MQTT
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Pesan diterima di topik: ");
  Serial.print(topic);
  Serial.print(". Pesan: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

// Fungsi untuk menghubungkan kembali ke MQTT broker
void reconnect() {
  // Loop sampai terhubung kembali
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Terhubung");
      client.subscribe("/esp32/mqtt/in");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 3 detik");
      delay(3000);
    }
  }
}

void setup() {
  // Inisialisasi pin LDR sebagai input
  pinMode(LDRPin, INPUT);

  // Inisialisasi pin relay sebagai output
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // Matikan relay pada awal

  // Mulai komunikasi serial
  Serial.begin(115200);

  // Inisialisasi DHT sensor
  dht.begin();

  // Inisialisasi Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi terhubung");

  // Menghubungkan ke broker MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // Memastikan koneksi ke broker MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Membaca nilai dari sensor LDR
  int LDRValue = analogRead(LDRPin);

  // Membaca suhu dan kelembapan dari DHT11
  float humidity = dht.readHumidity();    // Membaca kelembapan
  float temperature = dht.readTemperature();  // Membaca suhu dalam Celsius

  // Indikator Relay dan Motor
  String relayState = (digitalRead(relayPin) == HIGH) ? "ON" : "OFF";
  String tiraiState = (movedRight) ? "Terbuka" : "Tertutup";

  // Cek apakah sensor DHT11 memberikan pembacaan yang valid
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Gagal membaca dari DHT sensor!");
  } else {
    Serial.print("Kelembapan: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Suhu: ");
    Serial.print(temperature);
    Serial.println(" *C ");
    
    // Publikasikan data DHT dan relay ke MQTT
    String tempStr = "Suhu: " + String(temperature) + " *C";
    String humStr = "Kelembapan: " + String(humidity) + " %";
    client.publish("sensor/temperature", tempStr.c_str());
    client.publish("sensor/humidity", humStr.c_str());
    
    // Kontrol relay berdasarkan suhu
    if (temperature > 34) {
      digitalWrite(relayPin, HIGH);  // Aktifkan relay
      Serial.println("Relay aktif (suhu di atas 34°C)");
    } else {
      digitalWrite(relayPin, LOW);   // Matikan relay
      Serial.println("Relay mati (suhu di bawah 34°C)");
    }
  }

  // Cetak nilai LDR dan publikasikan ke MQTT
  Serial.print("LDR Value: ");
  Serial.println(LDRValue);
  String ldrStr = "LDR: " + String(LDRValue);
  client.publish("sensor/ldr", ldrStr.c_str());

  // Kontrol tirai berdasarkan nilai LDR
  if (LDRValue > 2000 && !movedRight) {
    motor1.setSpeed(60);  // Set kecepatan motor 1 (RPM)
    motor2.setSpeed(60);  // Set kecepatan motor 2 (RPM)

    for (int i = 0; i < stepCount; i++) {
      motor1.step(2);  // Langkah maju untuk motor 1
      motor2.step(2);  // Langkah maju untuk motor 2
    }
    movedRight = true;
    movedLeft = false;
    Serial.println("Motor bergerak ke kanan (Tirai Terbuka)");

    // Publikasi indikator tirai ke MQTT
    client.publish("tirai/status", "Tirai Terbuka");
  } else if (LDRValue <= 1000 && !movedLeft) {
    motor1.setSpeed(60);
    motor2.setSpeed(60);

    for (int i = 0; i < stepCount; i++) {
      motor1.step(-2);
      motor2.step(-2);
    }
    movedLeft = true;
    movedRight = false;
    Serial.println("Motor bergerak ke kiri (Tirai Tertutup)");

    // Publikasi indikator tirai ke MQTT
    client.publish("tirai/status", "Tirai Tertutup");
  }

  // Publikasikan status relay dan tirai ke MQTT
  client.publish("relay/status", relayState.c_str());
  client.publish("tirai/status", tiraiState.c_str());

  // Delay singkat sebelum iterasi berikutnya
  delay(500);
}
