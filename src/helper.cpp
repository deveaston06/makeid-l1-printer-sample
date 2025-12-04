#include <helper.h>

void clearBuffer(uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    buf[i] = 0;
  }
}

// Pure function: Copy buffer
void copyBuffer(uint8_t *dst, const uint8_t *src, size_t len) {
  for (size_t i = 0; i < len; i++) {
    dst[i] = src[i];
  }
}

// Pure function: Calculate pixel index from coordinates
int pixelToIndex(int x, int y) { return y * IMAGE_WIDTH + x; }

// Pure function: Calculate byte index from pixel index
int indexToByte(int idx) { return idx / 8; }

// Pure function: Calculate bit position from pixel index
int indexToBit(int idx) { return idx % 8; }

// Pure function: Create bit mask for position
uint8_t bitMask(int bitPos) { return 0x80 >> bitPos; }

// Pure function: Check if pixel is black
bool isPixelBlack(const Bitmap &bitmap, int x, int y) {
  if (x < 0 || x >= IMAGE_WIDTH || y < 0 || y >= IMAGE_HEIGHT) {
    return false;
  }
  const int idx = pixelToIndex(x, y);
  return bitmap.data[indexToByte(idx)] & bitMask(indexToBit(idx));
}

// Pure function: Calculate checksum
uint8_t calculateChecksum(const uint8_t *data, size_t len) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < len - 1; i++) {
    checksum = (checksum - data[i]) & 0xFF;
  }
  return checksum;
}
