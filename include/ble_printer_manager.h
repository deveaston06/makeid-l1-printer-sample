#include <NimBLEDevice.h>
#include <array_helper.h>

#ifndef CREDENTIALS
#include <credentials.h>
#endif // !MAC_ADDRESS

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

void beginBLESniffer();
void startPrintJob();
void setBitmapFrame(std::vector<uint8_t> framecontent);
void setExampleBitmapFrame();
