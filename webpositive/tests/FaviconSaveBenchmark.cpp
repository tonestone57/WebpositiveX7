#include <stdio.h>
#include <vector>
#include <cstring>
#include <new>
#include <stdint.h>
#include <stdlib.h>

// Types
typedef int32_t int32;
typedef uint32_t uint32;
typedef uint8_t uint8;
typedef int64_t status_t;
typedef int64_t ssize_t;
typedef int64_t off_t;

#define B_OK 0
#define B_RGBA32_LITTLE 0

namespace FaviconSaveBenchmark {

// Mocks
class BFile {
public:
    BFile() : fWriteCount(0) {}

    ssize_t Write(const void* buffer, size_t size) {
        fWriteCount++;
        const uint8* byteBuf = (const uint8*)buffer;
        fData.insert(fData.end(), byteBuf, byteBuf + size);
        return (ssize_t)size;
    }

    void Unset() {}

    int GetWriteCount() const { return fWriteCount; }
    void Reset() { fWriteCount = 0; fData.clear(); }
    const std::vector<uint8>& GetData() const { return fData; }

private:
    std::vector<uint8> fData;
    int fWriteCount;
};

class BRect {
public:
    BRect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
    int32 IntegerWidth() const { return (int32)(right - left); }
    int32 IntegerHeight() const { return (int32)(bottom - top); }
    float left, top, right, bottom;
};

class BBitmap {
public:
    BBitmap(int32 width, int32 height, int32 bytesPerRow)
        : fWidth(width), fHeight(height), fBytesPerRow(bytesPerRow) {
        fBits = new uint8[height * bytesPerRow];
        // Fill with some pattern
        for (int i = 0; i < height * bytesPerRow; i++) {
            fBits[i] = (uint8)(i % 256);
        }
    }

    ~BBitmap() {
        delete[] fBits;
    }

    void* Bits() const { return fBits; }
    int32 BytesPerRow() const { return fBytesPerRow; }
    BRect Bounds() const { return BRect(0, 0, fWidth - 1, fHeight - 1); }

private:
    int32 fWidth;
    int32 fHeight;
    int32 fBytesPerRow;
    uint8* fBits;
};

// Implementations

void TestOriginal(BFile& file, BBitmap* icon) {
    int32 width = icon->Bounds().IntegerWidth() + 1;
    int32 height = icon->Bounds().IntegerHeight() + 1;

    // Simulate header writing (simplified for this benchmark, we care about the loop)
    // In real code:
    // int32 widthLE = B_HOST_TO_LENDIAN_INT32(width);
    // int32 heightLE = B_HOST_TO_LENDIAN_INT32(height);
    // file.Write(&widthLE, sizeof(widthLE));
    // file.Write(&heightLE, sizeof(heightLE));

    int32 bytesPerRow = icon->BytesPerRow();
    int32 rowLen = width * 4;
    uint8* bits = (uint8*)icon->Bits();

    if (bytesPerRow == rowLen) {
        file.Write(bits, height * rowLen);
    } else {
        for (int32 i = 0; i < height; i++)
            file.Write(bits + (i * bytesPerRow), rowLen);
    }
}

void TestOptimized(BFile& file, BBitmap* icon) {
    int32 width = icon->Bounds().IntegerWidth() + 1;
    int32 height = icon->Bounds().IntegerHeight() + 1;

    int32 bytesPerRow = icon->BytesPerRow();
    int32 rowLen = width * 4;
    uint8* bits = (uint8*)icon->Bits();

    if (bytesPerRow == rowLen) {
        file.Write(bits, height * rowLen);
    } else {
        // Optimized: Buffer and write once
        size_t totalDataSize = (size_t)height * rowLen;
        uint8* buffer = new(std::nothrow) uint8[totalDataSize];
        if (buffer) {
            for (int32 i = 0; i < height; i++) {
                memcpy(buffer + (i * rowLen), bits + (i * bytesPerRow), rowLen);
            }
            file.Write(buffer, totalDataSize);
            delete[] buffer;
        } else {
            // Fallback
            for (int32 i = 0; i < height; i++)
                file.Write(bits + (i * bytesPerRow), rowLen);
        }
    }
}

} // namespace FaviconSaveBenchmark

int main() {
    using namespace FaviconSaveBenchmark;
    int32 width = 16;
    int32 height = 16;

    // Test Case 1: Packed Bitmap (BytesPerRow == width * 4)
    {
        printf("Test Case 1: Packed Bitmap (Stride matches width)\n");
        BBitmap icon(width, height, width * 4);
        BFile fileOriginal;
        BFile fileOptimized;

        TestOriginal(fileOriginal, &icon);
        printf("Original Write Calls: %d\n", fileOriginal.GetWriteCount());

        TestOptimized(fileOptimized, &icon);
        printf("Optimized Write Calls: %d\n", fileOptimized.GetWriteCount());

        if (fileOriginal.GetData() == fileOptimized.GetData()) {
            printf("Verification: SUCCESS (Data matches)\n");
        } else {
            printf("Verification: FAILED (Data mismatch)\n");
        }
        printf("\n");
    }

    // Test Case 2: Padded Bitmap (BytesPerRow > width * 4)
    {
        printf("Test Case 2: Padded Bitmap (Stride > width)\n");
        int32 paddedRow = width * 4 + 8; // Extra 8 bytes padding
        BBitmap icon(width, height, paddedRow);
        BFile fileOriginal;
        BFile fileOptimized;

        TestOriginal(fileOriginal, &icon);
        printf("Original Write Calls: %d\n", fileOriginal.GetWriteCount());

        TestOptimized(fileOptimized, &icon);
        printf("Optimized Write Calls: %d\n", fileOptimized.GetWriteCount());

        if (fileOriginal.GetData() == fileOptimized.GetData()) {
            printf("Verification: SUCCESS (Data matches)\n");
        } else {
            printf("Verification: FAILED (Data mismatch)\n");
        }

        if (fileOptimized.GetWriteCount() == 1) {
             printf("Optimization Check: PASSED (1 Write call)\n");
        } else {
             printf("Optimization Check: FAILED (%d Write calls)\n", fileOptimized.GetWriteCount());
        }
        printf("\n");
    }

    return 0;
}
