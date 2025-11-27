#include <Arduino.h>
#include <image_compressor.h>

uint8_t lzoWorkMem[LZO1X_1_MEM_COMPRESS];
uint8_t compressed[MAX_COMPRESSED_SIZE];
Bitmap printerFormatBuffer; // Static buffer for transformation

void extractChunkColumns(const Bitmap &printerFormat, Bitmap &chunk,
                         int startCol, int chunkWidth) {
  clearBuffer(chunk.data, BITMAP_SIZE);

  const int chunkBytes = chunkWidth * BYTES_PER_COLUMN;
  const int startByte = startCol * BYTES_PER_COLUMN;

  for (int i = 0; i < chunkBytes && (startByte + i) < BITMAP_SIZE; i++) {
    chunk.data[i] = printerFormat.data[startByte + i];
  }
}

std::vector<PrinterFrame> compressAndGenerateFrames(const Bitmap &userBitmap) {
  std::vector<PrinterFrame> frames;

  Serial.println("=== Starting Compression ===");

  // Step 1: Convert to printer format
  transformToPrinterFormatInPlace(userBitmap, printerFormatBuffer);

  // Step 2: Calculate chunk distribution (like Python code)
  int remainingWidth = IMAGE_WIDTH % DEFAULT_CHUNK_WIDTH;
  int numberOfChunks = ceil((float)IMAGE_WIDTH / DEFAULT_CHUNK_WIDTH);
  int framesRemaining = numberOfChunks - 1;

  Serial.printf(
      "Image width: %d, Chunks: %d, Default width: %d, Remainder: %d\n",
      IMAGE_WIDTH, numberOfChunks, DEFAULT_CHUNK_WIDTH, remainingWidth);

  int columnOffset = 0;

  // Temporary buffer for chunk compression
  uint8_t chunkBuffer[BYTES_PER_COLUMN * DEFAULT_CHUNK_WIDTH];
  lzo_uint compressedLen;

  for (int chunkIdx = 0; chunkIdx < numberOfChunks; chunkIdx++) {
    // Calculate chunk width (distribute remainder like Python)
    int chunkWidth = DEFAULT_CHUNK_WIDTH;

    // Calculate byte range for this chunk
    int byteOffset = columnOffset * BYTES_PER_COLUMN;
    int chunkBytes = chunkWidth * BYTES_PER_COLUMN;

    // Extract chunk data from printer format buffer
    memset(chunkBuffer, 0, sizeof(chunkBuffer));
    for (int i = 0; i < chunkBytes && (byteOffset + i) < BITMAP_SIZE; i++) {
      chunkBuffer[i] = printerFormatBuffer.data[byteOffset + i];
    }

    // Compress this chunk
    int result = lzo1x_1_compress(chunkBuffer, chunkBytes, compressed,
                                  &compressedLen, lzoWorkMem);

    if (result != LZO_E_OK) {
      Serial.printf("Chunk %d compression failed!\n", chunkIdx + 1);
      continue;
    }

    // Determine actual frame width for this chunk
    uint8_t frameWidth = (chunkIdx == numberOfChunks - 1 && remainingWidth > 0)
                             ? remainingWidth
                             : DEFAULT_CHUNK_WIDTH;

    // Create BLE frame
    auto frameData =
        createBLEFrame(compressed, compressedLen, framesRemaining, frameWidth);

    PrinterFrame frame;
    frame.data = frameData;
    frame.chunkWidth = frameWidth;
    frame.framesRemaining = framesRemaining;

    frames.push_back(frame);

    Serial.printf("Chunk %d/%d: width=%d cols, uncompressed=%dB → "
                  "compressed=%dB (%.1f%%), frame_total=%dB, remaining=%d\n",
                  chunkIdx + 1, numberOfChunks, chunkWidth, chunkBytes,
                  (int)compressedLen, (compressedLen * 100.0) / chunkBytes,
                  frameData.size(), framesRemaining);

    columnOffset += chunkWidth;
    framesRemaining--;
  }

  Serial.printf("Total frames generated: %d\n", frames.size());
  return frames;
}

std::vector<uint8_t> createBLEFrame(const uint8_t *compressedData,
                                    size_t compressedSize,
                                    uint16_t framesRemaining,
                                    uint8_t chunkWidth) {
  std::vector<uint8_t> frame;

  frame.push_back(0x66); // Magic number

  uint16_t length = 17 + compressedSize + 1;
  frame.push_back(length & 0xFF);
  frame.push_back((length >> 8) & 0xFF);

  // Printer constant (8 bytes)
  const uint8_t PRINTER_CONSTANT[] = {0x1B, 0x2F, 0x03, 0x01,
                                      0x00, 0x01, 0x00, 0x01};
  for (int i = 0; i < 8; i++) {
    frame.push_back(PRINTER_CONSTANT[i]);
  }

  // Image width (little endian)
  frame.push_back(IMAGE_WIDTH & 0xFF);
  frame.push_back((IMAGE_WIDTH >> 8) & 0xFF);

  // Chunk width and frames remaining
  frame.push_back(chunkWidth);
  frame.push_back((framesRemaining >> 8) & 0xFF);
  frame.push_back(framesRemaining & 0xFF);

  // End of header
  frame.push_back(0x00);

  // Compressed data
  for (size_t i = 0; i < compressedSize; i++) {
    frame.push_back(compressedData[i]);
  }

  // Calculate checksum
  uint8_t checksum = 0;
  for (size_t i = 0; i < frame.size(); i++) {
    checksum = (checksum - frame[i]) & 0xFF;
  }
  frame.push_back(checksum);

  return frame;
}

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
