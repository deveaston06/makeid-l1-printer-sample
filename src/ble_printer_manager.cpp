#include "ble_printer_manager.h"

const std::vector<uint8_t> frameheader = {0x66, 0x35, 0x00, 0x1b, 0x2f, 0x03,
                                          0x01, 0x00, 0x01, 0x00, 0x01, 0x33,
                                          0x01, 0x55, 0x00, 0x03};

NimBLERemoteService *pPrinterService = nullptr;
NimBLEClient *pClient = nullptr;
// globals
NimBLERemoteCharacteristic *pWriteChar;
NimBLERemoteCharacteristic *pNotifyChar;
volatile bool ackReceived = false;
std::vector<uint8_t> lastAck; // stores last notification bytes
int currentFrame = 0;
bool printingInProgress = false;
std::vector<uint8_t> frame1;
std::vector<uint8_t> frame2;
std::vector<uint8_t> frame3;
std::vector<uint8_t> frame4;

void setExampleBitmapFrame() {
  frame1 = {0x66, 0x35, 0x00, 0x1b, 0x2f, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01,
            0x33, 0x01, 0x55, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x3d, 0x03, 0x00, 0xff, 0x3f, 0xff, 0x28, 0x00, 0x00, 0x35,
            0x2e, 0x00, 0x00, 0x38, 0xf3, 0x08, 0x03, 0x00, 0x00, 0x20, 0x00,
            0x00, 0x00, 0x89, 0x2c, 0x00, 0x11, 0x00, 0x00, 0x63};
  frame2 = {0x66, 0x2f, 0x00, 0x1b, 0x2f, 0x03, 0x01, 0x00, 0x01, 0x00,
            0x01, 0x33, 0x01, 0x55, 0x00, 0x02, 0x00, 0x02, 0x00, 0x38,
            0x00, 0x00, 0x00, 0x80, 0x00, 0x02, 0x03, 0x00, 0x00, 0x38,
            0x00, 0xc3, 0x00, 0x03, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
            0xc5, 0x2c, 0x00, 0x11, 0x00, 0x00, 0xb1};
  frame3 = {0x66, 0x2f, 0x00, 0x1b, 0x2f, 0x03, 0x01, 0x00, 0x01, 0x00,
            0x01, 0x33, 0x01, 0x55, 0x00, 0x01, 0x00, 0x02, 0x00, 0x38,
            0x00, 0x00, 0x00, 0x80, 0x00, 0x02, 0x03, 0x00, 0x00, 0x38,
            0x00, 0xc3, 0x00, 0x03, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
            0xc5, 0x2c, 0x00, 0x11, 0x00, 0x00, 0xb2};
  frame4 = {0x66, 0x44, 0x00, 0x1b, 0x2f, 0x03, 0x01, 0x00, 0x01, 0x00,
            0x01, 0x33, 0x01, 0x34, 0x00, 0x00, 0x00, 0x02, 0x00, 0x38,
            0x00, 0x00, 0x00, 0x80, 0x00, 0x02, 0x03, 0x00, 0x00, 0x38,
            0x00, 0xc3, 0x00, 0x03, 0x00, 0x00, 0x20, 0x00, 0x00, 0x08,
            0x2f, 0x00, 0xff, 0x3f, 0xff, 0x28, 0x00, 0x00, 0x35, 0x2c,
            0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0xaa};
}

void setBitmapFrame(
    std::vector<uint8_t> framecontent) { // TODO: need help with this
  frame1 = frameheader;
  const std::vector<uint8_t> frame1content = {0x00, 0x02, 0x00};
  std::copy(std::begin(frameheader), std::end(frameheader), std::begin(frame1));
  pushArrayToAnother(frame1content, frame1);
}

// Notification callback
void notifyCallback(NimBLERemoteCharacteristic *chr, uint8_t *data, size_t len,
                    bool isNotify) {
  // copy ack
  lastAck.assign(data, data + len);
  ackReceived = true;

  Serial.print("Notification [");
  Serial.print(chr->getUUID().toString().c_str());
  Serial.print("] : ");
  for (size_t i = 0; i < len; ++i)
    Serial.printf("%02X ", data[i]);
  Serial.println();

  // When printing, use each notification as a signal to send the next frame
  if (printingInProgress && chr == pNotifyChar) {
    delay(50); // slight delay to allow internal buffer clear
    currentFrame++;
    const uint8_t *nextFrame = nullptr;
    size_t lenNext = 0;

    switch (currentFrame) {
    case 1:
      nextFrame = &frame2[0];
      lenNext = frame2.size();
      break;
    case 2:
      nextFrame = &frame3[0];
      lenNext = frame3.size();
      break;
    case 3:
      nextFrame = &frame4[0];
      lenNext = frame4.size();
      break;
    default:
      Serial.println("All frames sent. Printing should complete.");
      printingInProgress = false;
      return;
    }

    Serial.printf("Sending frame %d...\n", currentFrame + 1);
    Serial.printf("%s", nextFrame);
    pWriteChar->writeValue(nextFrame, lenNext, false);
  }
}

void startPrintJob() {
  if (!pWriteChar) {
    Serial.println("No write characteristic available!");
    return;
  }
  currentFrame = 0;
  printingInProgress = true;
  Serial.println("Starting print job (frame 1)...");
  pWriteChar->writeValue(&frame1[0], frame1.size(), false);
}

void startScanner() {
  // Scan
  NimBLEScan *pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new PrinterAdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  pScan->start(5, false);

  if (PRINTER_MAC[0] == '\0')
    return;
}

void startConnectionFindServices() {
  // Connect
  NimBLEAddress addr(std::string(PRINTER_MAC), BLE_ADDR_PUBLIC);
  pClient = NimBLEDevice::createClient();

  Serial.print("Connecting to printer: ");
  Serial.println(addr.toString().c_str());

  if (!pClient->connect(addr)) {
    Serial.println("Failed to connect.");
    return;
  }
  Serial.println("Connected!");

  // Negotiate MTU
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

  // Subscribe to ABF2 notifications
  if (pNotifyChar->canNotify()) {
    if (pNotifyChar->subscribe(true, notifyCallback)) {
      Serial.println("Subscribed to ABF2 notifications.");
    } else {
      Serial.println("Subscribe to ABF2 failed.");
    }
  }
}

void beginBLESniffer() {
  Serial.println("Starting BLE sniffer...");
  NimBLEDevice::init("ESP32-BLE-Sniffer");

  startScanner();
  if (PRINTER_MAC[0] != '\0')
    startConnectionFindServices();
}
