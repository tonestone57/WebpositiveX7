#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include <map>

// Mock Headers
#include "String.h"
#include "File.h"
#include "Message.h"
#include "FindDirectory.h"
#include "OS.h"
#include "Roster.h"
#include "Alert.h"
#include "Catalog.h"

// Define BFile::content
std::string BFile::content = "";

// Stub for find_directory
status_t find_directory(directory_which which, BPath* path) {
    path->SetTo("/tmp"); // Just a dummy path
    return B_OK;
}

// Stub for spawn_thread/resume_thread (execute immediately)
thread_id spawn_thread(status_t (*func)(void*), const char* name, int32 priority, void* data) {
    func(data);
    return 1;
}

status_t resume_thread(thread_id thread) {
    return B_OK;
}

status_t kill_thread(thread_id thread) {
    return B_OK;
}

bigtime_t system_time() {
    return 123456789;
}

// Define be_roster
BRoster* be_roster = new BRoster();

// Access private members
#define private public
#include "../support/PageSourceSaver.cpp"

int main() {
    printf("Running PageSourceSaverTest...\n");

    // Test Case: Message with "source" as raw data (simulating large buffer or non-string)
    // AddData uses B_RAW_TYPE (or any type). FindString should fail. FindData (impl in fix) should succeed.

    BMessage msg;
    const char* rawContent = "<html><body>Raw Data</body></html>";
    msg.AddData("source", B_RAW_TYPE, rawContent, strlen(rawContent));

    // Clear file content
    BFile::content = "";

    // Case 1: Raw Data
    PageSourceSaver::HandlePageSourceResult(&msg);

    printf("Case 1 (Raw Data) content: '%s'\n", BFile::content.c_str());
    std::string expected(rawContent);
    if (BFile::content == expected) {
        printf("SUCCESS: Raw content written correctly.\n");
    } else {
        printf("FAILURE: Raw content mismatch.\n");
    }

    // Case 2: String Data (Regression Check)
    BMessage* stringMsg = new BMessage();
    stringMsg->AddString("source", "StringContent");

    BFile::content = "";
    // Call _PageSourceThread directly because HandlePageSourceResult would consume it.
    // _PageSourceThread takes ownership and deletes the message.
    PageSourceSaver::_PageSourceThread(stringMsg);

    printf("Case 2 (String) content length: %lu\n", BFile::content.length());
    if (BFile::content == "StringContent") {
        printf("SUCCESS: String content written without null terminator.\n");
    } else if (BFile::content.length() == 14 && BFile::content[13] == '\0') {
        printf("FAILURE: String content written WITH null terminator.\n");
    } else {
         printf("FAILURE: String content mismatch.\n");
    }

    delete be_roster;
    return 0;
}
