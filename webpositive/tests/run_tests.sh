#!/bin/bash

# Define header search paths
INCLUDES="-I. -I./mocks -I../"

# Define mock sources that are implemented in headers or don't need compilation
# SyncTest needs to link against Sync.cpp but Sync.cpp includes BookmarkManager.h and BrowsingHistory.h
# We provide mocks for those in ./mocks so we don't link against the real ones.

# Compile URLHandlerTest (existing)
echo "Compiling URLHandlerTest..."
g++ $INCLUDES URLHandlerTest.cpp -o URLHandlerTest
if [ $? -eq 0 ]; then
    echo "Running URLHandlerTest..."
    ./URLHandlerTest
else
    echo "Failed to compile URLHandlerTest"
fi

echo "--------------------------------"

# Compile BrowsingHistoryItemTest (existing)
echo "Compiling BrowsingHistoryItemTest..."
# Added FileStub.cpp to link BFile::sFileSystem
g++ $INCLUDES BrowsingHistoryItemTest.cpp FileStub.cpp -o BrowsingHistoryItemTest
if [ $? -eq 0 ]; then
    echo "Running BrowsingHistoryItemTest..."
    ./BrowsingHistoryItemTest
else
    echo "Failed to compile BrowsingHistoryItemTest"
fi

echo "--------------------------------"

# Compile BookmarkLoadBenchmark (existing)
echo "Compiling BookmarkLoadBenchmark..."
g++ $INCLUDES BookmarkLoadBenchmark.cpp -o BookmarkLoadBenchmark
if [ $? -eq 0 ]; then
    echo "Running BookmarkLoadBenchmark..."
    ./BookmarkLoadBenchmark
else
    echo "Failed to compile BookmarkLoadBenchmark"
fi

echo "--------------------------------"

# Compile SyncTest (new)
echo "Compiling SyncTest..."
# We compile SyncTest.cpp and ../Sync.cpp
# We also link BrowsingHistoryStub.cpp to satisfy BrowsingHistory symbols
# And FileStub.cpp for BFile::sFileSystem
g++ $INCLUDES SyncTest.cpp ../Sync.cpp BrowsingHistoryStub.cpp FileStub.cpp -o SyncTest
if [ $? -eq 0 ]; then
    echo "Running SyncTest..."
    ./SyncTest
else
    echo "Failed to compile SyncTest"
fi

# Cleanup
rm -f URLHandlerTest BrowsingHistoryItemTest BookmarkLoadBenchmark SyncTest
