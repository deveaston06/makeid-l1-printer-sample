#include <Arduino.h>
#include <bitmap_operation.h>
#include <ble_printer_manager.h>
#include <image_compressor.h>

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize

  Serial.println("\n=== MakeID L1 Thermal Printer Demo ===");
  Serial.printf("Free heap at start: %d bytes\n", ESP.getFreeHeap());

  // CRITICAL: Initialize compression system FIRST
  if (!initCompression()) {
    Serial.println("FATAL: Failed to initialize compression!");
    Serial.println("Cannot continue. Halting.");
    while (1) {
      delay(1000);
    }
  }

  Serial.printf("Free heap after compression init: %d bytes\n",
                ESP.getFreeHeap());

  beginBLESniffer();
  if (PRINTER_MAC[0] == '\0') {
    Serial.println("No printer MAC configured!");
    return;
  }

  // -------------------------------------
  // MEMORY-SAFE BITMAP CREATION
  // -------------------------------------
  Serial.println("\n=== Creating Custom Bitmap (Heap) ===");

  // Use pointer version to avoid stack overflow
  Bitmap *testBitmap = createEmptyBitmapPtr();
  if (!testBitmap) {
    Serial.println("ERROR: Failed to allocate bitmap!");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    return;
  }

  Serial.println("Bitmap allocated successfully");
  Serial.printf("Free heap after bitmap: %d bytes\n", ESP.getFreeHeap());

  // Draw pattern
  drawDiagonals(*testBitmap);

  Serial.println("Pattern drawn");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

  // Print if connected
  if (isPrinterConnected()) {
    Serial.println("\n=== Printing Custom Bitmap ===");
    if (printBitmap(*testBitmap)) {
      // Wait for print to complete
      while (isPrinting()) {
        delay(100);
      }
      Serial.println("Print complete!");
    } else {
      Serial.println("Print failed!");
    }
  } else {
    Serial.println("Printer not connected - skipping print");
  }

  // Clean up bitmap
  delete testBitmap;
  Serial.println("Bitmap freed");
  Serial.printf("Free heap after cleanup: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
  // Monitor memory in loop
  static unsigned long lastMemCheck = 0;
  if (millis() - lastMemCheck > 5000) {
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    lastMemCheck = millis();
  }

  delay(1000);
}
