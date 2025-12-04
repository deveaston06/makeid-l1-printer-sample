#include <bitmap_operation.h>

// ============================================================================
// BIT REPRESENTATION:
// - In memory: 0 = white, 1 = black (matches thermal printer)
// - Pixel packing: MSB first (bit 7 = first pixel, bit 0 = last pixel)
// - Coordinates: (0,0) = top-left, (383,95) = bottom-right
// ============================================================================

// ============================================================================
// BITMAP CREATION (Returns new bitmap)
// ============================================================================

Bitmap *createEmptyBitmapPtr() {
  Bitmap *bmp = new (std::nothrow) Bitmap();
  if (!bmp)
    return nullptr;
  clearBuffer(bmp->data, BITMAP_SIZE);
  return bmp;
}

// Create empty bitmap (all white - 0x00)
Bitmap createEmptyBitmap() {
  Bitmap bitmap;
  clearBuffer(bitmap.data, BITMAP_SIZE);
  return bitmap;
}

// Create filled bitmap (0xFF = black, 0x00 = white)
Bitmap createFilledBitmap(bool black) {
  Bitmap bitmap;
  const uint8_t fillValue = black ? 0xFF : 0x00;
  for (size_t i = 0; i < BITMAP_SIZE; i++) {
    bitmap.data[i] = fillValue;
  }
  return bitmap;
}

// ============================================================================
// IN-PLACE OPERATIONS (Modify bitmap directly, no copies)
// ============================================================================

void setPixel(Bitmap &bitmap, int x, int y, bool black) {
  if (x < 0 || x >= IMAGE_WIDTH || y < 0 || y >= IMAGE_HEIGHT) {
    return;
  }

  const int idx = pixelToIndex(x, y);
  const int byteIdx = indexToByte(idx);
  const int bitPos = indexToBit(idx);
  const uint8_t mask = bitMask(bitPos);

  // Defensive: ensure we don't write beyond allocated buffer
  if (!isValidByteIndex(byteIdx)) {
    // In debug builds assert to catch logic errors; in production just return
    assert(false && "byteIdx out of range in setPixel");
    return;
  }

  if (black) {
    bitmap.data[byteIdx] |= mask;
  } else {
    bitmap.data[byteIdx] &= ~mask;
  }
}

// Clear pixel (helper for convenience)
void clearPixel(Bitmap &bitmap, int x, int y) { setPixel(bitmap, x, y, false); }

void fillBitmap(Bitmap &bitmap, bool black) {
  const uint8_t fillValue = black ? 0xFF : 0x00;
  for (size_t i = 0; i < BITMAP_SIZE; i++) {
    bitmap.data[i] = fillValue;
  }
}

// Clear entire bitmap (convenience wrapper)
void clearBitmap(Bitmap &bitmap) { clearBuffer(bitmap.data, BITMAP_SIZE); }

// Apply function to all pixels in range (modifies in place)
void mapPixels(Bitmap &bitmap, int x1, int y1, int x2, int y2,
               std::function<bool(int, int)> predicate) {
  for (int y = y1; y < y2; y++) {
    for (int x = x1; x < x2; x++) {
      if (predicate(x, y)) {
        setPixel(bitmap, x, y, true);
      }
    }
  }
}

void drawBorder(Bitmap &bitmap, int thickness) {
  // Use setPixel for consistency
  // Top border
  for (int y = 0; y < thickness; y++) {
    for (int x = 0; x < IMAGE_WIDTH; x++) {
      setPixel(bitmap, x, y, true);
    }
  }

  // Bottom border
  for (int y = IMAGE_HEIGHT - thickness; y < IMAGE_HEIGHT; y++) {
    for (int x = 0; x < IMAGE_WIDTH; x++) {
      setPixel(bitmap, x, y, true);
    }
  }

  // Left border
  for (int y = thickness; y < IMAGE_HEIGHT - thickness; y++) {
    for (int x = 0; x < thickness; x++) {
      setPixel(bitmap, x, y, true);
    }
  }

  // Right border
  for (int y = thickness; y < IMAGE_HEIGHT - thickness; y++) {
    for (int x = IMAGE_WIDTH - thickness; x < IMAGE_WIDTH; x++) {
      setPixel(bitmap, x, y, true);
    }
  }
}

void drawDiagonals(Bitmap &bitmap) {
  const int minDim = (IMAGE_WIDTH < IMAGE_HEIGHT) ? IMAGE_WIDTH : IMAGE_HEIGHT;

  for (int i = 0; i < minDim; i++) {
    // Main diagonal (top-left to bottom-right)
    setPixel(bitmap, i, i, true);

    // Anti-diagonal (top-right to bottom-left)
    setPixel(bitmap, IMAGE_WIDTH - 1 - i, i, true);
  }
}

// Draw character at position (modifies in place)
void drawChar(Bitmap &bitmap, char c, int startX, int startY) {
  static const uint8_t font[10][7] = {
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

  if (c < '0' || c > '9') {
    return;
  }

  const int digit = c - '0';

  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      if (font[digit][row] & (1 << (4 - col))) {
        const int x = startX + col;
        const int y = startY + row;
        if (x >= 0 && x < IMAGE_WIDTH && y >= 0 && y < IMAGE_HEIGHT) {
          const int idx = pixelToIndex(x, y);
          bitmap.data[indexToByte(idx)] |= bitMask(indexToBit(idx));
        }
      }
    }
  }
}

void drawString(Bitmap &bitmap, const char *str, int startX, int startY) {
  int x = startX;
  while (*str) {
    drawChar(bitmap, *str, x, startY);
    x += 6; // 5 pixels wide + 1 pixel spacing
    str++;
  }
}

void drawRect(Bitmap &bitmap, int x1, int y1, int x2, int y2) {
  // Ensure coordinates are in order
  if (x1 > x2) {
    int temp = x1;
    x1 = x2;
    x2 = temp;
  }
  if (y1 > y2) {
    int temp = y1;
    y1 = y2;
    y2 = temp;
  }

  // Top and bottom edges
  for (int x = x1; x <= x2; x++) {
    setPixel(bitmap, x, y1, true);
    setPixel(bitmap, x, y2, true);
  }

  // Left and right edges
  for (int y = y1; y <= y2; y++) {
    setPixel(bitmap, x1, y, true);
    setPixel(bitmap, x2, y, true);
  }
}

void fillRect(Bitmap &bitmap, int x1, int y1, int x2, int y2) {
  // Ensure coordinates are in order
  if (x1 > x2) {
    int temp = x1;
    x1 = x2;
    x2 = temp;
  }
  if (y1 > y2) {
    int temp = y1;
    y1 = y2;
    y2 = temp;
  }

  for (int y = y1; y <= y2; y++) {
    for (int x = x1; x <= x2; x++) {
      setPixel(bitmap, x, y, true);
    }
  }
}

void drawLine(Bitmap &bitmap, int x0, int y0, int x1, int y1) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    setPixel(bitmap, x0, y0, true);

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void drawCircle(Bitmap &bitmap, int centerX, int centerY, int radius) {
  int x = radius;
  int y = 0;
  int err = 0;

  while (x >= y) {
    setPixel(bitmap, centerX + x, centerY + y, true);
    setPixel(bitmap, centerX + y, centerY + x, true);
    setPixel(bitmap, centerX - y, centerY + x, true);
    setPixel(bitmap, centerX - x, centerY + y, true);
    setPixel(bitmap, centerX - x, centerY - y, true);
    setPixel(bitmap, centerX - y, centerY - x, true);
    setPixel(bitmap, centerX + y, centerY - x, true);
    setPixel(bitmap, centerX + x, centerY - y, true);

    if (err <= 0) {
      y += 1;
      err += 2 * y + 1;
    }
    if (err > 0) {
      x -= 1;
      err -= 2 * x + 1;
    }
  }
}

void fillCircle(Bitmap &bitmap, int centerX, int centerY, int radius) {
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x * x + y * y <= radius * radius) {
        setPixel(bitmap, centerX + x, centerY + y, true);
      }
    }
  }
}

void invertBitmap(Bitmap &bitmap) {
  for (int i = 0; i < BITMAP_SIZE; i++) {
    bitmap.data[i] ^= 0xFF;
  }
}

void copyBitmap(Bitmap &dest, const Bitmap &src) {
  copyBuffer(dest.data, src.data, BITMAP_SIZE);
}

void drawGrid(Bitmap &bitmap, int spacing) {
  // Vertical lines
  for (int x = 0; x < IMAGE_WIDTH; x += spacing) {
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
      setPixel(bitmap, x, y, true);
    }
  }

  // Horizontal lines
  for (int y = 0; y < IMAGE_HEIGHT; y += spacing) {
    for (int x = 0; x < IMAGE_WIDTH; x++) {
      setPixel(bitmap, x, y, true);
    }
  }
}

void drawCheckerboard(Bitmap &bitmap, int squareSize) {
  for (int y = 0; y < IMAGE_HEIGHT; y++) {
    for (int x = 0; x < IMAGE_WIDTH; x++) {
      bool shouldDraw = ((x / squareSize) + (y / squareSize)) % 2 == 0;
      setPixel(bitmap, x, y, shouldDraw);
    }
  }
}

// ============================================================================
// COMPOSE FUNCTION (Modified to work with in-place operations)
// ============================================================================

void compose(Bitmap &bitmap, void (*f1)(Bitmap &), void (*f2)(Bitmap &),
             void (*f3)(Bitmap &)) {
  if (f1)
    f1(bitmap);
  if (f2)
    f2(bitmap);
  if (f3)
    f3(bitmap);
}
