#include <ble_printer_manager.h>

// Printer constants
const uint8_t PRINTER_ID[8] = {0x1B, 0x2F, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01};
uint16_t mtu = 255;

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
std::vector<PrinterFrame> printFrames;
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

bool prepareFramesFromBitmap(const Bitmap &userBitmap) {

  printFrames = compressAndGenerateFrames(userBitmap, mtu);

  if (printFrames.empty()) {
    Serial.println("No frames generated!");
    return false;
  }

  Serial.printf("Total frames prepared: %d\n", printFrames.size());

  return true;
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

  if (printingInProgress && chr == pNotifyChar) {
    // Wait for ACK, then send next batch
    delay(100); // Give printer time to process

    if (currentFrameIndex < printFrames.size()) {
      sendFrameBatch(currentFrameIndex);
    } else {
      Serial.println("All frames sent! Print job complete.");
      printingInProgress = false;
    }
  }
}

// ============================================================================
// PRINT JOB CONTROL
// ============================================================================

void sendFrameBatch(int startIndex) {
  if (startIndex >= printFrames.size())
    return;

  Serial.println("=== Sending Frame Batch ===");

  // Send first frame (should be regular)
  const auto &firstFrame = printFrames[startIndex];
  if (!firstFrame.isContinuation) {
    pWriteChar->writeValue(firstFrame.data.data(), firstFrame.data.size(),
                           false);
    Serial.printf("Sent regular frame %d (%d bytes)\n", startIndex + 1,
                  (int)firstFrame.data.size());
    currentFrameIndex = startIndex + 1;

    // Send any continuation frames immediately
    while (currentFrameIndex < printFrames.size() &&
           printFrames[currentFrameIndex].isContinuation) {
      delay(20); // Small delay between continuation frames
      const auto &contFrame = printFrames[currentFrameIndex];
      pWriteChar->writeValue(contFrame.data.data(), contFrame.data.size(),
                             false);
      Serial.printf("Sent continuation frame %d (%d bytes)\n",
                    currentFrameIndex + 1, (int)contFrame.data.size());
      currentFrameIndex++;
    }

    Serial.println("=== Batch Complete ===");
  } else {
    Serial.println("Error: Expected regular frame at batch start!");
  }
}

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

  // Send first batch immediately
  sendFrameBatch(0);

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

  mtu = pClient->getMTU();
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
