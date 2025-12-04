#ifndef BITMAP_OPERATION_H
#define BITMAP_OPERATION_H

#include <functional>
#include <helper.h>

// ============================================================================
// BITMAP CREATION
// ============================================================================

// Create empty bitmap - returns pointer (caller must delete)
Bitmap *createEmptyBitmapPtr();

// Stack-friendly versions (for small scope usage only)
Bitmap createEmptyBitmap();
Bitmap createFilledBitmap(bool black);

// ============================================================================
// IN-PLACE OPERATIONS
// ============================================================================

void setPixel(Bitmap &bitmap, int x, int y, bool black);
void clearPixel(Bitmap &bitmap, int x, int y);
void fillBitmap(Bitmap &bitmap, bool black);
void clearBitmap(Bitmap &bitmap);
void mapPixels(Bitmap &bitmap, int x1, int y1, int x2, int y2,
               std::function<bool(int, int)> predicate);

// Drawing functions
void drawBorder(Bitmap &bitmap, int thickness);
void drawDiagonals(Bitmap &bitmap);
void drawChar(Bitmap &bitmap, char c, int startX, int startY);
void drawString(Bitmap &bitmap, const char *str, int startX, int startY);
void drawRect(Bitmap &bitmap, int x1, int y1, int x2, int y2);
void fillRect(Bitmap &bitmap, int x1, int y1, int x2, int y2);
void drawLine(Bitmap &bitmap, int x0, int y0, int x1, int y1);
void drawCircle(Bitmap &bitmap, int centerX, int centerY, int radius);
void fillCircle(Bitmap &bitmap, int centerX, int centerY, int radius);
void invertBitmap(Bitmap &bitmap);
void copyBitmap(Bitmap &dest, const Bitmap &src);
void drawGrid(Bitmap &bitmap, int spacing);
void drawCheckerboard(Bitmap &bitmap, int squareSize);

// Composition
void compose(Bitmap &bitmap, void (*f1)(Bitmap &), void (*f2)(Bitmap &),
             void (*f3)(Bitmap &));

#endif // !BITMAP_OPERATION_H
