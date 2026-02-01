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

namespace FaviconBenchmark {

// Mocks
class BFile {
public:
    BFile(const std::vector<uint8>& data) : fData(data), fPos(0), fReadCount(0) {}

    ssize_t Read(void* buffer, size_t size) {
        fReadCount++;
        if (fPos >= fData.size()) return 0;

        size_t available = fData.size() - fPos;
        if (size > available) size = available;

        memcpy(buffer, fData.data() + fPos, size);
        fPos += size;
        return (ssize_t)size;
    }

    void Seek(off_t pos) {
        fPos = pos;
    }

    int GetReadCount() const { return fReadCount; }
    void ResetCount() { fReadCount = 0; }

private:
    std::vector<uint8> fData;
    size_t fPos;
    int fReadCount;
};

class BBitmap {
public:
    BBitmap(int32 width, int32 height, int32 bytesPerRow)
        : fWidth(width), fHeight(height), fBytesPerRow(bytesPerRow) {
        fBits = new uint8[height * bytesPerRow];
        memset(fBits, 0, height * bytesPerRow);
    }

    ~BBitmap() {
        delete[] fBits;
    }

    void* Bits() const { return fBits; }
    int32 BytesPerRow() const { return fBytesPerRow; }
    int32 BitsLength() const { return fHeight * fBytesPerRow; }

    // For verification
    uint8* GetBits() { return fBits; }

private:
    int32 fWidth;
    int32 fHeight;
    int32 fBytesPerRow;
    uint8* fBits;
};

// Implementations

void TestOriginal(BFile& file, BBitmap* icon, int32 width, int32 height) {
    int32 bytesPerRow = icon->BytesPerRow();
    int32 rowLen = width * 4;
    uint8* bits = (uint8*)icon->Bits();
    bool success = true;
    for (int32 i = 0; i < height; i++) {
        if (file.Read(bits + (i * bytesPerRow), rowLen) != rowLen) {
            success = false;
            break;
        }
    }
}

void TestOptimized(BFile& file, BBitmap* icon, int32 width, int32 height) {
    size_t totalDataSize = (size_t)width * height * 4;

    if (icon->BytesPerRow() == width * 4) {
        // Optimized: Read directly into bitmap if packed
        if (file.Read(icon->Bits(), totalDataSize) == (ssize_t)totalDataSize) {
            // Success
        } else {
            // Failure
        }
    } else {
        // Optimized: Read into buffer then copy row-by-row if padded
        uint8* buffer = new(std::nothrow) uint8[totalDataSize];
        if (buffer) {
            if (file.Read(buffer, totalDataSize) == (ssize_t)totalDataSize) {
                uint8* bits = (uint8*)icon->Bits();
                int32 bytesPerRow = icon->BytesPerRow();
                int32 rowLen = width * 4;
                for (int32 i = 0; i < height; i++) {
                    memcpy(bits + (i * bytesPerRow), buffer + (i * rowLen), rowLen);
                }
            }
            delete[] buffer;
        } else {
            // Fallback to original loop if allocation fails
            int32 bytesPerRow = icon->BytesPerRow();
            int32 rowLen = width * 4;
            uint8* bits = (uint8*)icon->Bits();
            for (int32 i = 0; i < height; i++) {
                if (file.Read(bits + (i * bytesPerRow), rowLen) != rowLen) {
                    break;
                }
            }
        }
    }
}

} // namespace FaviconBenchmark

int main() {
    using namespace FaviconBenchmark;
    int32 width = 16;
    int32 height = 16;

    // Create test data (packed)
    std::vector<uint8> fileData;
    for (int i = 0; i < width * height * 4; i++) {
        fileData.push_back((uint8)(i % 256));
    }

    // Test Case 1: Packed Bitmap (BytesPerRow == width * 4)
    {
        printf("Test Case 1: Packed Bitmap (Stride matches width)\n");
        BFile file(fileData);
        BBitmap iconOriginal(width, height, width * 4);
        BBitmap iconOptimized(width, height, width * 4);

        file.Seek(0);
        file.ResetCount();
        TestOriginal(file, &iconOriginal, width, height);
        printf("Original Reads: %d\n", file.GetReadCount());

        file.Seek(0);
        file.ResetCount();
        TestOptimized(file, &iconOptimized, width, height);
        printf("Optimized Reads: %d\n", file.GetReadCount());

        // Verify Content
        if (memcmp(iconOriginal.GetBits(), iconOptimized.GetBits(), iconOriginal.BitsLength()) == 0) {
            printf("Verification: SUCCESS\n");
        } else {
            printf("Verification: FAILED\n");
        }
        printf("\n");
    }

    // Test Case 2: Padded Bitmap (BytesPerRow > width * 4)
    {
        printf("Test Case 2: Padded Bitmap (Stride > width)\n");
        int32 paddedRow = width * 4 + 8; // Extra 8 bytes padding
        BFile file(fileData);
        BBitmap iconOriginal(width, height, paddedRow);
        BBitmap iconOptimized(width, height, paddedRow);

        file.Seek(0);
        file.ResetCount();
        TestOriginal(file, &iconOriginal, width, height);
        printf("Original Reads: %d\n", file.GetReadCount());

        file.Seek(0);
        file.ResetCount();
        TestOptimized(file, &iconOptimized, width, height);
        printf("Optimized Reads: %d\n", file.GetReadCount());

        // Verify Content
        if (memcmp(iconOriginal.GetBits(), iconOptimized.GetBits(), iconOriginal.BitsLength()) == 0) {
            printf("Verification: SUCCESS\n");
        } else {
            printf("Verification: FAILED\n");
        }
        printf("\n");
    }

    return 0;
}
