#include <ble_printer_manager.h>

// Printer constants
const uint8_t PRINTER_ID[8] = {0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01};

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

// ============================================================================
// PRINT JOB MANAGEMENT
// ============================================================================

// Prepare frames from compressed bitmap data
bool prepareFramesFromBitmap(const Bitmap &userBitmap) {
  // Clear previous frames
  printFrames.clear();

  // Use the new Python-style compression
  auto frames = compressAndGenerateFrames(userBitmap);

  if (frames.empty()) {
    Serial.println("No frames generated!");
    return false;
  }

  // Convert to the existing frame format
  for (const auto &frame : frames) {
    printFrames.push_back(frame.data);
  }

  Serial.printf("Total frames prepared: %d\n", printFrames.size());

  return true;
}

// Set example frames (for testing with hardcoded data)
void setExampleBitmapFrame() {
  printFrames.clear();

  // Frame 1 - 62 bytes
  printFrames.push_back({0x66, 0x3E, 0x00, 0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01,
                         0x00, 0x01, 0x23, 0x01, 0x55, 0x00, 0x03, 0x00, 0x02,
                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x16, 0x13, 0x00,
                         0x00, 0x1F, 0x00, 0xC2, 0x00, 0xF8, 0x00, 0x20, 0x00,
                         0x00, 0x00, 0x86, 0x2C, 0x00, 0x0D, 0x00, 0x00, 0xF8,
                         0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0xF8, 0x00, 0x11, 0x00, 0x00, 0x72});

  // Frame 2 - 61 bytes
  printFrames.push_back({0x66, 0x3D, 0x00, 0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01,
                         0x00, 0x01, 0x23, 0x01, 0x55, 0x00, 0x02, 0x00, 0x03,
                         0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00, 0xF8,
                         0x00, 0x20, 0x00, 0x00, 0x00, 0xBE, 0x2C, 0x00, 0x00,
                         0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00,
                         0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0xF8, 0x00, 0x11, 0x00, 0x00, 0xEA});

  // Frame 3 - 61 bytes
  printFrames.push_back({0x66, 0x3D, 0x00, 0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01,
                         0x00, 0x01, 0x23, 0x01, 0x55, 0x00, 0x01, 0x00, 0x03,
                         0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00, 0xF8,
                         0x00, 0x20, 0x00, 0x00, 0x00, 0xBE, 0x2C, 0x00, 0x00,
                         0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00,
                         0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0xF8, 0x00, 0x11, 0x00, 0x00, 0xEB});

  // Frame 4 - 58 bytes
  printFrames.push_back(
      {0x66, 0x3A, 0x00, 0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x23,
       0x01, 0x24, 0x00, 0x00, 0x00, 0x03, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00,
       0x62, 0x00, 0xF8, 0x00, 0x20, 0x00, 0x48, 0x2D, 0x00, 0xFF, 0x20, 0x0B,
       0x00, 0x00, 0x0C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x11, 0x00, 0x00, 0x7F});

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
