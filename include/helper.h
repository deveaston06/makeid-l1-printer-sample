#ifndef HELPER_H
#define HELPER_H
#include <cstddef>
#include <cstdint>

#define IMAGE_WIDTH 384
#define IMAGE_HEIGHT 96
#define BYTES_PER_COLUMN (IMAGE_HEIGHT / 8)
#define BITMAP_SIZE ((IMAGE_WIDTH * IMAGE_HEIGHT) / 8)
#define MAX_COMPRESSED_SIZE (BITMAP_SIZE + BITMAP_SIZE / 16 + 64 + 3)
#define MAX_FRAME_PAYLOAD 500

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

#endif // HELPER_H
