#ifndef _MOCK_BITMAP_H
#define _MOCK_BITMAP_H
#include "SupportDefs.h"

enum color_space { B_CMAP8, B_RGBA32, B_RGB32 };
enum { B_BITMAP_NO_SERVER_LINK = 0 };

class BRect {
public:
    BRect(float l, float t, float r, float b) {}
    int32 IntegerWidth() const { return 0; }
};

class BBitmap {
public:
    BBitmap(BRect bounds, uint32 flags, color_space space) {}
    BBitmap(BRect bounds, color_space space) {}
    status_t InitCheck() { return B_OK; }
    void* Bits() const { return (void*)buffer; }
    int32 BitsLength() const { return 0; }
    void SetBits(const void* data, int32 length, int32 offset, color_space space) {}
    status_t ImportBits(const void* data, int32 length, int32 bpr, int32 offset, color_space space) { return B_OK; }
    int32 BytesPerRow() const { return 0; }

    char buffer[100];
};
#endif
