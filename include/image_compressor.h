#ifndef IMAGE_COMPRESSOR_H
#define IMAGE_COMPRESSOR_H

#include <cstdint>
#include <helper.h>
#include <minilzo.h>
#include <vector>

#define MAX_COMPRESSED_SIZE (BITMAP_SIZE + BITMAP_SIZE / 16 + 64 + 3)
#define DEFAULT_CHUNK_WIDTH 85
#define BYTES_PER_COLUMN 12
#define CID_0004_HEADER_BYTES 7

// Frame structure for BLE transmission
struct PrinterFrame {
  std::vector<uint8_t> data;
  size_t chunkWidth;
  uint16_t framesRemaining;
  bool isContinuation;
};

// CHANGED: No more global static arrays - use dynamic allocation
struct CompressionBuffers {
  uint8_t *lzoWorkMem;
  uint8_t *compressed;
  Bitmap *printerFormatBuffer;

  CompressionBuffers();
  ~CompressionBuffers();
  bool isValid() const;
};

// Initialize compression system (call once at startup)
bool initCompression();
void cleanupCompression();

void transformToPrinterFormat(const Bitmap &source, Bitmap &dest);
void transformToColumnMajor(const Bitmap &source, Bitmap &dest);
void transform16BitSwap(Bitmap &bitmap);

void extractChunkColumns(const Bitmap &printerFormat, Bitmap &chunk,
                         int startCol, int chunkWidth);

std::vector<PrinterFrame> compressAndGenerateFrames(const Bitmap &userBitmap,
                                                    uint16_t mtu);
std::vector<uint8_t> createBLEFrame(const uint8_t *compressedData,
                                    size_t compressedSize,
                                    uint16_t framesRemaining,
                                    uint8_t chunkWidth);

#endif // !IMAGE_COMPRESSOR_H
