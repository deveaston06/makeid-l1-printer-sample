#include <Arduino.h>
#include <ble_printer_manager.h>
#include <image_compressor.h>

void setup() {
  Serial.begin(115200);

  beginBLESniffer();
  if (PRINTER_MAC[0] != '\0') {
    setExampleBitmapFrame(); // replay btsnoop_hci3.log frames
    startPrintJob();
  }

  Bitmap image = createEmptyBitmap();
  image = createEmptyBitmap();
  image = drawBorder(image, 5);
  image = drawDiagonals(image);
  image = drawChar(image, '4', 50, 40);
  image = drawChar(image, '2', 60, 40);

  if (testCompression(image)) {
    Serial.println("Success!");
  }
}

void loop() { delay(1000); }
