#include <Arduino.h>
#include <image_compressor.h>

uint8_t lzoWorkMem[LZO1X_1_MEM_COMPRESS];
uint8_t compressed[MAX_COMPRESSED_SIZE];

bool testCompression(const Bitmap &userBitmap) {
  Serial.println("Converting to printer format...");

  // Pure transformation pipeline
  const Bitmap printerBitmap = transformToPrinterFormat(userBitmap);

  Serial.println("Compressing with LZO...");

  lzo_uint compressedLen;
  const int result = lzo1x_1_compress(printerBitmap.data, BITMAP_SIZE,
                                      compressed, &compressedLen, lzoWorkMem);

  if (result != LZO_E_OK) {
    Serial.println("LZO compression failed");
    return false;
  }

  Serial.printf("Compressed %d → %d bytes (%.1f%%)\n", BITMAP_SIZE,
                (int)compressedLen, (compressedLen * 100.0) / BITMAP_SIZE);
  return true;
}
