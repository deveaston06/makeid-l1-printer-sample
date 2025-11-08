#include <bitmap_operation.h>

Bitmap transformToColumnMajor(const Bitmap &bitmap) {
  Bitmap result = createEmptyBitmap();

  // Convert row-major to column-major (bottom-to-top)
  for (int x = 0; x < IMAGE_WIDTH; x++) {
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
      if (isPixelBlack(bitmap, x, y)) {
        const int colIdx = x * BYTES_PER_COLUMN + (IMAGE_HEIGHT - 1 - y) / 8;
        const int bitPos = (IMAGE_HEIGHT - 1 - y) % 8;
        result.data[colIdx] |= (1 << bitPos);
      }
    }
  }

  return result;
}

// Transform: 16-bit word swap
Bitmap transform16BitSwap(const Bitmap &bitmap) {
  Bitmap result = bitmap;

  for (int i = 0; i < BITMAP_SIZE; i += 2) {
    const uint8_t temp = result.data[i];
    result.data[i] = result.data[i + 1];
    result.data[i + 1] = temp;
  }

  return result;
}

// Transform: Bit inversion
Bitmap transformInvertBits(const Bitmap &bitmap) {
  Bitmap result = bitmap;

  for (int i = 0; i < BITMAP_SIZE; i++) {
    result.data[i] ^= 0xFF;
  }

  return result;
}

// Pure function: Complete printer format transformation pipeline
Bitmap transformToPrinterFormat(const Bitmap &bitmap) {
  return transformInvertBits(
      transform16BitSwap(transformToColumnMajor(bitmap)));
}

// ============================================================================
// BITMAP OPERATIONS (Pure functions returning new state)
// ============================================================================

// Create empty bitmap
Bitmap createEmptyBitmap() {
  Bitmap bitmap;
  clearBuffer(bitmap.data, BITMAP_SIZE);
  return bitmap;
}

// Create filled bitmap
Bitmap createFilledBitmap(bool black) {
  Bitmap bitmap;
  const uint8_t fillValue = black ? 0xFF : 0x00;
  for (size_t i = 0; i < BITMAP_SIZE; i++) {
    bitmap.data[i] = fillValue;
  }
  return bitmap;
}

// Set single pixel (returns modified bitmap)
Bitmap setPixel(const Bitmap &bitmap, int x, int y, bool black) {
  Bitmap result = bitmap;

  if (x < 0 || x >= IMAGE_WIDTH || y < 0 || y >= IMAGE_HEIGHT) {
    return result;
  }

  const int idx = pixelToIndex(x, y);
  const int byteIdx = indexToByte(idx);
  const int bitPos = indexToBit(idx);
  const uint8_t mask = bitMask(bitPos);

  if (black) {
    result.data[byteIdx] |= mask;
  } else {
    result.data[byteIdx] &= ~mask;
  }

  return result;
}

// Apply function to all pixels in range
Bitmap mapPixels(const Bitmap &bitmap, int x1, int y1, int x2, int y2,
                 std::function<bool(int, int)> predicate) {
  Bitmap result = bitmap;

  for (int y = y1; y < y2; y++) {
    for (int x = x1; x < x2; x++) {
      if (predicate(x, y)) {
        result = setPixel(result, x, y, true);
      }
    }
  }

  return result;
}

// Draw rectangle (functional style)
bool isInBorder(int x, int y, int thickness) {
  return x < thickness || x >= IMAGE_WIDTH - thickness || y < thickness ||
         y >= IMAGE_HEIGHT - thickness;
}

Bitmap drawBorder(const Bitmap &bitmap, int thickness) {
  return mapPixels(bitmap, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT,
                   [thickness](int x, int y) -> bool {
                     return isInBorder(x, y, thickness);
                   });
}

// Draw diagonal lines
bool isOnDiagonal(int x, int y) { return x == y || x == (IMAGE_WIDTH - 1 - y); }

Bitmap drawDiagonals(const Bitmap &bitmap) {
  const int minDim = (IMAGE_WIDTH < IMAGE_HEIGHT) ? IMAGE_WIDTH : IMAGE_HEIGHT;
  return mapPixels(bitmap, 0, 0, minDim, minDim, isOnDiagonal);
}

// Draw character at position
Bitmap drawChar(const Bitmap &bitmap, char c, int startX, int startY) {
  const uint8_t font[10][7] = {
      {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // 0
      {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
      {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, // 2
      {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E}, // 3
      {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
      {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
      {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
      {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
      {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
      {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // 9
  };

  if (c < '0' || c > '9')
    return bitmap;

  Bitmap result = bitmap;
  const int digit = c - '0';

  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      if (font[digit][row] & (1 << (4 - col))) {
        result = setPixel(result, startX + col, startY + row, true);
      }
    }
  }

  return result;
}

// Compose multiple bitmap operations (functional composition)
Bitmap compose(const Bitmap &bitmap, Bitmap (*f1)(const Bitmap &),
               Bitmap (*f2)(const Bitmap &), Bitmap (*f3)(const Bitmap &)) {
  Bitmap result = f1(bitmap);
  if (f2)
    result = f2(result);
  if (f3)
    result = f3(result);
  return result;
}
