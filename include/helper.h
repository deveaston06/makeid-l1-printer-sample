#ifndef HELPER_H
#define HELPER_H
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#define IMAGE_WIDTH 255
#define IMAGE_HEIGHT 96

// round up bits -> bytes
#define BITMAP_SIZE (((IMAGE_WIDTH) * (IMAGE_HEIGHT) + 7) / 8)
#define MAX_COMPRESSED_SIZE (BITMAP_SIZE + BITMAP_SIZE / 16 + 64 + 3)

struct Pixel {
  int x;
  int y;
  bool black;
};

struct Bitmap {
  uint8_t data[BITMAP_SIZE];
};

void clearBuffer(uint8_t *buf, size_t len);
void copyBuffer(uint8_t *dst, const uint8_t *src, size_t len);
int pixelToIndex(int x, int y);
int indexToByte(int idx);
int indexToBit(int idx);
uint8_t bitMask(int bitPos);
bool isPixelBlack(const Bitmap &bitmap, int x, int y);
uint8_t calculateChecksum(const uint8_t *data, size_t len);

// Safety helper: return true if byte index is valid for the Bitmap
inline bool isValidByteIndex(int byteIdx) {
  return (byteIdx >= 0 && static_cast<size_t>(byteIdx) < BITMAP_SIZE);
}

#endif // HELPER_H
