#include <NimBLEDevice.h>
#include <array_helper.h>

#ifndef CREDENTIALS
#include <credentials.h>
#endif // !MAC_ADDRESS

class PrinterAdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
public:
  PrinterAdvertisedDeviceCallbacks(NimBLEClient *client) { pClient = client; }

private:
  NimBLEClient *pClient;
  void onResult(NimBLEAdvertisedDevice *advertisedDevice);
};

void beginBLESniffer();
void startPrintJob();
void setBitmapFrame(std::vector<uint8_t> framecontent);
void setExampleBitmapFrame();