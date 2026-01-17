#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/time.h>
#include <algorithm>
#include <cstdint>

// --- Mocks ---

class BMenu;

class BString {
public:
    std::string s;
    BString() {}
    BString(const char* str) : s(str ? str : "") {}
    BString(const std::string& str) : s(str) {}

    const char* String() const { return s.c_str(); }
    int Length() const { return (int)s.length(); }
    void Truncate(int len) { if (len < (int)s.length()) s.resize(len); }
    bool operator<(const BString& other) const { return s < other.s; }
    bool operator==(const BString& other) const { return s == other.s; }
};

class BMessage {
public:
    BMessage(int what) {}
    void AddString(const char* name, const char* val) {}
};

class BMenuItem {
public:
    BMenuItem(BString label, BMessage* msg) : fLabel(label), fMsg(msg), fSubmenu(NULL) {}
    BMenuItem(void* menu, BMessage* msg) : fLabel(""), fMsg(msg), fSubmenu((BMenu*)menu) {}
    ~BMenuItem() { delete fMsg; /* delete fSubmenu? ownership rules vary */ }

    const char* Label() const { return fLabel.String(); }
    BMenu* Submenu() const { return fSubmenu; }
    void SetEnabled(bool) {}

private:
    BString fLabel;
    BMessage* fMsg;
    BMenu* fSubmenu;
};

class BMenu {
public:
    std::vector<BMenuItem*> items;
    std::string name;

    BMenu(const char* n) : name(n) {}
    virtual ~BMenu() {
        for (size_t i = 0; i < items.size(); i++) delete items[i];
    }

    void AddItem(BMenuItem* item) { items.push_back(item); }
    void AddItem(BMenuItem* item, int index) {
        if (index >= 0 && index <= (int)items.size())
            items.insert(items.begin() + index, item);
        else
            items.push_back(item);
    }
    void AddItem(BMenu* menu) {
        // In BeAPI adding a menu adds a specific MenuItem wrapping it
        AddItem(new BMenuItem(menu, new BMessage(0)));
    }

    BMenuItem* RemoveItem(int index) {
        if (index < 0 || index >= (int)items.size()) return NULL;
        BMenuItem* item = items[index];
        items.erase(items.begin() + index);
        return item;
    }

    int32_t CountItems() const { return (int32_t)items.size(); }
    BMenuItem* ItemAt(int32_t index) const {
        if (index < 0 || index >= (int32_t)items.size()) return NULL;
        return items[index];
    }

    void AddSeparatorItem() {}
};

class BTime {
public:
    BTime(int, int, int) {}
};

class BDate {
public:
    long days;
    BDate() : days(0) {}
    void AddDays(int d) { days += d; }
    BString LongDayName() { return "Some Day"; }
};

class BDateTime {
public:
    long val; // abstract time
    BDate date;

    BDateTime(long v = 0) : val(v) {}

    static BDateTime CurrentDateTime(int) {
        return BDateTime(1000000);
    }

    void SetTime(const BTime&) {}
    BDate& Date() { return date; }

    bool operator<(const BDateTime& other) const { return val < other.val; }
    bool operator!=(const BDateTime& other) const { return val != other.val; }
};

#define B_LOCAL_TIME 0
#define B_TRUNCATE_END 0
#define B_TRANSLATE(x) x

class BFont {
public:
    void TruncateString(BString* inOut, int mode, float width) {}
};
BFont* be_plain_font = new BFont();

// BrowsingHistory Mocks

class BrowsingHistoryItem {
public:
    BrowsingHistoryItem(BString url, BDateTime dt) : fURL(url), fDateTime(dt) {}
    const BString& URL() const { return fURL; }
    const BDateTime& DateTime() const { return fDateTime; }
private:
    BString fURL;
    BDateTime fDateTime;
};

class BrowsingHistory {
public:
    std::vector<BrowsingHistoryItem> items;

    // Modification: Generation counter
    uint32_t fGeneration;

    BrowsingHistory() : fGeneration(0) {}

    static BrowsingHistory* DefaultInstance() {
        static BrowsingHistory instance;
        return &instance;
    }

    bool Lock() { return true; }
    void Unlock() {}

    int32_t CountItems() const { return (int32_t)items.size(); }
    BrowsingHistoryItem HistoryItemAt(int32_t index) const { return items[index]; }

    void AddItem(BrowsingHistoryItem item) {
        items.push_back(item);
        fGeneration++;
    }

    uint32_t Generation() const { return fGeneration; }
};

// Helper function from original code
static BString baseURL(BString label) {
    return label; // Simplified
}

static void addItemToMenuOrSubmenu(BMenu* menu, BMenuItem* newItem) {
    // Simplified version of logic
    menu->AddItem(newItem);
}

static void addOrDeleteMenu(BMenu* menu, BMenu* toMenu) {
    if (menu->CountItems() > 0)
        toMenu->AddItem(menu);
    else
        delete menu;
}


class BrowserWindowBenchmark {
public:
    BMenu* fHistoryMenu;
    int32_t fHistoryMenuFixedItemCount;

    // Optimization fields
    uint32_t fLastHistoryGeneration;
    BDateTime fLastHistoryMenuDate;

    BrowserWindowBenchmark() {
        fHistoryMenu = new BMenu("History");
        // Add some fixed items
        fHistoryMenu->AddItem(new BMenuItem("Back", NULL));
        fHistoryMenu->AddItem(new BMenuItem("Forward", NULL));
        fHistoryMenuFixedItemCount = fHistoryMenu->CountItems();

        fLastHistoryGeneration = 0;
        fLastHistoryMenuDate = BDateTime(0);
    }

    ~BrowserWindowBenchmark() {
        delete fHistoryMenu;
    }

    void _UpdateHistoryMenu_Original() {
        BMenuItem* menuItem;
        // Removing items one by one is O(N) in BList if removing from middle,
        // but here we are removing from end or start?
        // RemoveItem(index) shifts items. Doing this in a loop is O(N^2) if removing from 0.
        // Wait, fHistoryMenu->RemoveItem(fHistoryMenuFixedItemCount).
        // If items shift down, we are always removing the item at 'fixed' index.
        // So we remove the first dynamic item repeatedly.
        // This causes N shifts of remaining items. Total O(N^2).
        while ((menuItem = fHistoryMenu->RemoveItem(fHistoryMenuFixedItemCount)))
            delete menuItem;

        BrowsingHistory* history = BrowsingHistory::DefaultInstance();
        if (!history->Lock())
            return;

        int32_t count = history->CountItems();
        BMenuItem* clearHistoryItem = new BMenuItem(B_TRANSLATE("Clear history"),
            new BMessage(0));
        clearHistoryItem->SetEnabled(count > 0);
        fHistoryMenu->AddItem(clearHistoryItem);
        if (count == 0) {
            history->Unlock();
            return;
        }
        fHistoryMenu->AddSeparatorItem();

        BDateTime todayStart = BDateTime::CurrentDateTime(B_LOCAL_TIME);
        // ... Date calc logic mocked ...

        // Create buckets
        BMenu* buckets[7];
        for(int i=0; i<7; i++) buckets[i] = new BMenu("Bucket");

        for (int32_t i = 0; i < count; i++) {
            BrowsingHistoryItem historyItem = history->HistoryItemAt(i);
            BMessage* message = new BMessage(0);
            message->AddString("url", historyItem.URL().String());

            BString truncatedUrl(historyItem.URL());
            // be_plain_font->TruncateString ...

            menuItem = new BMenuItem(truncatedUrl, message);

            // Simplified bucket logic
            addItemToMenuOrSubmenu(buckets[0], menuItem);
        }
        history->Unlock();

        for(int i=0; i<7; i++) addOrDeleteMenu(buckets[i], fHistoryMenu);
    }

    void _UpdateHistoryMenu_Optimized() {
        BrowsingHistory* history = BrowsingHistory::DefaultInstance();
        BDateTime today = BDateTime::CurrentDateTime(B_LOCAL_TIME);

        // Optimization Check
        // Note: In real code, we should also check if we have built the menu at least once.
        // Or if fHistoryMenu has items (beyond fixed ones).
        if (history->Generation() == fLastHistoryGeneration &&
            fLastHistoryMenuDate.val == today.val && // Simplified date check
            fHistoryMenu->CountItems() > fHistoryMenuFixedItemCount) {
            return;
        }

        // If we proceed, update markers
        fLastHistoryGeneration = history->Generation();
        fLastHistoryMenuDate = today;

        // ... Existing logic ...
        _UpdateHistoryMenu_Original();
        // Note: The original logic in real code would be here.
        // Since I called Original(), I'm effectively measuring the cost of the rebuild
        // when the check fails. When check passes, I return early.
    }
};

int main() {
    BrowsingHistory* history = BrowsingHistory::DefaultInstance();

    // Populate History
    int numItems = 2000;
    printf("Populating history with %d items...\n", numItems);
    for(int i=0; i<numItems; i++) {
        char buf[100];
        sprintf(buf, "http://www.example.com/page/%d", i);
        history->AddItem(BrowsingHistoryItem(buf, BDateTime(1000000 - i)));
    }

    BrowserWindowBenchmark app;

    // Warmup / Initial State
    app._UpdateHistoryMenu_Original();
    printf("Menu Item Count after build: %d\n", app.fHistoryMenu->CountItems());

    struct timeval start, end;

    // Benchmark Original (Rebuilds every time)
    printf("Benchmarking Original (100 iterations)...\n");
    gettimeofday(&start, NULL);
    for(int i=0; i<100; i++) {
        app._UpdateHistoryMenu_Original();
    }
    gettimeofday(&end, NULL);
    double elapsedOriginal = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Original Total Time: %.4f s\n", elapsedOriginal);
    printf("Original Avg Time:   %.4f s\n", elapsedOriginal / 100.0);

    // Benchmark Optimized (Should skip rebuilds)

    // First call will rebuild (updating generation)
    app._UpdateHistoryMenu_Optimized();

    printf("Benchmarking Optimized (100 iterations, no history changes)...\n");
    gettimeofday(&start, NULL);
    for(int i=0; i<100; i++) {
        app._UpdateHistoryMenu_Optimized();
    }
    gettimeofday(&end, NULL);
    double elapsedOptimized = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Optimized Total Time: %.4f s\n", elapsedOptimized);
    printf("Optimized Avg Time:   %.4f s\n", elapsedOptimized / 100.0);

    // Verify robustness: Add item and ensure it rebuilds
    printf("Adding new item to history...\n");
    history->AddItem(BrowsingHistoryItem("http://new.com", BDateTime(1000001)));

    gettimeofday(&start, NULL);
    app._UpdateHistoryMenu_Optimized(); // Should rebuild
    gettimeofday(&end, NULL);
    double elapsedOneRebuild = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Time for single rebuild after change: %.4f s\n", elapsedOneRebuild);

    return 0;
}
