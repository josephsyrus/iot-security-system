#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include "time.h"

// ===== WIFI & Firebase =====
const char* ssid = "AndroidAPDBF9";
const char* password = "123456787654321";
const char* firebaseURL = "https://iot-security-system-dadf2-default-rtdb.asia-southeast1.firebasedatabase.app/logs.json";

// ===== NTP Settings =====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; 
const int daylightOffset_sec = 0;

// ===== HC-SR04 Pins =====
const int trigPin = 5; 
const int echoPin = 4; 

// ===== Passive Buzzer =====
const int buzzerPin = 33; 

// ===== RFID =====
#define RST_PIN 22
#define SS_PIN 21
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ===== LCD 16x2 =====
LiquidCrystal lcd(13, 12, 14, 27, 26, 25); // RS, E, D4, D5, D6, D7

// ===== Authorized Cards =====
struct User {
  String uid;
  String name;
};
User authorizedUsers[] = {
  {"43:E7:D7:0B", "Joseph"},
  {"13:2A:D9:46", "Jim"}
};
const int userCount = sizeof(authorizedUsers)/sizeof(authorizedUsers[0]);

// ===== Timing =====
const int scanWindowSec = 5; 

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("IoT Security");
  lcd.setCursor(0,1);
  lcd.print("System Ready");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Setup NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

// ===== Loop =====
void loop() {
  lcd.setCursor(0,0);
  lcd.print("Monitoring       ");
  lcd.setCursor(0,1);
  lcd.print("Distance sensor  ");

  // Check presence with ultrasonic
  long duration, distance;
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH, 30000);
  distance = duration * 0.034 / 2;

  if(distance > 0 && distance < 30){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Presence Detected");

    unsigned long startTime = millis();
    bool cardDetected = false;
    String uid = "";
    String scannedName = "";
    bool fullPresence = true;

    while(millis() - startTime < scanWindowSec * 1000UL){
      int secondsLeft = scanWindowSec - ((millis() - startTime)/1000);
      lcd.setCursor(0,1);
      lcd.print("Scan Card [");
      lcd.print(secondsLeft);
      lcd.print("s]  ");

      digitalWrite(trigPin, LOW); delayMicroseconds(2);
      digitalWrite(trigPin, HIGH); delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      duration = pulseIn(echoPin, HIGH, 30000);
      distance = duration * 0.034 / 2;
      if(distance <= 0 || distance > 30){
        fullPresence = false;
        break; 
      }

      uid = readUID();
      if(uid != ""){
        scannedName = "Unknown";
        for(int i=0;i<userCount;i++){
          if(uid == authorizedUsers[i].uid){
            scannedName = authorizedUsers[i].name;
            break;
          }
        }
        cardDetected = true;
        break;
      }
      delay(200);
    }

    String event = (cardDetected ? "Access Granted" : (fullPresence ? "Intruder Alert" : "No Alert"));

    lcd.clear();
    lcd.setCursor(0,0);
    if(event == "Access Granted") lcd.print("Access Granted");
    else if(event == "Intruder Alert") lcd.print("INTRUDER ALERT!");
    else lcd.print("Presence Gone");
    lcd.setCursor(0,1);
    if(cardDetected) lcd.print(scannedName);


    if(cardDetected){
      tone(buzzerPin, 2000, 200);
      delay(200);
    } 
    else if(fullPresence){
      unsigned long intruderStart = millis();
      while(millis() - intruderStart < 5000UL){
        tone(buzzerPin, 2000, 500);
        delay(500);
        noTone(buzzerPin);
        delay(500);
      }
    }

    if(event != "No Alert"){
      String timestamp = getCurrentTimestamp();
      String payload = "{\"timestamp\":\"" + timestamp + "\",\"card_uid\":\"" + uid + "\",\"name\":\"" + scannedName + "\",\"event\":\"" + event + "\"}";
      sendToFirebase(payload);
    }

    delay(1500);
  }

  delay(200);
}

// ===== Functions =====
String readUID() {
  if (!mfrc522.PICC_IsNewCardPresent()) return "";
  if (!mfrc522.PICC_ReadCardSerial()) return "";

  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i != mfrc522.uid.size - 1) uid += ":";
  }
  uid.toUpperCase();
  return uid;
}

String getCurrentTimestamp() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to get NTP time");
    return "0";
  }
  time_t now = mktime(&timeinfo);
  return String(now * 1000UL);
}

void sendToFirebase(String payload){
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin(firebaseURL);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(payload);
    if(httpResponseCode > 0){
      Serial.printf("Firebase POST code: %d\n", httpResponseCode);
    } else {
      Serial.printf("Error sending to Firebase: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi not connected, cannot send data");
  }
}