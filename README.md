# MakeID L1 Printer Sample Log
I'm trying to reproduce an Android printer app’s BLE print protocol from captured hci_snoop logs and want help
1) identifying the bitmap compression/encoding and
2) implementing reliable ESP32 code that sends the frames (wait-for-notify handshake) so prints don’t stop halfway.
## Problem Description
I have a Bluetooth LE thermal printer (firmware advertises service 0xABF0, write char 0xABF1, notify char 0xABF2). The official Android app successfully prints images. I captured the phone to printer BLE traffic (using hci_snoop_hci.log and Wireshark). The app splits each print job into 4 GATT Write frames. Each frame starts with 0x66 and contains a header plus compressed/raster payload. After each Write the printer sends a notification (on ABF2) which I believe is the ACK/ready-for-next-chunk signal.

When I replay the frames from an ESP32 (NimBLE), I found:
1) If I send frames too fast the printer prints only the first chunk and then stops.
2) If I send one frame then wait for the ABF2 notification, then send the next, printing completes successfully.
I do not yet know the exact compression format the app uses. Tried PackBits / simple RLE and raw 1bpp tests — no match.

I want help with either:

1) Identifying the exact compression/encoding algorithm (so I can implement it on ESP32), or
2) Implementing a robust ESP32 client that reliably replays the exact captured frames in the correct order (waiting for notifications) and/or a compressor that matches the app.
## Environment / hardware
* Printer: Bluetooth LE thermal printer (address 58:8C:81:72:AB:0A, model id seen on advertisement L1C25E01553)
* Phone used for captures: Samsung (A50 / SamsungE_72...)
* Development board: NodeMCU-32S (ESP32)
* Framework: PlatformIO (board: nodemcu-32s), NimBLE-Arduino v2.3.6
* Serial monitor: 115200
## What I’ve captured & tried
1) Wireshark / hci_snoop captures
I captured two print jobs (same image but different mm heights): hci_snoop1.log and hci_snoop2.log. Each job contains 4 main Write Commands to the printer (handle 0x002a), with these high-level patterns (each frame has the same format):
- always starts with 66 (magic number),
- 35 00 is how many bytes this frame contains (first low byte, then high byte, 53 is the frame length in this case)
- 1b2f030100010001 could be the printer id
- 3301 is the printing job id
- 55 is another magic number (so far its same 55 for 3 first frames, and random seemingly on the last frame),
- 0003 is how many parts of 4 part frames are still left after this (this case is 3 frames left)
- 000200000000003d0300ff3fff280000352e000038f30803000020000000892c00 is the bitmap payload (unknown encoding and compression algo)
- 110000 signifies the end the part frame and start of checkNum
- 63 at the end is the checkNum (and function below in python is the algorithm).
```python
def getCheckNum(bArr):
    """
    Exact Java checksum algorithm
    Calculates checksum on all bytes except the last one
    """
    b = 0
    for i in range(len(bArr) - 1):
        byte_val = bArr[i] & 0xFF
        b = (b - byte_val) & 0xFF
    return b
```
2) Example frames (hex)

Example frame1 (taken from dump):
```hex
66 35 00 1b 2f 03 01 00 01 00 01 33 01 55 00 03
00 02 00 00 00 00 00 3d 03 00 ff 3f ff 28 00 00
35 2e 00 00 38 f3 08 03 00 00 20 00 00 00 89 2c
00 11 00 00 63
```
3) Observations
The second byte after 0x66 (e.g. 0x35 or 0x44) varies, seems to be the compressed payload length.

The sequence ... 0155 0003 / 0155 0002 / 0155 0001 / 0134 0000 appears in logs; I read this as a frame counter or stage marker.

Printer sends notifications on ABF2 after receiving frames. The notification payloads look like:
```hex
23 23 01 01 66 25 00 10 00 52 18 0F 00 00 ...
```
This appears to be an ACK/metadata packet.
\
I tried running simple decoders:
* PackBits (Mac) → produced output but not a validate bitmap
* Simple RLE ([count][value] & [value][count]) — outputs are large but not matching reference PNG
* raw 1bpp interpretation for common widths → no good match

4) Working ESP32 behavior
Using NimBLE I can connect, discover services & characteristics, subscribe to ABF2 notifications.

I can find ABF1 (write) and ABF2 (notify).

Sending frame1 then waiting for ABF2 notify, then frame2 etc. works (prints successfully). But I want to generate frames on-the-fly from images rather than replay hex dumps.

## What I need from the community

1. Compression identification help
Which compression algorithm does this look like? (RLE variant, PackBits, PackBits+bitpack, LZ? specifics please)
If you have a known implementation (Java or C/C++) that produces the exact byte patterns for 1bpp thermal images that match the bitstream format starting with 0x66 ..., please share or point to it.

2. ESP32 (NimBLE) implementation help

Example code that reliably writes frames to the printer char ABF1 and waits for the ABF2 notification before sending the next chunk. (I have code but would appreciate robust production-ready code with proper locking/MTU handling.)

If you can adapt the compressor into a compact C++ function suitable for ESP32 (PlatformIO, Arduino), that would be ideal.

3. Binary analysis hints

If you can help inspect the Android app (I can pull APK) for the encoding routine, I can provide the APK. If you’ve done APK static analysis and can point to likely class/method names, that would save time.

## Minimal reproducible code
NodeMCU-32S (ESP32) — connect, subscribe, send one frame then wait
```cpp
NimBLERemoteCharacteristic* pWrite = pService->getCharacteristic("ABF1");
NimBLERemoteCharacteristic* pNotify = pService->getCharacteristic("ABF2");

volatile bool gotNotify = false;
std::vector<uint8_t> lastNotify;

void notifyCallback(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
  lastNotify.assign(data, data + len);
  gotNotify = true;
}

bool sendFrameWaitAck(NimBLERemoteCharacteristic* writeChar,
                      const uint8_t* buf, size_t len,
                      NimBLERemoteCharacteristic* notifyChar,
                      uint32_t timeoutMs = 2000) {
  gotNotify = false;
  bool ok = writeChar->writeValue((uint8_t*)buf, len, false); // Write Command (no response)
  if (!ok) return false;
  uint32_t start = millis();
  while (!gotNotify && millis() - start < timeoutMs) delay(5);
  return gotNotify;
}
```
## Specific questions (answer these if you can)
1. Is the 0x66 prefix a known vendor packet wrapper for bitmap frames for any printer SDKs? (Which SDK / models use it?)
2. Which compression method best explains why a 44mm image has more bytes than a 45mm image? (I think RLE + 1bpp packing, but want exact mechanism.)
3. If you know the encoder code in Android (Java) that does this, can you point to the exact function/class? (APK is in the Github repo)
