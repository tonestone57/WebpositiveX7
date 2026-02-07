/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <vector>
#include <string>
#include <iostream>

#include "ConsoleListHelper.h"
#include <String.h>

class MockConsoleList : public IConsoleList {
public:
	std::vector<std::string> items;

	virtual int32 CountItems() const {
		return (int32)items.size();
	}

	virtual void RemoveItem(int32 index) {
		if (index >= 0 && index < (int32)items.size()) {
			items.erase(items.begin() + index);
		}
	}

	virtual void RemoveItems(int32 index, int32 count) {
		if (index >= 0 && count > 0 && index + count <= (int32)items.size()) {
			items.erase(items.begin() + index, items.begin() + index + count);
		}
	}

	virtual void AddItem(const char* text) {
		items.push_back(text ? text : "");
	}

	virtual void SetItemText(int32 index, const char* text) {
		if (index >= 0 && index < (int32)items.size()) {
			items[index] = text ? text : "";
		}
	}

	virtual const char* GetItemText(int32 index) const {
		if (index >= 0 && index < (int32)items.size()) {
			return items[index].c_str();
		}
		return NULL;
	}
};

BString FormatMessage(const ConsoleMessage& msg) {
	return msg.text;
}

BString FormatRepeat(int32 count) {
	BString s;
	s << "Repeated " << count << " time" << (count == 1 ? "" : "s");
	return s;
}

int main() {
	MockConsoleList list;
	std::deque<ConsoleMessage> messages;
	BString previousText;
	int32 repeatCounter;

	// Test 1: Populate empty list
	messages.push_back({"A"});
	messages.push_back({"B"});

	UpdateConsoleMessageList(list, messages, false, previousText, repeatCounter, FormatMessage, FormatRepeat);

	if (list.CountItems() != 2) return 1;
	if (list.items[0] != "A") return 2;
	if (list.items[1] != "B") return 3;

	// Test 2: Repeat
	messages.clear();
	messages.push_back({"A"});
	messages.push_back({"A"});
	messages.push_back({"A"});

	UpdateConsoleMessageList(list, messages, false, previousText, repeatCounter, FormatMessage, FormatRepeat);

	if (list.CountItems() != 2) return 4;
	if (list.items[0] != "A") return 5;
	if (list.items[1] != "Repeated 2 times") return 6;

	// Test 3: Shift (pop front)
	// Current list: [A, "Repeated 2 times"]
	// New messages: [A, A] (one popped)
	messages.pop_front();

	UpdateConsoleMessageList(list, messages, false, previousText, repeatCounter, FormatMessage, FormatRepeat);

	if (list.CountItems() != 2) return 7;
	if (list.items[0] != "A") return 8;
	if (list.items[1] != "Repeated 1 time") return 9;

	// Test 4: Full Shift
	// Current: [A, "Repeated 1 time"]
	// New: [B]
	messages.clear();
	messages.push_back({"B"});

	UpdateConsoleMessageList(list, messages, false, previousText, repeatCounter, FormatMessage, FormatRepeat);

	if (list.CountItems() != 1) return 10;
	if (list.items[0] != "B") return 11;

	// Test 5: Errors Only
	messages.clear();
	messages.push_back({"Info", "", 0, 0, false});
	messages.push_back({"Error", "", 0, 0, true});

	UpdateConsoleMessageList(list, messages, true, previousText, repeatCounter, FormatMessage, FormatRepeat);

	if (list.CountItems() != 1) return 12;
	if (list.items[0] != "Error") return 13;

	// Test 6: Bulk Removal (Shift)
	list.items.clear();
	list.AddItem("Old1");
	list.AddItem("Old2");
	list.AddItem("Match");

	messages.clear();
	messages.push_back({"Match"});
	messages.push_back({"New"});

	UpdateConsoleMessageList(list, messages, false, previousText, repeatCounter, FormatMessage, FormatRepeat);

	if (list.CountItems() != 2) return 14;
	if (list.items[0] != "Match") return 15;
	if (list.items[1] != "New") return 16;

	std::cout << "All tests passed!" << std::endl;
	return 0;
}
