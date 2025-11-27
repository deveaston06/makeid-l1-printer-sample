#ifndef IMAGE_COMPRESSOR_H
#define IMAGE_COMPRESSOR_H

#include <helper.h>
#include <minilzo.h>
#include <vector>

#define MAX_COMPRESSED_SIZE (BITMAP_SIZE + BITMAP_SIZE / 16 + 64 + 3)
#define DEFAULT_CHUNK_WIDTH 85
#define BYTES_PER_COLUMN (IMAGE_HEIGHT / 8) // 12 bytes for 96 pixels

// Frame structure for BLE transmission
struct PrinterFrame {
  std::vector<uint8_t> data;
  size_t chunkWidth;
  uint16_t framesRemaining;
};

extern uint8_t lzoWorkMem[LZO1X_1_MEM_COMPRESS];
extern uint8_t compressed[MAX_COMPRESSED_SIZE];
extern Bitmap printerFormatBuffer;

void extractChunkColumns(const Bitmap &printerFormat, Bitmap &chunk,
                         int startCol, int chunkWidth);

std::vector<PrinterFrame> compressAndGenerateFrames(const Bitmap &userBitmap);
std::vector<uint8_t> createBLEFrame(const uint8_t *compressedData,
                                    size_t compressedSize,
                                    uint16_t framesRemaining,
                                    uint8_t chunkWidth);

void transformToPrinterFormatInPlace(const Bitmap &source, Bitmap &dest);
void transformToColumnMajorInPlace(const Bitmap &source, Bitmap &dest);
void transform16BitSwapInPlace(Bitmap &bitmap);
void transformInvertBitsInPlace(Bitmap &bitmap);

bool testCompression(const Bitmap &userBitmap);

#endif // !IMAGE_COMPRESSOR_H
