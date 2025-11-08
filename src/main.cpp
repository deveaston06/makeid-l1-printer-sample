#include <Arduino.h>
#include <ble_printer_manager.h>
#include <image_compressor.h>

void setup() {
  Serial.begin(115200);
  Serial.printf("Free heap at start: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Free stack: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));

  beginBLESniffer();
  if (PRINTER_MAC[0] != '\0') {
    setExampleBitmapFrame(); // replay btsnoop_hci3.log frames
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
  if (testCompression(image)) {
    Serial.println("Success!");
  }
}

void loop() { delay(1000); }
