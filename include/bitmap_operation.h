#include <functional>
#include <helper.h>

// ============================================================================
// BITMAP CREATION (Return new bitmap)
// ============================================================================

// Create new bitmaps
Bitmap createEmptyBitmap();
Bitmap createFilledBitmap(bool black);

// ============================================================================
// IN-PLACE OPERATIONS (Modify bitmap directly - no copies, no stack usage)
// ============================================================================

// Basic pixel operations
void setPixel(Bitmap &bitmap, int x, int y, bool black);
void clearPixel(Bitmap &bitmap, int x, int y);

// Fill operations
void fillBitmap(Bitmap &bitmap, bool black);
void clearBitmap(Bitmap &bitmap);

// Copy operation
void copyBitmap(Bitmap &dest, const Bitmap &src);

// Invert colors
void invertBitmap(Bitmap &bitmap);

// Higher-order function for custom drawing
void mapPixels(Bitmap &bitmap, int x1, int y1, int x2, int y2,
               std::function<bool(int, int)> predicate);

// Drawing primitives
void drawBorder(Bitmap &bitmap, int thickness);
void drawDiagonals(Bitmap &bitmap);
void drawChar(Bitmap &bitmap, char c, int startX, int startY);
void drawString(Bitmap &bitmap, const char *str, int startX, int startY);
void drawLine(Bitmap &bitmap, int x0, int y0, int x1, int y1);
void drawRect(Bitmap &bitmap, int x1, int y1, int x2, int y2);
void fillRect(Bitmap &bitmap, int x1, int y1, int x2, int y2);
void drawCircle(Bitmap &bitmap, int centerX, int centerY, int radius);
void fillCircle(Bitmap &bitmap, int centerX, int centerY, int radius);

// Patterns
void drawGrid(Bitmap &bitmap, int spacing);
void drawCheckerboard(Bitmap &bitmap, int squareSize);

// Helper functions (used internally)
bool isInBorder(int x, int y, int thickness);
bool isOnDiagonal(int x, int y);

// Composition (apply multiple operations in sequence)
void compose(Bitmap &bitmap, void (*f1)(Bitmap &) = nullptr,
             void (*f2)(Bitmap &) = nullptr, void (*f3)(Bitmap &) = nullptr);
