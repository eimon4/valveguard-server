#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ===== WiFi & API Settings =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Replace with your Node.js server IP and Port (e.g. http://192.168.1.5:3000)
// If hosted online, use the domain name (e.g. https://my-valveguard.onrender.com)
const String SERVER_URL = "http://YOUR_SERVER_IP:3000";

// Replace with the User ID shown in the ValveGuard Dashboard!
const String USER_ID = "YOUR_USER_ID_HERE";

// ===== GSM Settings =====
#define RXD2 16
#define TXD2 17
// We change this to a String so it can be updated dynamically
String phoneNumber = "+639685698288"; // Fallback number

// ===== Pins =====
const int servoPin = 18;
const int buzzerPin = 19;
const int mq2Pin = 34;

const int gasThreshold = 1200;

// ===== Objects =====
Servo myservo;

// ===== Variables =====
unsigned long servoStart = 0;
unsigned long buzzerStart = 0;

// Timer for API fetch
unsigned long lastFetchTime = 0;
const unsigned long FETCH_INTERVAL = 30000; // Fetch new number every 30 seconds

bool servoActive = false;
bool buzzerActive = false;
bool systemLocked = false;

// ===== Stability Filter =====
int highGasCount = 0;

void setup() {

  Serial.begin(115200);

  // GSM Serial
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  delay(5000);
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // GSM Handshake
  for (int i = 0; i < 3; i++) {
    Serial2.println("AT");
    delay(500);
  }

  // Servo — start at 95
  myservo.attach(servoPin);
  myservo.write(95);

  // Buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  Serial.println("System Ready!");
  
  // Fetch the number immediately on startup
  fetchPhoneNumber();
}

// ===== API Fetch Function =====
void fetchPhoneNumber() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String requestUrl = SERVER_URL + "/api/phone/" + USER_ID;
    
    Serial.println("Fetching updated number from: " + requestUrl);
    http.begin(requestUrl);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      if (httpResponseCode == 200) {
        // Update phone number if successful
        phoneNumber = payload;
        phoneNumber.trim(); // remove whitespace or newlines
        Serial.println("✅ Stored Number Updated: " + phoneNumber);
      } else {
        Serial.println("⚠️ Server returned code: " + String(httpResponseCode));
        Serial.println("Payload: " + payload);
      }
    } else {
      Serial.println("❌ Error on HTTP request: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("❌ WiFi Disconnected. Cannot fetch number.");
  }
}

// ===== SMS Function =====
void sendSMS(String message) {

  Serial.println("\n--- SENDING SMS ---");

  Serial2.println("AT+CMGF=1");
  delay(1000);

  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");

  delay(1000);

  Serial2.print(message);
  delay(500);

  Serial2.write(26); // CTRL + Z

  delay(5000);

  while (Serial2.available()) {
    Serial.println(Serial2.readString());
  }

  Serial.println("--- SMS SENT ---");
}

void loop() {

  // ===== FETCH NUMBER PERIODICALLY =====
  if (millis() - lastFetchTime > FETCH_INTERVAL) {
    fetchPhoneNumber();
    lastFetchTime = millis();
  }

  int gasValue = analogRead(mq2Pin);

  Serial.print("Gas Value: ");
  Serial.println(gasValue);

  // ===== FILTER LOGIC =====
  if (gasValue > gasThreshold) {
    highGasCount++;
  } else {
    highGasCount = 0;
  }

  // ===== TRIGGER ONLY IF STABLE =====
  if (highGasCount >= 5 && !systemLocked) {

    Serial.println("GAS LEAK CONFIRMED!");

    // Servo ON — move to 0
    myservo.write(0);
    servoActive = true;
    servoStart = millis();

    // Buzzer ON
    digitalWrite(buzzerPin, HIGH);
    buzzerActive = true;
    buzzerStart = millis();

    // SMS ALERT
    sendSMS(
      "GAS LEAK DETECTED!\n"
      "The system detected a gas leak in your house.\n"
      "Please check immediately and ensure safety precautions are followed.\n\n"
      "In case of fire or explosion call:\n"
      "0918 241 7423\n"
      "0920 986 1460\n"
      "or call 911\n\n"
      "Gas measurement (ppm): " + String(gasValue)
    );

    systemLocked = true;
  }

  // ===== Servo Timer =====
  if (servoActive && millis() - servoStart >= 25000) {
    myservo.write(95);  // Return to 95
    servoActive = false;
    Serial.println("Servo OFF");
  }

  // ===== Buzzer Timer =====
  if (buzzerActive && millis() - buzzerStart >= 20000) {
    digitalWrite(buzzerPin, LOW);
    buzzerActive = false;
    Serial.println("Buzzer OFF");
  }

  // ===== RESET SYSTEM =====
  if (gasValue < (gasThreshold - 100) &&
      systemLocked &&
      !servoActive &&
      !buzzerActive) {

    systemLocked = false;
    highGasCount = 0;

    Serial.println("System Reset.");
  }

  delay(100);
}
