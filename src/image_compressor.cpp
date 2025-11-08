#include <Arduino.h>
#include <image_compressor.h>

uint8_t lzoWorkMem[LZO1X_1_MEM_COMPRESS];
uint8_t compressed[MAX_COMPRESSED_SIZE];
Bitmap printerFormatBuffer; // Static buffer for transformation

// Transform to column-major in place (uses static buffer)
void transformToColumnMajorInPlace(const Bitmap &source, Bitmap &dest) {
  clearBuffer(dest.data, BITMAP_SIZE);

  // Convert row-major to column-major (bottom-to-top)
  for (int x = 0; x < IMAGE_WIDTH; x++) {
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
      if (isPixelBlack(source, x, y)) {
        const int colIdx = x * BYTES_PER_COLUMN + (IMAGE_HEIGHT - 1 - y) / 8;
        const int bitPos = (IMAGE_HEIGHT - 1 - y) % 8;
        dest.data[colIdx] |= (1 << bitPos);
      }
    }
  }
}

void transform16BitSwapInPlace(Bitmap &bitmap) {
  for (int i = 0; i < BITMAP_SIZE; i += 2) {
    const uint8_t temp = bitmap.data[i];
    bitmap.data[i] = bitmap.data[i + 1];
    bitmap.data[i + 1] = temp;
  }
}

void transformInvertBitsInPlace(Bitmap &bitmap) {
  for (int i = 0; i < BITMAP_SIZE; i++) {
    bitmap.data[i] ^= 0xFF;
  }
}

void transformToPrinterFormatInPlace(const Bitmap &source, Bitmap &dest) {
  transformToColumnMajorInPlace(source, dest);
  transform16BitSwapInPlace(dest);
  transformInvertBitsInPlace(dest);
}

// stack overflow safe
bool testCompression(const Bitmap &userBitmap) {
  Serial.println("Converting to printer format...");
  Serial.printf("Free stack before transform: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));

  transformToPrinterFormatInPlace(userBitmap, printerFormatBuffer);

  Serial.printf("Free stack after transform: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));
  Serial.println("Compressing with LZO...");

  lzo_uint compressedLen;
  const int result = lzo1x_1_compress(printerFormatBuffer.data, BITMAP_SIZE,
                                      compressed, &compressedLen, lzoWorkMem);

  if (result != LZO_E_OK) {
    Serial.println("LZO compression failed");
    return false;
  }

  Serial.printf("Compressed %d → %d bytes (%.1f%%)\n", BITMAP_SIZE,
                (int)compressedLen, (compressedLen * 100.0) / BITMAP_SIZE);

  Serial.printf("Free stack after compress: %d bytes\n",
                uxTaskGetStackHighWaterMark(NULL));

  return true;
}
