#!/bin/bash

# Define header search paths (assuming Haiku system headers are in standard path)
INCLUDES="-I. -I../"

# Compile URLHandlerTest (existing)
echo "Compiling URLHandlerTest..."
# Needs to link against URLHandler.cpp. In a real build system this would use object files.
g++ $INCLUDES URLHandlerTest.cpp ../support/URLHandler.cpp -o URLHandlerTest -lbe
if [ $? -eq 0 ]; then
    echo "Running URLHandlerTest..."
    ./URLHandlerTest
else
    echo "Failed to compile URLHandlerTest"
fi

echo "--------------------------------"

# Compile BrowsingHistoryItemTest (new)
echo "Compiling BrowsingHistoryItemTest..."
# BrowsingHistory depends on other files, might be tricky to link standalone without library.
# We include BrowsingHistory.cpp in the test file via #include in the previous version,
# but now we changed it to linking.
# Wait, I removed the #include "../BrowsingHistory.cpp" from BrowsingHistoryItemTest.cpp.
# So I must compile it.
# BrowsingHistory.cpp depends on SettingsKeys.cpp
g++ $INCLUDES BrowsingHistoryItemTest.cpp ../BrowsingHistory.cpp ../SettingsKeys.cpp -o BrowsingHistoryItemTest -lbe
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
