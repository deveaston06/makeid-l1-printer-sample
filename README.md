# Reverse Engineering the MakeID L1 BLE Thermal Printer Protocol

This project aims to reverse engineer the Bluetooth LE (BLE) communication protocol of the MakeID L1 thermal printer. The primary goal is to understand its proprietary image compression algorithm to allow printing custom images from microcontrollers like the ESP32.

## The Core Challenge: Unknown Image Compression

The central problem is that the printer uses an unknown compression and/or encoding format for the bitmap data. While the overall communication flow and frame structure have been successfully analyzed, the payload containing the image data remains undecipherable.

**Primary Goal:** Identify the exact compression/encoding algorithm used for the image payload.

## Printer Details

*   **Model:** MakeID L1 (ID seen on advertisement: `L1C25E01553`)
*   **BLE Address:** `58:8C:81:72:AB:0A`
*   **BLE Services:**
    *   Service UUID: `0xABF0`
    *   Write Characteristic UUID: `0xABF1` (for sending print data)
    *   Notify Characteristic UUID: `0xABF2` (for receiving acknowledgements)

## Communication Protocol

### High-Level Flow

The official Android application sends an image to the printer in a series of chunks. The communication follows a strict write-and-response flow:

1.  The app splits a single print job into four data frames.
2.  The app sends one frame to the printer via a GATT Write operation on characteristic `0xABF1`.
3.  The printer processes the frame and sends a notification on characteristic `0xABF2`. This acts as an acknowledgement (ACK) and a signal that the printer is ready for the next frame.
4.  The app waits for this notification before sending the subsequent frame.

This flow is critical. Sending frames too quickly without waiting for the ACK results in a failed print, where only the first chunk is printed.

### Frame Structure

Each data frame sent to the printer has a consistent structure. All multi-byte values are Little Endian.

**Example Frame (53 bytes):**
```hex
66 35 00 1b 2f 03 01 00 01 00 01 33 01 55 00 03
00 02 00 00 00 00 00 3d 03 00 ff 3f ff 28 00 00
35 2e 00 00 38 f3 08 03 00 00 20 00 00 00 89 2c
00 11 00 00 63
```

**Breakdown:**

| Field                 | Example Value                      | Length | Description                                                                 |
| --------------------- | ---------------------------------- | ------ | --------------------------------------------------------------------------- |
| Magic Number          | `66`                               | 1 byte | Start of frame marker.                                                      |
| Frame Length          | `35 00`                            | 2 bytes| Total length of the frame (e.g., 0x0035 = 53 bytes).                        |
| Printer ID?           | `1b 2f 03 01 00 01 00 01`          | 8 bytes| Appears to be a constant identifier for the printer.                        |
| Job ID                | `33 01`                            | 2 bytes| Identifier for the current print job.                                       |
| Frame Magic           | `55`                               | 1 byte | `0x55` for the first three frames, changes on the final frame.              |
| Remaining Frames      | `00 03`                            | 2 bytes| A counter for remaining frames. Decrements from 3 down to 0.                |
| **Compressed Payload**| `00 02 ... 89 2c 00`               | Var.   | **The unknown compressed bitmap data.**                                     |
| End of Payload Marker | `11 00 00`                         | 3 bytes| Marks the end of the payload data.                                          |
| Checksum              | `63`                               | 1 byte | A checksum calculated over the entire frame (excluding the checksum byte itself). |

### Checksum Calculation

The checksum is calculated by subtracting each byte of the frame (except the last one) from a running total, with overflow. The algorithm was reconstructed from the reference APK.

```python
def getCheckNum(bArr):
    """
    Calculates the checksum for a given frame byte array.
    The input array should include a placeholder byte for the checksum.
    """
    b = 0
    # Iterate over all bytes except the last one (the checksum placeholder)
    for i in range(len(bArr) - 1):
        byte_val = bArr[i] & 0xFF
        b = (b - byte_val) & 0xFF
    return b
```
## Repository Contents

*   `/btsnoop_hci_logs`: Raw BLE traffic captures from five different print jobs, captured using Android's HCI snoop log feature and Wireshark.
*   `/pictures`: The original, uncompressed PNG images that were sent to the printer during the capture sessions.
*   `/src`: ESP32 source code (using PlatformIO and NimBLE) for connecting to the printer and replaying the captured, hardcoded data frames. This code successfully prints when respecting the ACK mechanism.
*   `/reference_apk`: The original MakeID-Life Android application package (`.xapk`) used for generating the print jobs.
*   `/include`: Header files for the ESP32 project **(please change MAC_ADDRESS to match yours)**.

## Development Environment

*   **Printer:** [MakeID L1 Thermal Printer](https://shopee.com.my/Makeid-L1-C-Label-Printer-Machine-with-Tape-Portable-Bluetooth-Thermal-Sticker-Maker-for-Home-Office-School(Waterproof-Oilproof-09mm-12mm-16mm-Width-Tape)-i.1560984266.44004392560?extraParams=%7B%22display_model_id%22%3A300372772623%2C%22model_selection_logic%22%3A3%7D&sp_atk=7531acc9-78f1-42f4-ae8f-ea3e5a169470&xptdk=7531acc9-78f1-42f4-ae8f-ea3e5a169470)
*   **Development Board:** NodeMCU-32S (ESP32)
*   **Framework:** PlatformIO with `board = nodemcu-32s`
*   **Bluetooth Stack:** `NimBLE-Arduino` (v2.3.6)
*   **Phone (for captures):** Samsung A50

*\*P.S.: Technology used for the MakeID L1 Thermal Printer in the Malaysian market is different (BLE instead of BT Classic)*
