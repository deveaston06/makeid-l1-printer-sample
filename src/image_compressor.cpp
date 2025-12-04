#include <Arduino.h>
#include <image_compressor.h>

// ========================================================
// GLOBAL HEAP BUFFERS (initialized by initCompression())
// ========================================================

static uint8_t *g_lzoWorkMem = nullptr;
static uint8_t *g_compressed = nullptr;
static Bitmap *g_printerFormatBuffer = nullptr;

// ========================================================
// COMPRESSION LIFECYCLE
// ========================================================

bool initCompression() {
  g_lzoWorkMem = (uint8_t *)malloc(LZO1X_1_MEM_COMPRESS);
  g_compressed = (uint8_t *)malloc(MAX_COMPRESSED_SIZE);
  g_printerFormatBuffer = (Bitmap *)malloc(sizeof(Bitmap));

  if (!g_lzoWorkMem || !g_compressed || !g_printerFormatBuffer) {
    free(g_lzoWorkMem);
    free(g_compressed);
    free(g_printerFormatBuffer);
    g_lzoWorkMem = nullptr;
    g_compressed = nullptr;
    g_printerFormatBuffer = nullptr;
    return false;
  }

  memset(g_lzoWorkMem, 0, LZO1X_1_MEM_COMPRESS);
  memset(g_compressed, 0, MAX_COMPRESSED_SIZE);
  clearBuffer(g_printerFormatBuffer->data, BITMAP_SIZE);

  return true;
}

void cleanupCompression() {
  free(g_lzoWorkMem);
  g_lzoWorkMem = nullptr;
  free(g_compressed);
  g_compressed = nullptr;
  free(g_printerFormatBuffer);
  g_printerFormatBuffer = nullptr;
}

// ========================================================
// BITMAP TRANSFORMS
// ========================================================

void transformToColumnMajor(const Bitmap &source, Bitmap &dest) {
  clearBuffer(dest.data, BITMAP_SIZE);

  for (int x = 0; x < IMAGE_WIDTH; x++) {
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
      if (isPixelBlack(source, x, y)) {
        int col = x * BYTES_PER_COLUMN;
        int row = (IMAGE_HEIGHT - 1 - y) / 8;
        int bit = (IMAGE_HEIGHT - 1 - y) % 8;
        int idx = col + row;

        if (idx >= 0 && idx < BITMAP_SIZE) {
          dest.data[idx] |= (1 << bit);
        }
      }
    }
  }
}

// 16-bit row swap (height assumed multiple of 8)
void transform16BitSwap(Bitmap &bitmap) {
  uint8_t *temp = (uint8_t *)malloc(BITMAP_SIZE);
  if (!temp)
    return;

  copyBuffer(temp, bitmap.data, BITMAP_SIZE);

  for (int col = 0; col < IMAGE_WIDTH; col++) {
    int s = col * BYTES_PER_COLUMN;

    for (int i = 0; i < BYTES_PER_COLUMN; i += 2) {
      int dst = s + i;
      int src = s + (BYTES_PER_COLUMN - 2 - i);

      if (dst + 1 < BITMAP_SIZE && src + 1 < BITMAP_SIZE) {
        bitmap.data[dst] = temp[src];
        bitmap.data[dst + 1] = temp[src + 1];
      }
    }
  }

  free(temp);
}

void transformToPrinterFormat(const Bitmap &source, Bitmap &dest) {
  transformToColumnMajor(source, dest);
  transform16BitSwap(dest);
}

// ========================================================
// COMPRESSION + FRAME GENERATION
// ========================================================

std::vector<PrinterFrame> compressAndGenerateFrames(const Bitmap &userBitmap,
                                                    uint16_t mtu) {
  std::vector<PrinterFrame> frames;

  if (!g_printerFormatBuffer || !g_lzoWorkMem || !g_compressed) {
    Serial.println("ERROR: Compression not initialized");
    return frames;
  }

  transformToPrinterFormat(userBitmap, *g_printerFormatBuffer);

  int chunks = (IMAGE_WIDTH + DEFAULT_CHUNK_WIDTH - 1) / DEFAULT_CHUNK_WIDTH;
  int remainder = IMAGE_WIDTH - (chunks - 1) * DEFAULT_CHUNK_WIDTH;

  uint8_t *chunkBuffer =
      (uint8_t *)malloc(BYTES_PER_COLUMN * DEFAULT_CHUNK_WIDTH);
  if (!chunkBuffer)
    return frames;

  int columnOffset = 0;
  int framesRemaining = chunks - 1;

  for (int chunkIdx = 0; chunkIdx < chunks; chunkIdx++) {
    int chunkWidth = (chunkIdx == chunks - 1) ? remainder : DEFAULT_CHUNK_WIDTH;

    int chunkBytes = chunkWidth * BYTES_PER_COLUMN;
    int byteOffset = columnOffset * BYTES_PER_COLUMN;

    memset(chunkBuffer, 0, chunkBytes);

    for (int i = 0; i < chunkBytes && (byteOffset + i) < BITMAP_SIZE; i++) {
      chunkBuffer[i] = g_printerFormatBuffer->data[byteOffset + i];
    }

    lzo_uint compressedLen = 0;
    int res = lzo1x_1_compress(chunkBuffer, chunkBytes, g_compressed,
                               &compressedLen, g_lzoWorkMem);

    if (res != LZO_E_OK) {
      Serial.printf("Compression failed (chunk %d)\n", chunkIdx);
      continue;
    }

    auto fullFrame = createBLEFrame(g_compressed, compressedLen,
                                    framesRemaining, chunkWidth);

    size_t idx = 0;
    bool first = true;

    while (idx < fullFrame.size()) {
      PrinterFrame f;
      size_t sz = std::min((size_t)mtu, fullFrame.size() - idx);

      f.data.assign(fullFrame.begin() + idx, fullFrame.begin() + idx + sz);
      f.chunkWidth = chunkWidth;
      f.framesRemaining = framesRemaining;
      f.isContinuation = !first;

      frames.push_back(f);
      first = false;
      idx += sz;
    }

    framesRemaining--;
    columnOffset += chunkWidth;
  }

  free(chunkBuffer);
  return frames;
}

std::vector<uint8_t> createBLEFrame(const uint8_t *compressedData,
                                    size_t compressedSize,
                                    uint16_t framesRemaining,
                                    uint8_t chunkWidth) {
  std::vector<uint8_t> frame;

  frame.push_back(0x66);

  uint16_t length = 17 + compressedSize + 1;
  frame.push_back(length & 0xFF);
  frame.push_back((length >> 8) & 0xFF);

  const uint8_t CMD[] = {0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01};
  frame.insert(frame.end(), CMD, CMD + 8);

  frame.push_back(IMAGE_WIDTH & 0xFF);
  frame.push_back(IMAGE_WIDTH >> 8);

  frame.push_back(chunkWidth);
  frame.push_back(framesRemaining >> 8);
  frame.push_back(framesRemaining & 0xFF);
  frame.push_back(0x00);

  frame.insert(frame.end(), compressedData, compressedData + compressedSize);

  uint8_t checksum = 0;
  for (uint8_t b : frame)
    checksum -= b;
  frame.push_back(checksum);

  return frame;
}
