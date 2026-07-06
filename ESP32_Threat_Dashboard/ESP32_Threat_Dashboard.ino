#include <WiFi.h>
#include <WebServer.h>

// =========================
// WIFI SETTINGS
// =========================
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

HardwareSerial mySerial(2);

// =========================
// FRONT DEVICES
// =========================
const int frontPir = 18;
const int frontBuzzer1 = 21;
const int frontBuzzer2 = 22;
const int frontLed = 25;

// =========================
// BACK DEVICES
// =========================
const int backPir = 19;
const int backBuzzer = 23;
const int backLed = 26;

// =========================
// DETERRENT SYSTEM
// =========================
const int solenoid6Relay = 14;
const int pump2Relay = 27;

// =========================
// TIMING
// =========================
unsigned long alertDuration = 5000;

// =========================
// FRONT VARIABLES
// =========================
unsigned long frontLastMotion = 0;
unsigned long frontStartTime = 0;
bool frontThreat = false;
bool frontDeterrentActivated = false;

// =========================
// BACK VARIABLES
// =========================
unsigned long backLastMotion = 0;
unsigned long backStartTime = 0;
bool backThreat = false;
bool backDeterrentActivated = false;

// =========================
// SYSTEM VARIABLES
// =========================
bool deterrentSystemActive = false;
String latestMoistureData = "Waiting for data from Arduino...";

// =====================================================
// WEB SERVER HANDLER
// =====================================================
void handleRoot() {
  String page = "";

  page += "<!DOCTYPE html><html>";
  page += "<head>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"; // Makes it scale to your phone
  page += "<meta http-equiv='refresh' content='2'>";
  page += "<style>";
  page += "body { font-family: Arial, sans-serif; text-align: center; margin-top: 20px; background-color: #f4f4f4; }";
  page += ".card { background: white; margin: 10px auto; padding: 15px; border-radius: 8px; width: 90%; max-width: 400px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); }";
  page += ".active { color: red; font-weight: bold; }";
  page += ".clear { color: green; font-weight: bold; }";
  page += "</style>";
  page += "</head>";

  page += "<body>";
  page += "<h2>SMART FARM DASHBOARD</h2>";

  // Security Card
  page += "<div class='card'>";
  page += "<h3>SECURITY STATUS</h3>";
  
  if (frontThreat) {
    page += "<p>Front Threat: <span class='active'>ACTIVE</span></p>";
  } else {
    page += "<p>Front Threat: <span class='clear'>CLEAR</span></p>";
  }

  if (backThreat) {
    page += "<p>Back Threat: <span class='active'>ACTIVE</span></p>";
  } else {
    page += "<p>Back Threat: <span class='clear'>CLEAR</span></p>";
  }

  page += "<p>Deterrent System: <strong>" + String(deterrentSystemActive ? "ON" : "OFF") + "</strong></p>";
  page += "</div>";

  // Irrigation Card
  page += "<div class='card'>";
  page += "<h3>IRRIGATION STATUS</h3>";
  page += "<p>" + latestMoistureData + "</p>";
  page += "</div>";

  page += "</body>";
  page += "</html>";

  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, 16, 17);

  // ---------- WIFI SETUP ----------
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nCONNECTED!");
  Serial.print("Dashboard IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web Server Started");

  // ---------- PIN SETUP ----------
  // FRONT
  pinMode(frontPir, INPUT);
  pinMode(frontBuzzer1, OUTPUT);
  pinMode(frontBuzzer2, OUTPUT);
  pinMode(frontLed, OUTPUT);

  // BACK
  pinMode(backPir, INPUT);
  pinMode(backBuzzer, OUTPUT);
  pinMode(backLed, OUTPUT);

  // DETERRENT
  pinMode(solenoid6Relay, OUTPUT);
  pinMode(pump2Relay, OUTPUT);

  // OFF initially
  digitalWrite(frontBuzzer1, HIGH);
  digitalWrite(frontBuzzer2, HIGH);
  digitalWrite(backBuzzer, HIGH);
  digitalWrite(frontLed, LOW);
  digitalWrite(backLed, LOW);
  digitalWrite(solenoid6Relay, HIGH);
  digitalWrite(pump2Relay, HIGH);

  Serial.println("Integrated Threat System Ready");
  delay(2000);
}

void loop() {
  // Listen for Web Clients (Your Phone)
  server.handleClient();

  handleFrontZone();
  handleBackZone();
  manageDeterrentSystem();

  // ---------- LISTEN FOR ARDUINO DATA ----------
  while (mySerial.available()) {
    String incomingData = mySerial.readStringUntil('\n');
    incomingData.trim();
    if (incomingData.length() > 0) {
      Serial.print("[ARDUINO]: ");
      Serial.println(incomingData);
      
      // Update the global string if it contains moisture data
      if (incomingData.startsWith("Moisture:")) {
        latestMoistureData = incomingData;
      }
    }
  }

  delay(50); // Reduced delay slightly so the web server feels snappier
}

// =====================================================
// FRONT ZONE
// =====================================================

void handleFrontZone() {
  int motion = digitalRead(frontPir);

  if (motion == HIGH) {
    frontLastMotion = millis();
    if (!frontThreat) {
      frontStartTime = millis();
      Serial.println("FRONT MOTION DETECTED");
    }
    frontThreat = true;
  }

  if (frontThreat && (millis() - frontLastMotion < alertDuration)) {
    unsigned long elapsed = millis() - frontStartTime;

    if (elapsed < 5000) {
      digitalWrite(frontBuzzer1, LOW);
      digitalWrite(frontBuzzer2, LOW);
      digitalWrite(frontLed, LOW);
    } else if (elapsed < 10000) {
      digitalWrite(frontBuzzer1, LOW);
      digitalWrite(frontBuzzer2, LOW);
      digitalWrite(frontLed, millis() / 300 % 2);
    } else {
      digitalWrite(frontBuzzer1, LOW);
      digitalWrite(frontBuzzer2, LOW);
      digitalWrite(frontLed, millis() / 150 % 2);

      if (!frontDeterrentActivated) {
        frontDeterrentActivated = true;
        Serial.println("FRONT THREAT");
        mySerial.println("FRONT_THREAT");
      }
    }
  } else {
    if (frontThreat) {
      Serial.println("FRONT CLEAR");
      mySerial.println("FRONT_CLEAR");
    }
    digitalWrite(frontBuzzer1, HIGH);
    digitalWrite(frontBuzzer2, HIGH);
    digitalWrite(frontLed, LOW);
    frontThreat = false;
    frontDeterrentActivated = false;
  }
}

// =====================================================
// BACK ZONE
// =====================================================

void handleBackZone() {
  int motion = digitalRead(backPir);

  if (motion == HIGH) {
    backLastMotion = millis();
    if (!backThreat) {
      backStartTime = millis();
      Serial.println("BACK MOTION DETECTED");
    }
    backThreat = true;
  }

  if (backThreat && (millis() - backLastMotion < alertDuration)) {
    unsigned long elapsed = millis() - backStartTime;

    if (elapsed < 5000) {
      digitalWrite(backBuzzer, LOW);
      digitalWrite(backLed, LOW);
    } else if (elapsed < 10000) {
      digitalWrite(backBuzzer, LOW);
      digitalWrite(backLed, millis() / 300 % 2);
    } else {
      digitalWrite(backBuzzer, LOW);
      digitalWrite(backLed, millis() / 150 % 2);

      if (!backDeterrentActivated) {
        backDeterrentActivated = true;
        Serial.println("BACK THREAT");
        mySerial.println("BACK_THREAT");
      }
    }
  } else {
    if (backThreat) {
      Serial.println("BACK CLEAR");
      mySerial.println("BACK_CLEAR");
    }
    digitalWrite(backBuzzer, HIGH);
    digitalWrite(backLed, LOW);
    backThreat = false;
    backDeterrentActivated = false;
  }
}

// =====================================================
// DETERRENT SYSTEM MANAGEMENT
// =====================================================

void manageDeterrentSystem() {
  if (frontDeterrentActivated || backDeterrentActivated) {
    if (!deterrentSystemActive) {
      deterrentSystemActive = true;
      Serial.println("DETERRENT SYSTEM ACTIVE");
      digitalWrite(solenoid6Relay, LOW);
      delay(1000);
      digitalWrite(pump2Relay, LOW);
    }
  } else {
    if (deterrentSystemActive) {
      deterrentSystemActive = false;
      Serial.println("DETERRENT SYSTEM OFF");
      digitalWrite(pump2Relay, HIGH);
      delay(1000);
      digitalWrite(solenoid6Relay, HIGH);
      mySerial.println("NORMAL");
    }
  }
}
