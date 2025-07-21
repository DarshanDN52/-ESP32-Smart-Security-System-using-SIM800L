#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SIM800_RX 26
#define SIM800_TX 27
#define SIM800_RST 25
HardwareSerial sim800(1);

const int ir1Pin = 15;
const int ir2Pin = 4;
const int flamePin = 5;
const int ldrPin = 32;
const int buzzerPin = 2;

int irCount = 0;
String irState = "idle";
unsigned long triggerTime = 0;
bool scrollActive = false;
unsigned long lastScrollTime = 0;
unsigned long scrollStartTime = 0;
int scrollPos = 0;
const String scrollMsg = "  Person Entered Through Front Gate   ";

bool showingAlert = false;
String currentCallType = "";
bool callInProgress = false;
unsigned long callStartTime = 0;
const unsigned long callDuration = 20000;

bool systemSecure = false;

String lastIncomingMessage = "";

const String phoneNumber = "+91xxxxxxxxxx";

// === Setup ===
void setup() {
  pinMode(ir1Pin, INPUT);
  pinMode(ir2Pin, INPUT);
  pinMode(flamePin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  pinMode(SIM800_RST, OUTPUT);
  digitalWrite(SIM800_RST, LOW);
  delay(1000);
  digitalWrite(SIM800_RST, HIGH);
  delay(5000);

  Serial.begin(115200);
  sim800.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  delay(2000);
  lcd.clear();

  flushSIM800();

  Serial.println("Initializing SIM800...");
  if (!waitForAT() || !waitForNetwork()) {
    Serial.println("SIM800L not ready.");
    while (true);
  }

  sendCommand("AT+CMGF=1");
  sendCommand("AT+CSCS=\"GSM\"");
  sendCommand("AT+CNMI=1,2,0,0,0");
  sendCommand("AT+CMGDA=\"DEL ALL\"");

  Serial.println("System ready.");
}

// === Loop ===
void loop() {
  unsigned long now = millis();

  int ir1Val = digitalRead(ir1Pin);
  int ir2Val = digitalRead(ir2Pin);

  // IR logic: IR1 → IR2 = increment; IR2 → IR1 = decrement
  if (irState == "idle") {
    if (ir1Val == LOW && ir2Val == HIGH) {
      irState = "wait_ir2";
      triggerTime = now;
    } else if (ir2Val == LOW && ir1Val == HIGH) {
      irState = "wait_ir1";
      triggerTime = now;
    }
  }

  if (irState == "wait_ir2" && now - triggerTime < 1000) {
    if (ir2Val == LOW) {
      irCount++;
      beep();
      scrollActive = true;
      scrollPos = 0;
      irState = "idle";
    }
  } else if (irState == "wait_ir1" && now - triggerTime < 1000) {
    if (ir1Val == LOW && irCount > 0) {
      irCount--;
      beep();
      irState = "idle";
    }
  }

  if ((irState == "wait_ir2" || irState == "wait_ir1") && now - triggerTime >= 1000) {
    irState = "idle";
  }

  bool flameDetected = digitalRead(flamePin) == HIGH;
  int ldrVal = analogRead(ldrPin);
  bool laserCut = ldrVal < 3500;

  Serial.printf("IR1:%d IR2:%d Count:%d Flame:%d LDR:%d\n", ir1Val, ir2Val, irCount, flameDetected, ldrVal);

  if ((flameDetected || laserCut) && systemSecure) systemSecure = false;

  if ((flameDetected && currentCallType != "flame") || (laserCut && currentCallType != "laser")) {
    String alertType = flameDetected ? "flame" : "laser";
    String alertMsg = flameDetected ? "Fire Alert" : "Laser Alert";
    String smsText = flameDetected ? "Fire Detected! People Count: " : "Laser Broken! People Count: ";
    smsText += String(irCount);

    currentCallType = alertType;
    showingAlert = true;
    scrollActive = false;
    digitalWrite(buzzerPin, HIGH);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(alertMsg);
    lcd.setCursor(0, 1);
    lcd.print("Total People: ");
    lcd.print(irCount);

    Serial.println("ALERT: " + alertMsg);
    Serial.println("Sending SMS...");
    if (sendSMS(smsText)) {
      Serial.println("SMS sent successfully.");
    } else {
      Serial.println("SMS failed after retries.");
    }

    if (!callInProgress) {
      Serial.println("Dialing call...");
      if (makeCall(phoneNumber)) {
        callInProgress = true;
        callStartTime = millis();
        Serial.println("Call started.");
      } else {
        Serial.println("Call failed.");
      }
    } else {
      Serial.println("Call already in progress. Skipping new call.");
    }
  }

  if (callInProgress && millis() - callStartTime > callDuration) {
    Serial.println("Ending call after timeout...");
    endCall();
  }

  if (sim800.available()) {
    lastIncomingMessage = sim800.readString();
    Serial.println("Received: " + lastIncomingMessage);

    if (lastIncomingMessage.indexOf("Secure") != -1 || lastIncomingMessage.indexOf("SECURE") != -1) {
      systemSecure = true;
      showingAlert = false;
      currentCallType = "";
      digitalWrite(buzzerPin, LOW);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Status: Secure");
      lcd.setCursor(0, 1);
      lcd.print("Total People: ");
      lcd.print(irCount);

      Serial.println("System marked as secure. Buzzer OFF.");
    }
  }

  if (!showingAlert && scrollActive) {
    if (millis() - lastScrollTime > 300) {
      lastScrollTime = millis();
      lcd.setCursor(0, 0);
      lcd.print(scrollMsg.substring(scrollPos, scrollPos + 16));
      scrollPos++;
      if (scrollPos > scrollMsg.length() - 16) scrollActive = false;
    }
    lcd.setCursor(0, 1);
    lcd.print("Total People: ");
    lcd.print(irCount);
    lcd.print("  ");
  } else if (!showingAlert && systemSecure) {
    lcd.setCursor(0, 0);
    lcd.print("Status: Secure   ");
    lcd.setCursor(0, 1);
    lcd.print("Total People: ");
    lcd.print(irCount);
    lcd.print("  ");
  }

  delay(100);
}

// === Functions ===

void beep() {
  digitalWrite(buzzerPin, HIGH);
  delay(150);
  digitalWrite(buzzerPin, LOW);
}

bool sendSMS(String message) {
  Serial.println("Starting SMS sending...");

  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.print("SMS Attempt: ");
    Serial.println(attempt);

    sim800.println("AT+CMGF=1");
    if (!waitForResponse("OK", 3000)) {
      Serial.println("Failed to set SMS mode.");
      continue;
    }

    sim800.println("AT+CMGDA=\"DEL ALL\"");
    waitForResponse("OK", 2000);

    sim800.print("AT+CMGS=\"");
    sim800.print(phoneNumber);
    sim800.println("\"");

    if (!waitForResponse(">", 5000)) {
      Serial.println("No '>' prompt received.");
      continue;
    }

    sim800.print(message);
    delay(500);
    sim800.write(26); // Ctrl+Z to send

    Serial.println("Waiting for SMS confirmation...");
    if (waitForResponse("OK", 10000)) {
      Serial.println("SMS sent successfully.");
      flushSIM800();
      return true;
    } else {
      Serial.println("SMS failed.");
      flushSIM800();
    }
  }

  Serial.println("All SMS attempts failed.");
  return false;
}

bool makeCall(String number) {
  Serial.println("Starting call...");

  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.print("Call Attempt: ");
    Serial.println(attempt);

    String dialCmd = "ATD" + number + ";";
    sim800.println(dialCmd);
    Serial.println("Sent: " + dialCmd);

    if (waitForResponse("OK", 5000)) {
      Serial.println("Call started.");
      return true;
    } else {
      Serial.println("Call attempt failed.");
    }
  }

  Serial.println("All call attempts failed.");
  return false;
}

void endCall() {
  sim800.println("ATH");
  delay(1000);
  flushSIM800();
  callInProgress = false;
  currentCallType = "";
}

void sendCommand(String cmd) {
  sim800.println(cmd);
  delay(1000);
  flushSIM800();
}

bool waitForAT() {
  for (int i = 0; i < 10; i++) {
    sim800.println("AT");
    delay(500);
    if (sim800.find("OK")) return true;
    flushSIM800();
  }
  return false;
}

bool waitForNetwork() {
  for (int i = 0; i < 10; i++) {
    sim800.println("AT+CREG?");
    delay(500);
    if (sim800.find("+CREG: 0,1") || sim800.find("+CREG: 0,5")) return true;
    flushSIM800();
  }
  return false;
}

bool waitForResponse(const char* resp, unsigned long timeout) {
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (sim800.find(resp)) return true;
  }
  return false;
}

void flushSIM800() {
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
}
