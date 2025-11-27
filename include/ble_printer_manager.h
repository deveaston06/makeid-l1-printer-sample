#include <NimBLEDevice.h>
#include <credentials.h>
#include <helper.h>
#include <image_compressor.h>
#include <vector>

// ============================================================================
// BLE INITIALIZATION
// ============================================================================

// Initialize BLE, scan for printer, and connect
void beginBLESniffer();

// ============================================================================
// PRINT JOB MANAGEMENT
// ============================================================================

// Start printing the prepared frames
bool startPrintJob();

// High-level: Prepare and print in one call
bool printBitmap(const Bitmap &userBitmap);

// Load example frames (for testing with hardcoded data)
void setExampleBitmapFrame();

// ============================================================================
// STATUS QUERIES
// ============================================================================

// Check if printer is connected via BLE
bool isPrinterConnected();

// Check if a print job is currently in progress
bool isPrinting();

// ============================================================================
// FRAME UTILITIES (Advanced)
// ============================================================================

// Calculate checksum for a frame
uint8_t calculateFrameChecksum(const uint8_t *data, size_t len);

// Create a single BLE frame from compressed data chunk
std::vector<uint8_t> createFrame(const uint8_t *compressedData, size_t offset,
                                 size_t chunkSize, uint16_t jobId,
                                 uint16_t framesRemaining, bool isFinal);
