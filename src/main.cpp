#include <Arduino.h>
#include <ble_printer_manager.h>

void setup() {
  Serial.begin(115200);

  NimBLEDevice::init("ESP32-BLE-Sniffer");

  beginBLESniffer();
  if (PRINTER_MAC[0] != '\0') {
    setExampleBitmapFrame(); // replay btsnoop_hci3.log frames
    startPrintJob();
  }
}

void loop() { delay(1000); }
