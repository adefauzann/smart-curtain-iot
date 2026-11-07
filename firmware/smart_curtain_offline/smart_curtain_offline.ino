#include <Stepper.h>
#include <DHT.h>

// Konfigurasi Stepper Motor
const int stepsPerRevolution = 200;  // Jumlah langkah per revolusi motor
Stepper motor1(stepsPerRevolution, 13, 14, 12, 27); 
Stepper motor2(stepsPerRevolution, 25, 32, 26, 33);

int stepCount = 6500;     // Jumlah langkah untuk pergerakan motor
int LDRPin = 34;         // Pin untuk LDR sensor (GPIO34 pada ESP32)
bool movedRight = false; // Untuk melacak apakah motor sudah bergerak ke kanan
bool movedLeft = false;  // Untuk melacak apakah motor sudah bergerak ke kiri

// Konfigurasi DHT11 Sensor
#define DHTPIN 4         // Pin untuk DHT11 sensor (misalnya GPIO4)
#define DHTTYPE DHT11    // DHT 11
DHT dht(DHTPIN, DHTTYPE);  // Inisialisasi sensor DHT11

// Konfigurasi Relay
int relayPin = 5;        // Pin untuk relay (misalnya GPIO5)

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
}

void loop() {
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
    
    // Kontrol relay berdasarkan suhu
    if (temperature > 29) {
      digitalWrite(relayPin, HIGH);  // Aktifkan relay
      Serial.println("Relay aktif (suhu di atas 29°C)");
    } else {
      digitalWrite(relayPin, LOW);   // Matikan relay
      Serial.println("Relay mati (suhu di bawah 29°C)");
    }
  }

  // Cetak nilai LDR
  Serial.print("LDR Value: ");
  Serial.println(LDRValue);

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
  }

  // Delay singkat sebelum iterasi berikutnya
  delay(100);
}
