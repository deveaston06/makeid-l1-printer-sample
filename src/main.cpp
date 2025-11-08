#include <Arduino.h>
#include <ble_printer_manager.h>
#include <image_compressor.h>

void setup() {
  Serial.begin(115200);
  Serial.printf("Free heap at start: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Free stack: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));

  beginBLESniffer();
  if (PRINTER_MAC[0] != '\0') {
    setExampleBitmapFrame(); // this only replay btsnoop_hci3.log
    startPrintJob();
  }

  Bitmap image = createEmptyBitmap();

  Serial.printf("After createEmpty - Free stack: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));

  drawBorder(image, 5);
  Serial.printf("After drawBorder - Free stack: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));

  drawDiagonals(image);
  Serial.printf("After drawDiagonals - Free stack: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));

  drawChar(image, '4', 50, 40);
  drawChar(image, '2', 60, 40);

  Serial.printf("After all operations - Free stack: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));

  Serial.println("\n=== Starting Compression Test ===");
  if (testCompression(image)) {
    Serial.println("Compression test success!");
  } else {
    Serial.println("Compression test failed!");
  }

  Serial.println("\n=== Final Memory Stats ===");
  Serial.printf("Free heap at end: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Free stack at end: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Serial.printf("Stack used: %d bytes\n",
                8192 - uxTaskGetStackHighWaterMark(NULL));

  // Summary
  Serial.println("\n=== Memory Summary ===");
  Serial.println("Static allocations:");
  Serial.printf("  - lzoWorkMem: %d bytes\n", LZO1X_1_MEM_COMPRESS);
  Serial.printf("  - compressed buffer: %d bytes\n", MAX_COMPRESSED_SIZE);
  Serial.printf("  - printerFormatBuffer: %d bytes\n", BITMAP_SIZE);
  Serial.printf("  Total static: %d bytes\n",
                LZO1X_1_MEM_COMPRESS + MAX_COMPRESSED_SIZE + BITMAP_SIZE);
  Serial.printf("\nStack allocation:");
  Serial.printf("  - image bitmap: %d bytes\n", BITMAP_SIZE);
  Serial.printf("  - other variables: ~%d bytes\n",
                8192 - uxTaskGetStackHighWaterMark(NULL) - BITMAP_SIZE);
}

void loop() { delay(1000); }
