#include <Arduino.h>
#include <ble_printer_manager.h>
#include <image_compressor.h>

void setup() {
  Serial.begin(115200);

  Serial.println("\n=== MakeID L1 Thermal Printer Demo ===");
  Serial.printf("Free heap at start: %d bytes\n", ESP.getFreeHeap());

  beginBLESniffer();
  if (PRINTER_MAC[0] != '\0') {
    setExampleBitmapFrame(); // this only replay btsnoop_hci3.log
    startPrintJob();
  }

  Serial.println("\n=== Creating Custom Bitmap ===");
  Bitmap image = createEmptyBitmap();

  // Draw border and diagonals
  drawBorder(image, 5);
  drawDiagonals(image);

  // Add text
  drawString(image, "42", 150, 40);

  Serial.println("Bitmap created");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

  // Print it!
  if (isPrinterConnected()) {
    Serial.println("\n=== Printing Custom Bitmap ===");
    if (printBitmap(image)) {
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
