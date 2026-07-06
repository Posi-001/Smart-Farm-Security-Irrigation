#include <SoftwareSerial.h>

// Rx = Pin 10 (Connect to ESP32 TX pin 17)
// Tx = Pin 11 (Connect to ESP32 RX pin 16 through level shifter)
SoftwareSerial espSerial(10, 11);

// ---------- SENSOR PINS ----------
const int sensorPins[5] = {A0, A1, A2, A3, A4};

// ---------- VALVE RELAYS ----------
// Index 0, 1, 2 = Pins 2, 3, 4 (FRONT)
// Index 3, 4    = Pins 5, 6 (BACK)
const int valveRelays[5] = {2, 3, 4, 5, 6};

// ---------- PUMP RELAY ----------
const int pumpRelay = 7;

// ---------- THRESHOLD ----------
int dryThreshold = 600;

// ---------- SYSTEM STATE ----------
enum SystemState { NORMAL_IRRIGATION, SECURITY_OVERRIDE };
SystemState currentState = NORMAL_IRRIGATION;

int moistureValues[5];
bool zoneDry[5];
bool anyZoneDry = false;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  // Relay setup
  for (int i = 0; i < 5; i++) {
    pinMode(valveRelays[i], OUTPUT);
    digitalWrite(valveRelays[i], HIGH); // OFF initially
  }

  pinMode(pumpRelay, OUTPUT);
  digitalWrite(pumpRelay, HIGH); // Pump OFF initially

  Serial.println("Smart Irrigation System Started");
}

void loop() {
  // ---------- 1. CHECK FOR ESP32 THREAT MESSAGES ----------
  if (espSerial.available()) {
    String msg = espSerial.readStringUntil('\n');
    msg.trim();

    if (msg == "FRONT_THREAT") {
      Serial.println("SECURITY OVERRIDE: FRONT THREAT");
      currentState = SECURITY_OVERRIDE;
      activateSecurityRouting(true); // true = front
    } 
    else if (msg == "BACK_THREAT") {
      Serial.println("SECURITY OVERRIDE: BACK THREAT");
      currentState = SECURITY_OVERRIDE;
      activateSecurityRouting(false); // false = back
    } 
    else if (msg == "NORMAL" || msg == "FRONT_CLEAR" || msg == "BACK_CLEAR") {
      if (currentState != NORMAL_IRRIGATION) {
        Serial.println("SECURITY CLEARED: Returning to Normal");
        currentState = NORMAL_IRRIGATION;
        resetAllValves(); 
      }
    }
  }

  // ---------- 2. READ & SEND MOISTURE SENSORS ----------
  anyZoneDry = false;
  String moistureData = "Moisture: ";

  for (int i = 0; i < 5; i++) {
    moistureValues[i] = analogRead(sensorPins[i]);
    
    // Append to the string we will send to the ESP32
    moistureData += "Z" + String(i+1) + ":" + String(moistureValues[i]) + " ";

    if (moistureValues[i] > dryThreshold) {
      zoneDry[i] = true;
      anyZoneDry = true;
    } else {
      zoneDry[i] = false;
    }
  }

  // Send the data string to ESP32
  espSerial.println(moistureData);

  // ---------- 3. HANDLE NORMAL IRRIGATION ----------
  if (currentState == NORMAL_IRRIGATION) {
    if (anyZoneDry) {
      // OPEN required valves first
      for (int i = 0; i < 5; i++) {
        if (zoneDry[i]) digitalWrite(valveRelays[i], LOW);
        else digitalWrite(valveRelays[i], HIGH);
      }
      delay(1000); // Wait for valves to open
      
      // THEN start pump
      digitalWrite(pumpRelay, LOW);
    } 
    else {
      // STOP pump FIRST
      digitalWrite(pumpRelay, HIGH);
      delay(1000);
      
      // THEN close all valves
      resetAllValves();
    }
  }

  delay(1000); // Shorter delay keeps serial responsive
}

// =====================================================
// HELPER FUNCTIONS
// =====================================================

void activateSecurityRouting(bool isFront) {
  // 1. IMMEDIATELY shut down the irrigation pump
  digitalWrite(pumpRelay, HIGH);
  delay(1000); // Give pressure 1 second to drop

  // 2. Close all valves to reset state
  resetAllValves();
  
  // 3. Open the specific target valves for the deterrent
  if (isFront) {
    digitalWrite(valveRelays[0], LOW); // Pin 2
    digitalWrite(valveRelays[1], LOW); // Pin 3
    digitalWrite(valveRelays[2], LOW); // Pin 4
    Serial.println("Front Deterrent Path Opened (Pins 2,3,4)");
  } else {
    digitalWrite(valveRelays[3], LOW); // Pin 5
    digitalWrite(valveRelays[4], LOW); // Pin 6
    Serial.println("Back Deterrent Path Opened (Pins 5,6)");
  }
}

void resetAllValves() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(valveRelays[i], HIGH);
  }
}
