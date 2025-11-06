#include <Arduino.h>
#include <HX711.h>
#include <LiquidCrystal_I2C.h>
#include <ble_printer_manager.h>

// Defines pin numbers
const int trigPin = 12; // Connects to the Trig pin of the HC-SR04P
const int echoPin = 13; // Connects to the Echo pin of the HC-SR04P

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 18;
const int LOADCELL_SCK_PIN = 19;

// Defines variables
long duration;  // Variable for the duration of sound wave travel
int distanceMM; // Variable for the distance measurement in centimeters

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;

// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

HX711 scale;
const float CALIBRATION_FACTOR = 1100;
int reading;
int lastReading;

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  lcd.init();
  lcd.backlight();

  NimBLEDevice::init("ESP32-BLE-Sniffer");

  beginBLESniffer();
  setExampleBitmapFrame(); // set to print payload_3_45mm.png
  startPrintJob();

  // scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  // scale.set_scale(CALIBRATION_FACTOR); // this value is obtained by
  // calibrating the scale with known weights
  // scale.tare();                        // reset the scale to 0
}

void loop() {

  // Clears the trigPin by setting it LOW for a short period
  // digitalWrite(trigPin, LOW);
  // delayMicroseconds(2);
  //
  // // Sets the trigPin HIGH for 10 microseconds to send a pulse
  // digitalWrite(trigPin, HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigPin, LOW);
  //
  // // Reads the echoPin, returns the sound wave travel time in microseconds
  // duration = pulseIn(echoPin, HIGH);
  //
  // // Calculates the distance in centimeters
  // // Speed of sound in air is approximately 343 meters/second or 0.343
  // // mm/microsecond The pulse travels to the object and back, so divide by 2
  // distanceMM = duration * 0.343 / 2;
  //
  // // Displays the distance on the Serial Monitor
  // lcd.setCursor(0, 0);
  // lcd.print("Distance:       ");
  // lcd.setCursor(10, 0);
  // lcd.print(distanceMM);
  // lcd.print(" mm");

  // Example payload from Wireshark (QR command)
  // uint8_t qr1[] = {0x66, 0x06, 0x00, 0x10, 0x00, 0x84};
  //
  // if (pWriteChar) {
  //   sendChunkWaitAck(qr1, sizeof(qr1));
  // }

  // if (scale.wait_ready_timeout(200)) {
  //   reading = round(scale.get_units());
  //   Serial.print("Weight: ");
  //   Serial.println(reading);
  //   if (reading != lastReading) {
  //     lcd.setCursor(0, 1);
  //     lcd.print("Weight:         ");
  //     lcd.setCursor(8, 1);
  //     lcd.print(reading);
  //     lcd.print(" g");
  //   }
  //   lastReading = reading;
  // } else {
  //   Serial.println("HX711 not found.");
  // }
  //
  delay(1000); // Small delay to allow for stable readings
}
