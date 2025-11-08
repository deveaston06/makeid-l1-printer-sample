#include <bitmap_operation.h>
#include <minilzo.h>

// Maximum size after LZO compression
#define MAX_COMPRESSED_SIZE (BITMAP_SIZE + BITMAP_SIZE / 16 + 64 + 3)

// Static buffers (declared in .cpp)
extern uint8_t lzoWorkMem[LZO1X_1_MEM_COMPRESS];
extern uint8_t compressed[MAX_COMPRESSED_SIZE];
extern Bitmap printerFormatBuffer;

// ============================================================================
// Convert bitmap data into printer formet operations (stack overflow safe)
// ============================================================================

void transformToPrinterFormatInPlace(const Bitmap &source, Bitmap &dest);
void transformToColumnMajorInPlace(const Bitmap &source, Bitmap &dest);
void transform16BitSwapInPlace(Bitmap &bitmap);
void transformInvertBitsInPlace(Bitmap &bitmap);

bool testCompression(const Bitmap &userBitmap);
