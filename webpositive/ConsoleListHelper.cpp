/*
 * Copyright 2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "ConsoleListHelper.h"

void UpdateConsoleMessageList(
	IConsoleList& list,
	const std::deque<ConsoleMessage>& messages,
	bool errorsOnly,
	BString& previousText,
	int32& repeatCounter,
	MessageFormatter formatMessage,
	RepeatFormatter formatRepeat)
{
	previousText = "";
	repeatCounter = 0;

	int32 listIndex = 0;
	bool firstMessageProcessed = false;

	for (const auto& msg : messages) {
		if (errorsOnly && !msg.isError)
			continue;

		BString finalText = formatMessage(msg);

		if (!firstMessageProcessed) {
			// First visible message. Check if we can sync with list head.
			// Look for match in first 200 items of current list.
			int32 limit = list.CountItems();
			if (limit > 200) limit = 200;

			int32 matchIndex = -1;
			for (int32 i = 0; i < limit; i++) {
				const char* itemText = list.GetItemText(i);
				if (itemText && finalText == itemText) {
					matchIndex = i;
					break;
				}
			}

			if (matchIndex > 0) {
				// Remove garbage at top
				list.RemoveItems(0, matchIndex);
			} else if (matchIndex == -1) {
				// No match found near top.
				// Assuming list is completely different/invalid.
				// We proceed to overwrite/add from index 0.
			}
			firstMessageProcessed = true;
		}

		if (finalText == previousText) {
			repeatCounter++;
			BString repeatText = formatRepeat(repeatCounter);

			if (repeatCounter > 1) {
				// Update the *previous* item in the list (which is the repeat message)
				// The item at listIndex - 1
				int32 updateIndex = listIndex - 1;
				if (updateIndex >= 0) {
					if (list.CountItems() > updateIndex) {
						const char* currentText = list.GetItemText(updateIndex);
						if (!currentText || BString(currentText) != repeatText) {
							list.SetItemText(updateIndex, repeatText.String());
						}
					} else {
						// Should not happen if logic is correct
						list.AddItem(repeatText.String());
					}
				}
			} else {
				// Add new repeat message
				// This corresponds to listIndex
				if (listIndex < list.CountItems()) {
					const char* currentText = list.GetItemText(listIndex);
					if (!currentText || BString(currentText) != repeatText) {
						list.SetItemText(listIndex, repeatText.String());
					}
				} else {
					list.AddItem(repeatText.String());
				}
				listIndex++;
			}
		} else {
			previousText = finalText;
			repeatCounter = 0;

			// Add/Update message
			if (listIndex < list.CountItems()) {
				const char* currentText = list.GetItemText(listIndex);
				if (!currentText || BString(currentText) != finalText) {
					list.SetItemText(listIndex, finalText.String());
				}
			} else {
				list.AddItem(finalText.String());
			}
			listIndex++;
		}
	}

	// If no messages were processed (e.g. empty or all filtered), listIndex is 0.
	// We should clear the list.
	// If messages were processed, remove excess items.

	int32 count = list.CountItems();
	if (count > listIndex) {
		list.RemoveItems(listIndex, count - listIndex);
	}
}
