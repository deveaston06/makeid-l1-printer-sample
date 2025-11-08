#include <functional>
#include <helper.h>

Bitmap transformToColumnMajor(const Bitmap &bitmap);
Bitmap transform16BitSwap(const Bitmap &bitmap);
Bitmap transformInvertBits(const Bitmap &bitmap);
Bitmap transformToPrinterFormat(const Bitmap &bitmap);

// function that create bitmaps

Bitmap createEmptyBitmap();
Bitmap createFilledBitmap(bool black);
Bitmap setPixel(const Bitmap &bitmap, int x, int y, bool black);
Bitmap mapPixels(const Bitmap &bitmap, int x1, int y1, int x2, int y2,
                 std::function<bool(int, int)> predicate);
bool isInBorder(int x, int y, int thickness);
bool isOnDiagonal(int x, int y);
Bitmap drawBorder(const Bitmap &bitmap, int thickness);
Bitmap drawDiagonals(const Bitmap &bitmap);
Bitmap drawChar(const Bitmap &bitmap, char c, int startX, int startY);
Bitmap compose(const Bitmap &bitmap, Bitmap (*f1)(const Bitmap &),
               Bitmap (*f2)(const Bitmap &), Bitmap (*f3)(const Bitmap &));
