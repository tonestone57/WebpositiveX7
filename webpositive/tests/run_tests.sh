#!/bin/bash

# Define header search paths
INCLUDES="-I. -I./mocks -I../"

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

# Compile BrowsingHistoryItemTest (new)
echo "Compiling BrowsingHistoryItemTest..."
g++ $INCLUDES BrowsingHistoryItemTest.cpp -o BrowsingHistoryItemTest
if [ $? -eq 0 ]; then
    echo "Running BrowsingHistoryItemTest..."
    ./BrowsingHistoryItemTest
else
    echo "Failed to compile BrowsingHistoryItemTest"
fi

echo "--------------------------------"

# Compile BookmarkLoadBenchmark (new)
echo "Compiling BookmarkLoadBenchmark..."
g++ $INCLUDES BookmarkLoadBenchmark.cpp -o BookmarkLoadBenchmark
if [ $? -eq 0 ]; then
    echo "Running BookmarkLoadBenchmark..."
    ./BookmarkLoadBenchmark
else
    echo "Failed to compile BookmarkLoadBenchmark"
fi

# Cleanup
rm -f URLHandlerTest BrowsingHistoryItemTest BookmarkLoadBenchmark
