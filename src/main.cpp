#include <Arduino.h>
#include <bitmap_operation.h>
#include <ble_printer_manager.h>
#include <image_compressor.h>

void setup() {
  Serial.begin(115200);

  Serial.println("\n=== MakeID L1 Thermal Printer Demo ===");
  Serial.printf("Free heap at start: %d bytes\n", ESP.getFreeHeap());

  beginBLESniffer();
  if (PRINTER_MAC[0] == '\0')
    return;

  // setExampleBitmapFrame();
  //
  // if (isPrinterConnected()) {
  //   Serial.println("\n=== Printing using Python Generated Frames ===");
  //   if (startPrintJob()) {
  //     // Wait for print to complete
  //     while (isPrinting()) {
  //       delay(100);
  //     }
  //     Serial.println("Print complete!");
  //   } else {
  //     Serial.println("Print failed!");
  //   }
  // }

  // -------------------------------------
  // drawing from scratch programmatically
  // -------------------------------------
  Serial.println("\n=== Creating Custom Bitmap ===");
  Bitmap testBitmap = createEmptyBitmap();

  drawCheckerboard(testBitmap, 10);

  Serial.println("Bitmap created");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

  if (isPrinterConnected()) {
    Serial.println("\n=== Printing Custom Bitmap ===");
    if (printBitmap(testBitmap)) {
      // Wait for print to complete
      while (isPrinting()) {
        delay(100);
      }
      Serial.println("Print complete!");
    } else {
      Serial.println("Print failed!");
    }
  }
}

void loop() { delay(1000); }
