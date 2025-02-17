#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h" // Algorithm for calculating heart rate
#include <Adafruit_SSD1306.h> // Library for OLED display

#define SCREEN_WIDTH 128 // Width of the display
#define SCREEN_HEIGHT 64 // Height of the display
#define BUZZER_PIN 25    // Buzzer pin
#define PIEZO_PIN A0     // Piezoelectric sensor pin

MAX30105 particleSensor;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define debug Serial // For debugging via Serial

float spo2 = 0.0;
int heartRate = 0;
int breathRate = 0;
int criticalBreathCounter = 0;

void setup()
{
  debug.begin(9600);
  debug.println("Initializing...");

  // Buzzer initialization
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // MAX30102 initialization
  if (!particleSensor.begin()) {
    debug.println("MAX30102 not found. Check the connection!");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); // Low power for red LED
  particleSensor.setPulseAmplitudeGreen(0); // Green LED off

  // Display initialization
  if (!display.begin(SSD1306_I2C_ADDRESS, 0x3C)) {
    debug.println("Failed to initialize the display!");
    while (1);
  }
  display.clearDisplay();
  display.display();
}

void loop()
{
  // Get data from MAX30102
  long red = particleSensor.getRed();
  long ir = particleSensor.getIR();

  if (ir < 50000) {
    displayData("No finger", "", "", "");
    return;
  }

  // Calculate SpO2
  float R = (float)red / (float)ir;
  spo2 = 104.0 - 17.0 * R;

  // Calculate heart rate
  heartRate = checkHeartRate(ir);

  // Read data from piezoelectric sensor
  int piezoValue = analogRead(PIEZO_PIN);
  if (piezoValue > 1000) {
    criticalBreathCounter++;
  } else {
    criticalBreathCounter = 0;
  }
  
  // Calculate respiratory rate (simulation)
  breathRate = calculateBreathRate(piezoValue);

  // Check for critical parameters
  if (spo2 < 95 || heartRate > 150 || criticalBreathCounter > 300) {
    triggerAlarm();
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Display data on the screen
  displayData("SpO2:", String(spo2, 1) + "%", "HR:", String(heartRate) + " bpm");
  delay(1000);
}

int checkHeartRate(long irValue) {
  static int rates[4];
  static int rateIndex = 0;
  static long lastBeat = 0;
  long currentTime = millis();

  if (checkForBeat(irValue)) {
    long delta = currentTime - lastBeat;
    lastBeat = currentTime;

    int bpm = 60 / (delta / 1000.0);
    bpm = constrain(bpm, 30, 180); // Limit HR to reasonable bounds
    
    rates[rateIndex++] = bpm;
    rateIndex %= 4;

    int sum = 0;
    for (int i = 0; i < 4; i++) sum += rates[i];
    return sum / 4;
  }
  return heartRate;
}

int calculateBreathRate(int piezoValue) {
  // Simulated respiratory rate calculation, replace with real algorithm if needed
  return map(piezoValue, 0, 1023, 10, 30);
}

void triggerAlarm() {
  digitalWrite(BUZZER_PIN, HIGH);
  debug.println("CRITICAL PARAMETERS!");
}

void displayData(String label1, String value1, String label2, String value2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print(label1);
  display.setCursor(64, 0);
  display.print(value1);

  display.setCursor(0, 16);
  display.print(label2);
  display.setCursor(64, 16);
  display.print(value2);

  display.display();
}
