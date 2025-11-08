#include <ble_printer_manager.h>

// Printer constants
const uint8_t PRINTER_ID[8] = {0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01};
const uint16_t DEFAULT_JOB_ID = 0x012B;

// BLE globals
NimBLERemoteService *pPrinterService = nullptr;
NimBLEClient *pClient = nullptr;
NimBLERemoteCharacteristic *pWriteChar = nullptr;
NimBLERemoteCharacteristic *pNotifyChar = nullptr;

// Print job state
volatile bool ackReceived = false;
std::vector<uint8_t> lastAck;
bool printingInProgress = false;

// Frame storage for current print job
std::vector<std::vector<uint8_t>> printFrames;
int currentFrameIndex = 0;
uint16_t currentJobId = DEFAULT_JOB_ID;

// ============================================================================
// FRAME CONSTRUCTION
// ============================================================================

// Calculate checksum for frame
uint8_t calculateFrameChecksum(const uint8_t *data, size_t len) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < len - 1; i++) {
    checksum = (checksum - data[i]) & 0xFF;
  }
  return checksum;
}

// Create a single frame from compressed data chunk
std::vector<uint8_t> createFrame(const uint8_t *compressedData, size_t offset,
                                 size_t chunkSize, uint16_t jobId,
                                 uint16_t framesRemaining, bool isFinal) {
  std::vector<uint8_t> frame;

  // Magic number
  frame.push_back(0x66);

  // Frame length (little endian, excludes magic & length bytes)
  uint16_t len = 16 + chunkSize + 4;
  frame.push_back(len & 0xFF);
  frame.push_back((len >> 8) & 0xFF);

  // Printer ID (8 bytes)
  for (int i = 0; i < 8; i++) {
    frame.push_back(PRINTER_ID[i]);
  }

  // Job ID (little endian)
  frame.push_back(jobId & 0xFF);
  frame.push_back((jobId >> 8) & 0xFF);

  // Frame magic (0x55 for data frames, 0x2C for final)
  frame.push_back(isFinal ? 0x2C : 0x55);

  // Frames remaining (little endian)
  frame.push_back((framesRemaining >> 8) & 0xFF);
  frame.push_back(framesRemaining & 0xFF);

  // Compressed payload
  for (size_t i = 0; i < chunkSize; i++) {
    frame.push_back(compressedData[offset + i]);
  }

  // End marker
  frame.push_back(0x11);
  frame.push_back(0x00);
  frame.push_back(0x00);

  // Checksum
  uint8_t checksum = calculateFrameChecksum(frame.data(), frame.size() + 1);
  frame.push_back(checksum);

  return frame;
}

// ============================================================================
// PRINT JOB MANAGEMENT
// ============================================================================

// Prepare frames from compressed bitmap data
bool prepareFramesFromBitmap(const Bitmap &userBitmap) {
  Serial.println("=== Preparing Print Job ===");

  // Clear previous frames
  printFrames.clear();

  // Transform to printer format (uses static buffer)
  Serial.println("Converting to printer format...");
  transformToPrinterFormatInPlace(userBitmap, printerFormatBuffer);

  // Compress with LZO
  Serial.println("Compressing with LZO...");
  lzo_uint compressedLen = 0;
  int result = lzo1x_1_compress(printerFormatBuffer.data, BITMAP_SIZE,
                                compressed, &compressedLen, lzoWorkMem);

  if (result != LZO_E_OK) {
    Serial.println("LZO compression failed!");
    return false;
  }

  Serial.printf("Compressed %lu → %lu bytes (%.1f%%)\n",
                (unsigned long)BITMAP_SIZE, (unsigned long)compressedLen,
                (compressedLen * 100.0) / BITMAP_SIZE);

  // ========================================================================
  // Always split into 4 frames equally (last frame gets leftover bytes)
  // ========================================================================
  const int NUM_FRAMES = 4;
  size_t baseChunkSize = compressedLen / NUM_FRAMES;
  size_t remainder = compressedLen % NUM_FRAMES;
  size_t offset = 0;

  for (int i = 0; i < NUM_FRAMES; i++) {
    size_t chunkSize = baseChunkSize;

    // Distribute remainder bytes evenly among first few chunks
    if (i < remainder) {
      chunkSize++;
    }

    // If compressedLen < NUM_FRAMES (edge case), assign 0-size safely
    if (offset >= compressedLen)
      chunkSize = 0;
    else if (offset + chunkSize > compressedLen)
      chunkSize = compressedLen - offset;

    bool isFinal = (i == NUM_FRAMES - 1);
    uint16_t framesRemaining = NUM_FRAMES - 1 - i;

    std::vector<uint8_t> frame = createFrame(
        compressed, offset, chunkSize, currentJobId, framesRemaining, isFinal);

    printFrames.push_back(frame);

    Serial.printf("Frame %d/%d: payload=%d bytes, remaining=%d\n", i + 1,
                  NUM_FRAMES, (int)chunkSize, framesRemaining);

    offset += chunkSize;
  }

  // Verify total coverage
  Serial.printf("Total frames: %d, total payload bytes used: %u / %u\n",
                (int)printFrames.size(), (unsigned int)offset,
                (unsigned int)compressedLen);

  // Increment Job ID for next print
  currentJobId++;

  return true;
}

// Set example frames (for testing with hardcoded data)
void setExampleBitmapFrame() {
  printFrames.clear();

  printFrames.push_back({0x66, 0x35, 0x00, 0x1b, 0x2f, 0x03, 0x01, 0x00, 0x01,
                         0x00, 0x01, 0x33, 0x01, 0x55, 0x00, 0x03, 0x00, 0x02,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x03, 0x00, 0xff,
                         0x3f, 0xff, 0x28, 0x00, 0x00, 0x35, 0x2e, 0x00, 0x00,
                         0x38, 0xf3, 0x08, 0x03, 0x00, 0x00, 0x20, 0x00, 0x00,
                         0x00, 0x89, 0x2c, 0x00, 0x11, 0x00, 0x00, 0x63});

  printFrames.push_back(
      {0x66, 0x2f, 0x00, 0x1b, 0x2f, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x33,
       0x01, 0x55, 0x00, 0x02, 0x00, 0x02, 0x00, 0x38, 0x00, 0x00, 0x00, 0x80,
       0x00, 0x02, 0x03, 0x00, 0x00, 0x38, 0x00, 0xc3, 0x00, 0x03, 0x00, 0x00,
       0x20, 0x00, 0x00, 0x00, 0xc5, 0x2c, 0x00, 0x11, 0x00, 0x00, 0xb1});

  printFrames.push_back(
      {0x66, 0x2f, 0x00, 0x1b, 0x2f, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x33,
       0x01, 0x55, 0x00, 0x01, 0x00, 0x02, 0x00, 0x38, 0x00, 0x00, 0x00, 0x80,
       0x00, 0x02, 0x03, 0x00, 0x00, 0x38, 0x00, 0xc3, 0x00, 0x03, 0x00, 0x00,
       0x20, 0x00, 0x00, 0x00, 0xc5, 0x2c, 0x00, 0x11, 0x00, 0x00, 0xb2});

  printFrames.push_back(
      {0x66, 0x44, 0x00, 0x1b, 0x2f, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x33,
       0x01, 0x34, 0x00, 0x00, 0x00, 0x02, 0x00, 0x38, 0x00, 0x00, 0x00, 0x80,
       0x00, 0x02, 0x03, 0x00, 0x00, 0x38, 0x00, 0xc3, 0x00, 0x03, 0x00, 0x00,
       0x20, 0x00, 0x00, 0x08, 0x2f, 0x00, 0xff, 0x3f, 0xff, 0x28, 0x00, 0x00,
       0x35, 0x2c, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0xaa});

  Serial.printf("Loaded %d example frames\n", printFrames.size());
}

// ============================================================================
// BLE NOTIFICATION CALLBACK
// ============================================================================

void notifyCallback(NimBLERemoteCharacteristic *chr, uint8_t *data, size_t len,
                    bool isNotify) {
  // Store ACK
  lastAck.assign(data, data + len);
  ackReceived = true;

  Serial.print("Notification [");
  Serial.print(chr->getUUID().toString().c_str());
  Serial.print("] : ");
  for (size_t i = 0; i < len; ++i) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();

  // If printing, send next frame after ACK
  if (printingInProgress && chr == pNotifyChar) {
    delay(50); // Brief delay for printer buffer

    currentFrameIndex++;

    if (currentFrameIndex < printFrames.size()) {

      const auto &frame = printFrames[currentFrameIndex];
      pWriteChar->writeValue(frame.data(), frame.size(), false);

      Serial.printf("=== Frame %d (%d bytes) ===\n", currentFrameIndex + 1,
                    (int)frame.size());
      for (size_t j = 0; j < frame.size(); j++) {
        if (j % 16 == 0)
          Serial.println(); // break lines every 16 bytes
        Serial.printf("%02X ", frame[j]);
      }
      Serial.println("\n===========================");
    } else {
      Serial.println("All frames sent! Print job complete.");
      printingInProgress = false;
    }
  }
}

// ============================================================================
// PRINT JOB CONTROL
// ============================================================================

// Start printing the prepared frames
bool startPrintJob() {
  if (!pWriteChar) {
    Serial.println("No write characteristic available!");
    return false;
  }

  if (printFrames.empty()) {
    Serial.println("No frames prepared! Call prepareFramesFromBitmap() first.");
    return false;
  }

  currentFrameIndex = 0;
  printingInProgress = true;

  const auto &firstFrame = printFrames[0];
  pWriteChar->writeValue(firstFrame.data(), firstFrame.size(), false);

  Serial.printf("=== Frame 1 (%d bytes) ===\n", (int)firstFrame.size());
  for (size_t j = 0; j < firstFrame.size(); j++) {
    if (j % 16 == 0)
      Serial.println(); // break lines every 16 bytes
    Serial.printf("%02X ", firstFrame[j]);
  }
  Serial.println("\n===========================");

  return true;
}

// High-level: Prepare and print in one call
bool printBitmap(const Bitmap &userBitmap) {
  if (!prepareFramesFromBitmap(userBitmap)) {
    return false;
  }
  return startPrintJob();
}

// ============================================================================
// BLE SCANNER & CONNECTION
// ============================================================================

class PrinterAdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    Serial.print("Found device: ");
    Serial.println(advertisedDevice->toString().c_str());

    if (PRINTER_MAC[0] != '\0' &&
        advertisedDevice->getAddress().toString() == std::string(PRINTER_MAC)) {
      Serial.println("Found target printer, stopping scan...");
      NimBLEDevice::getScan()->stop();
    }
  }
};

void startScanner() {
  NimBLEScan *pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new PrinterAdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  pScan->start(5, false);
}

void startConnectionFindServices() {
  NimBLEAddress addr(std::string(PRINTER_MAC), BLE_ADDR_PUBLIC);
  pClient = NimBLEDevice::createClient();

  Serial.print("Connecting to printer: ");
  Serial.println(addr.toString().c_str());

  if (!pClient->connect(addr)) {
    Serial.println("Failed to connect.");
    return;
  }
  Serial.println("Connected!");

  uint16_t mtu = pClient->getMTU();
  Serial.printf("Negotiated MTU: %u\n", mtu);

  // Find printer service (UUID 0xABF0)
  pPrinterService = pClient->getService("ABF0");
  if (!pPrinterService) {
    Serial.println("Printer service (0xABF0) not found!");
    return;
  }
  Serial.println("Printer service found.");

  // Get write characteristic (ABF1)
  pWriteChar = pPrinterService->getCharacteristic("ABF1");
  if (!pWriteChar) {
    Serial.println("Write characteristic ABF1 not found!");
    return;
  }
  Serial.println("Write characteristic ABF1 found.");

  // Get notify characteristic (ABF2)
  pNotifyChar = pPrinterService->getCharacteristic("ABF2");
  if (!pNotifyChar) {
    Serial.println("Notify characteristic ABF2 not found!");
    return;
  }
  Serial.println("Notify characteristic ABF2 found.");

  // Subscribe to notifications
  if (pNotifyChar->canNotify()) {
    if (pNotifyChar->subscribe(true, notifyCallback)) {
      Serial.println("Subscribed to ABF2 notifications.");
    } else {
      Serial.println("Subscribe to ABF2 failed.");
    }
  }
}

void beginBLESniffer() {
  Serial.println("Starting BLE...");
  NimBLEDevice::init("ESP32-Printer");

  startScanner();
  if (PRINTER_MAC[0] != '\0') {
    startConnectionFindServices();
  }
}

// Check if printer is connected
bool isPrinterConnected() {
  return pClient != nullptr && pClient->isConnected();
}

// Check if currently printing
bool isPrinting() { return printingInProgress; }
