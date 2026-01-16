#include <stdio.h>
#include <OS.h>
#include <Message.h>
#include <File.h>
#include <Directory.h>
#include <Path.h>
#include <FindDirectory.h>
#include <vector>
#include <string>

// Mock DownloadWindow environment for benchmark
class DownloadProgressViewStub {
public:
    DownloadProgressViewStub(int id) : fId(id) {}
    status_t SaveSettings(BMessage* archive) {
        archive->AddInt32("id", fId);
        archive->AddString("path", "/boot/home/Downloads/file.zip");
        archive->AddString("url", "http://example.com/file.zip");
        archive->AddFloat("value", 50.0);
        return B_OK;
    }
private:
    int fId;
};

// Simplified synchronous save
void SyncSave(const std::vector<DownloadProgressViewStub*>& views) {
    BFile file("bench_test_sync.settings", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
    BMessage message;
    for (auto* view : views) {
        BMessage downloadArchive;
        if (view->SaveSettings(&downloadArchive) == B_OK)
            message.AddMessage("download", &downloadArchive);
    }
    message.Flatten(&file);
}

// Simplified throttled/async save simulation
// In real code we'd use BMessageRunner and threads
void AsyncSaveStub(const std::vector<DownloadProgressViewStub*>& views) {
    // Gather phase (Main Thread)
    BMessage message;
    for (auto* view : views) {
        BMessage downloadArchive;
        if (view->SaveSettings(&downloadArchive) == B_OK)
            message.AddMessage("download", &downloadArchive);
    }

    // Save phase (Background Thread)
    // We simulate thread creation overhead? Or just measure flattened time?
    // In optimized version, the UI thread only does the gathering.
    // The flattening happens elsewhere.
}

int main() {
    std::vector<DownloadProgressViewStub*> views;
    for (int i = 0; i < 100; i++) {
        views.push_back(new DownloadProgressViewStub(i));
    }

    bigtime_t start = system_time();
    for (int i = 0; i < 50; i++) {
        SyncSave(views);
    }
    bigtime_t end = system_time();
    printf("Synchronous Save (50 iterations, 100 items): %lld us\n", end - start);

    // Optimized: Gather only on main thread
    start = system_time();
    for (int i = 0; i < 50; i++) {
        // Gather
        BMessage message;
        for (auto* view : views) {
            BMessage downloadArchive;
            if (view->SaveSettings(&downloadArchive) == B_OK)
                message.AddMessage("download", &downloadArchive);
        }
        // Flatten/Save would be async, so we don't count it for UI thread block time
    }
    end = system_time();
    printf("Optimized Save (Main Thread Cost): %lld us\n", end - start);

    // Cleanup
    for (auto* view : views) delete view;
    return 0;
}
