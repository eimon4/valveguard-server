#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Network Settings
const char* ssid = "PaoJulgwapo";
const char* password = "PaopaoJulia1225";

// API Settings
const String SERVER_URL = "https://valveguard-server.onrender.com";
const String USER_ID = "3b2e92f9-b687-4190-8094-f16a1eefadd7";

// Fallback Number
String phoneNumber = "+639685698288";

// GSM Pins
#define RXD2 16
#define TXD2 17

// Hardware Pins
const int servoPin = 18;
const int buzzerPin = 19;
const int mq2Pin = 34;

const int gasThreshold = 1200;

Servo myservo;

// State Variables
unsigned long servoStart = 0;
unsigned long buzzerStart = 0;
unsigned long lastFetchTime = 0;
unsigned long lastPrintTime = 0;
const unsigned long FETCH_INTERVAL = 30000;

bool servoActive = false;
bool buzzerActive = false;
bool systemLocked = false;
int highGasCount = 0;

void setup() {
  Serial.begin(115200);

  // Init GSM
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(5000);

  // Connect WiFi
  Serial.print("Connecting WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected.");

  // GSM Handshake
  for (int i = 0; i < 3; i++) {
    Serial2.println("AT");
    delay(500);
  }

  // Init Servo
  myservo.attach(servoPin);
  myservo.write(95);

  // Init Buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  Serial.println("System Ready.");

  // Initial Fetch
  fetchPhoneNumber();
}

// Fetch Number from API
void fetchPhoneNumber() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String requestUrl = SERVER_URL + "/api/phone/" + USER_ID;

    Serial.println("Fetching number...");
    http.begin(requestUrl);

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      payload.trim();
      phoneNumber = payload;
      Serial.println("Stored Number: " + phoneNumber);
    } else {
      Serial.println("HTTP Error: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected.");
  }
}

// Sends SMS
void sendSMS(String message) {
  Serial.println("\nSending SMS...");

  Serial2.println("AT+CMGF=1");
  delay(1000);

  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");
  delay(1000);

  Serial2.print(message);
  delay(500);

  Serial2.write(26); // Send
  delay(5000);

  while (Serial2.available()) {
    Serial.println(Serial2.readString());
  }

  Serial.println("SMS Sent.");
}

void loop() {
  // Fetch Number Periodically
  if (millis() - lastFetchTime > FETCH_INTERVAL) {
    fetchPhoneNumber();
    lastFetchTime = millis();
  }

  int gasValue = analogRead(mq2Pin);

  if (millis() - lastPrintTime >= 2000) {
    Serial.print("Gas Value: ");
    Serial.println(gasValue);
    lastPrintTime = millis();
  }

  // Gas Filter
  if (gasValue > gasThreshold) {
    highGasCount++;
  } else {
    highGasCount = 0;
  }

  // Trigger Emergency
  if (highGasCount >= 1 && !systemLocked) {
    Serial.println("GAS LEAK CONFIRMED!");

    myservo.write(0);
    servoActive = true;
    servoStart = millis();

    digitalWrite(buzzerPin, HIGH);
    buzzerActive = true;
    buzzerStart = millis();

    sendSMS(
      "!!! GAS LEAK ALERT !!!\n\n"
      "A possible gas leak has been detected in your house.\n\n"
      "ACTION REQUIRED:\n"
      "- Check the area immediately.\n"
      "- Turn off the gas source if safe to do so.\n"
      "- Avoid flames and electrical switches.\n\n"
      "EMERGENCY CONTACTS:\n"
      "Fire Department: 0918 241 7423\n"
      "0920 986 1460 or you can call 911\n"
      "Please prioritize safety.\n\n"
      "GAS LEVEL:\n"
      + String(gasValue) + " ppm\n\n"
    );

    systemLocked = true;
  }

  // Reset Servo
  if (servoActive && millis() - servoStart >= 25000) {
    myservo.write(95);
    servoActive = false;
    Serial.println("Servo OFF");
  }

  // Reset Buzzer
  if (buzzerActive && millis() - buzzerStart >= 20000) {
    digitalWrite(buzzerPin, LOW);
    buzzerActive = false;
    Serial.println("Buzzer OFF");
  }

  // Reset System
  if (gasValue < (gasThreshold - 100) && systemLocked && !servoActive && !buzzerActive) {
    systemLocked = false;
    highGasCount = 0;
    Serial.println("System Reset.");
  }

  delay(100);
}
