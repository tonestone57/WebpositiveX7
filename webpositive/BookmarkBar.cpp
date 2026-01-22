/*
 * Copyright 2014, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * Distributed under the terms of the MIT License.
 */


#include "BookmarkBar.h"

#include <Alert.h>
#include <OS.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <GroupLayout.h>
#include <IconMenuItem.h>
#include <MessageQueue.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <PromptWindow.h>
#include <TextControl.h>
#include <Window.h>

#include "tracker_private.h"

#include "BrowserWindow.h"
#include "NavMenu.h"

#include <stdio.h>
#include <vector>


#define B_TRANSLATION_CONTEXT "BookmarkBar"

const uint32 kOpenNewTabMsg = 'opnt';
const uint32 kDeleteMsg = 'dele';
const uint32 kAskBookmarkNameMsg = 'askn';
const uint32 kShowInTrackerMsg = 'otrk';
const uint32 kRenameBookmarkMsg = 'rena';
const uint32 kFolderMsg = 'fold';

const uint32 kMsgInitialBookmarksLoaded = 'ibld';

struct LoaderParams {
	node_ref dirRef;
	BMessenger target;
};

struct BookmarkItem {
	ino_t inode;
	BMenuItem* item;
};


static BMenuItem*
CreateBookmarkItem(BEntry* entry, BHandler* handler)
{
	char name[B_FILE_NAME_LENGTH];
	entry->GetName(name);

	entry_ref ref;
	entry->GetRef(&ref);

	// In case it's a symlink, follow link to get the right icon,
	// but add the symlink's entry_ref for the IconMenuItem so it gets renamed/deleted/etc.
	BEntry followedLink(&ref, true); // traverse link

	if (followedLink.IsDirectory()) {
		BNavMenu* menu = new BNavMenu(name, B_REFS_RECEIVED, handler);
		menu->SetNavDir(&ref);
		BMessage* message = new BMessage(kFolderMsg);
		message->AddRef("refs", &ref);
		return new IconMenuItem(menu, message, "application/x-vnd.Be-directory", B_MINI_ICON);

	} else {
		BNode node(&followedLink);
		BNodeInfo info(&node);
		BMessage* message = new BMessage(B_REFS_RECEIVED);
		message->AddRef("refs", &ref);
		return new IconMenuItem(name, message, &info, B_MINI_ICON);
	}
}


static status_t
LoadBookmarksThread(void* data)
{
	LoaderParams* params = (LoaderParams*)data;
	BDirectory dir(&params->dirRef);
	BEntry bookmark;

	std::vector<BookmarkItem*>* items = new std::vector<BookmarkItem*>();
	items->reserve(20);

	while (dir.GetNextEntry(&bookmark, false) == B_OK) {
		node_ref ref;
		if (bookmark.GetNodeRef(&ref) == B_OK) {
			BMenuItem* item = CreateBookmarkItem(&bookmark, NULL);
			if (item) {
				BookmarkItem* bookmarkItem = new BookmarkItem;
				bookmarkItem->inode = ref.node;
				bookmarkItem->item = item;
				items->push_back(bookmarkItem);

				if (items->size() >= 20) {
					BMessage message(kMsgInitialBookmarksLoaded);
					message.AddPointer("list", items);
					if (params->target.SendMessage(&message) != B_OK) {
						for (size_t i = 0; i < items->size(); i++) {
							delete (*items)[i]->item;
							delete (*items)[i];
						}
						delete items;
					}
					items = new std::vector<BookmarkItem*>();
					items->reserve(20);
				}
			}
		}
	}

	if (!items->empty()) {
		BMessage message(kMsgInitialBookmarksLoaded);
		message.AddPointer("list", items);
		if (params->target.SendMessage(&message) != B_OK) {
			for (size_t i = 0; i < items->size(); i++) {
				delete (*items)[i]->item;
				delete (*items)[i];
			}
			delete items;
		}
	} else {
		delete items;
	}

	delete params;
	return B_OK;
}


BookmarkBar::BookmarkBar(const char* title, BHandler* target,
	const entry_ref* navDir)
	:
	BMenuBar(title)
{
	SetFlags(Flags() | B_FRAME_EVENTS);
	BEntry(navDir).GetNodeRef(&fNodeRef);

	fOverflowMenu = new BMenu(B_UTF8_ELLIPSIS);
	fOverflowMenuOwner.reset(fOverflowMenu);

	fPopUpMenu.reset(new BPopUpMenu("Bookmark Popup", false, false));
	fPopUpMenu->AddItem(
		new BMenuItem(B_TRANSLATE("Open in new tab"), new BMessage(kOpenNewTabMsg)));
	fPopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("Rename"), new BMessage(kAskBookmarkNameMsg)));
	fPopUpMenu->AddItem(
		new BMenuItem(B_TRANSLATE("Show in Tracker"), new BMessage(kShowInTrackerMsg)));
	fPopUpMenu->AddItem(new BSeparatorItem());
	fPopUpMenu->AddItem(new BMenuItem(B_TRANSLATE("Delete"), new BMessage(kDeleteMsg)));
}


BookmarkBar::~BookmarkBar()
{
	stop_watching(BMessenger(this));
}


void
BookmarkBar::MouseDown(BPoint where)
{
	fSelectedItemIndex = -1;
	BMessage* message = Window()->CurrentMessage();
	if (message != nullptr) {
		int32 buttons = 0;
		if (message->FindInt32("buttons", &buttons) == B_OK) {
			if (buttons & B_SECONDARY_MOUSE_BUTTON) {

				bool foundItem = false;
				for (int32 i = 0; i < CountItems(); i++) {
					BRect itemBounds = ItemAt(i)->Frame();
					if (itemBounds.Contains(where)) {
						foundItem = true;
						fSelectedItemIndex = i;
						break;
					}
				}
				if (foundItem) {
					BPoint screenWhere(where);
					ConvertToScreen(&screenWhere);
					if (ItemAt(fSelectedItemIndex)->Message()->what == kFolderMsg) {
						// This is a directory item, disable "open in new tab"
						fPopUpMenu->ItemAt(0)->SetEnabled(false);
					} else
						fPopUpMenu->ItemAt(0)->SetEnabled(true);

					// Pop up the menu
					fPopUpMenu->SetTargetForItems(this);
					fPopUpMenu->Go(screenWhere, true, true, true);
					return;
				}
			}
		}
	}

	BMenuBar::MouseDown(where);
}


void
BookmarkBar::AttachedToWindow()
{
	BMenuBar::AttachedToWindow();
	watch_node(&fNodeRef, B_WATCH_DIRECTORY, BMessenger(this));

	// Enumerate initial directory content
	LoaderParams* params = new LoaderParams;
	params->dirRef = fNodeRef;
	params->target = BMessenger(this);

	thread_id loader = spawn_thread(LoadBookmarksThread, "BookmarkLoader",
		B_NORMAL_PRIORITY, params);
	if (loader >= 0) {
		if (resume_thread(loader) != B_OK) {
			kill_thread(loader);
			delete params;
		}
	} else {
		delete params;
	}
}


void
BookmarkBar::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgInitialBookmarksLoaded:
		{
			std::vector<BookmarkItem*>* list;
			if (message->FindPointer("list", (void**)&list) == B_OK) {
				bool addedAny = false;
				for (size_t i = 0; i < list->size(); i++) {
					BookmarkItem* data = (*list)[i];
					if (fItemsMap.find(data->inode) == fItemsMap.end()) {
						IconMenuItem* item = (IconMenuItem*)data->item;

						// Fix target if it was NULL (from thread)
						if (BMenu* submenu = item->Submenu())
							submenu->SetTargetForItems(Window());

						bool addedToOverflow = false;
						// Optimize batch loading by adding directly to the overflow menu if we
						// know the bar is full.
						if (CountItems() > 0) {
							BMenuItem* last = ItemAt(CountItems() - 1);
							float maxRight = Bounds().Width();
							if (IndexOf(fOverflowMenu) != B_ERROR || fOverflowMenu->CountItems() > 0)
								maxRight -= 32;

							if (last->Frame().right > maxRight) {
								if (fOverflowMenu->AddItem(item))
									addedToOverflow = true;
								else {
									delete item;
									continue;
								}
							}
						}

						if (!addedToOverflow) {
							int32 count = CountItems();
							if (IndexOf(fOverflowMenu) != B_ERROR)
								count--;

							if (!BMenuBar::AddItem(item, count)) {
								delete item;
								continue;
							}
						}

						fItemsMap[data->inode] = item;
						addedAny = true;
					} else {
						delete data->item;
					}
					delete data;
				}
				delete list;

				if (addedAny) {
					BRect rect = Bounds();
					FrameResized(rect.Width(), rect.Height());
				}
			}
			break;
		}

		case B_NODE_MONITOR:
		{
			int32 opcode = message->FindInt32("opcode");
			ino_t inode = message->FindInt64("node");
			switch (opcode) {
				case B_ENTRY_CREATED:
				{
					entry_ref ref;
					const char* name;

					message->FindInt32("device", &ref.device);
					message->FindInt64("directory", &ref.directory);
					message->FindString("name", &name);
					ref.set_name(name);

					BEntry entry(&ref);
					if (entry.InitCheck() == B_OK) {
						// Only relayout if this is the last message
						_AddItem(inode, &entry, false);
						BMessageQueue* queue = Window()->MessageQueue();
						queue->Lock();
						BMessage* next = queue->FindMessage(0);
						bool more = (next != NULL && next->what == B_NODE_MONITOR);
						queue->Unlock();

						if (!more) {
							BRect rect = Bounds();
							FrameResized(rect.Width(), rect.Height());
						}
					}
					break;
				}
				case B_ENTRY_MOVED:
				{
					entry_ref ref;
					const char* name;

					message->FindInt32("device", &ref.device);
					message->FindInt64("to directory", &ref.directory);
					message->FindString("name", &name);
					ref.set_name(name);

					BEntry entry(&ref);
					BEntry followedEntry(&ref, true); // traverse in case it's a symlink

					if (fItemsMap[inode] == NULL) {
						_AddItem(inode, &entry, false);

						BMessageQueue* queue = Window()->MessageQueue();
						queue->Lock();
						BMessage* next = queue->FindMessage(0);
						bool more = (next != NULL && next->what == B_NODE_MONITOR);
						queue->Unlock();

						if (!more) {
							BRect rect = Bounds();
							FrameResized(rect.Width(), rect.Height());
						}
						break;
					} else {
						// Existing item. Check if it's a rename or a move
						// to some other folder.
						ino_t from, to;
						message->FindInt64("to directory", &to);
						message->FindInt64("from directory", &from);
						if (from == to) {
							const char* name;
							if (message->FindString("name", &name) == B_OK)
								fItemsMap[inode]->SetLabel(name);

							BMessage* itemMessage = new BMessage(
								followedEntry.IsDirectory() ? kFolderMsg : B_REFS_RECEIVED);
							itemMessage->AddRef("refs", &ref);
							fItemsMap[inode]->SetMessage(itemMessage);

							if (followedEntry.IsDirectory()) {
								BMenu* submenu = fItemsMap[inode]->Submenu();
								if (submenu) {
									BNavMenu* navMenu = dynamic_cast<BNavMenu*>(submenu);
									if (navMenu)
										navMenu->SetNavDir(&ref);
								}
							}

							break;
						}
					}

					// fall through: the item was moved from here to
					// elsewhere, remove it from the bar.
				}
				case B_ENTRY_REMOVED:
				{
					IconMenuItem* item = fItemsMap[inode];
					RemoveItem(item);
					fOverflowMenu->RemoveItem(item);
					fItemsMap.erase(inode);
					delete item;

					// Reevaluate whether the "more" menu is still needed
					BMessageQueue* queue = Window()->MessageQueue();
					queue->Lock();
					BMessage* next = queue->FindMessage(0);
					bool more = (next != NULL && next->what == B_NODE_MONITOR);
					queue->Unlock();

					if (!more) {
						BRect rect = Bounds();
						FrameResized(rect.Width(), rect.Height());
					}
					break;
				}
			}
			break;
		}

		case kOpenNewTabMsg:
		{
			if (fSelectedItemIndex >= 0 && fSelectedItemIndex < CountItems()) {
				// Get the bookmark refs
				entry_ref ref;
				BMenuItem* selectedItem = ItemAt(fSelectedItemIndex);
				if (selectedItem->Message()->what == kFolderMsg
						|| selectedItem->Message()->FindRef("refs", &ref) != B_OK) {
					break;
				}

				// Use the entry_ref to create a BEntry instance and get its path
				BEntry entry(&ref);
				BPath path;
				entry.GetPath(&path);

				BMessage* message = new BMessage(B_REFS_RECEIVED);
				message->AddRef("refs", &ref);
				Window()->PostMessage(message);
			}
			break;
		}
		case kDeleteMsg:
		{
			if (fSelectedItemIndex >= 0 && fSelectedItemIndex < CountItems()) {
				BMenuItem* selectedItem = ItemAt(fSelectedItemIndex);
				// Get the bookmark refs
				entry_ref ref;
				if (selectedItem->Message()->FindRef("refs", &ref) != B_OK)
					break;

				// Use the entry_ref to create a BEntry instance and get its path
				BEntry entry(&ref);
				BPath path;
				entry.GetPath(&path);

				// Remove the bookmark file
				if (entry.Remove() != B_OK) {
					// handle error case if necessary
					BString errorMessage = B_TRANSLATE("Failed to delete bookmark:\n'%path%'");
					errorMessage.ReplaceFirst("%path%", path.Path());
					BAlert* alert = new BAlert("Error", errorMessage.String(), B_TRANSLATE("OK"));
					alert->Go();
					break;
				}

				// Remove the item from the bookmark bar
				if (!RemoveItem(fSelectedItemIndex)) {
					// handle error case if necessary
					BString errorMessage = B_TRANSLATE("Failed to remove bookmark '%leaf%' "
							"from boookmark bar.");
					errorMessage.ReplaceFirst("%leaf%", path.Leaf());
					BAlert* alert = new BAlert("Error", errorMessage.String(), B_TRANSLATE("OK"));
					alert->Go();
				}
			}
			break;
		}
		case kShowInTrackerMsg:
		{
			entry_ref ref;
			if (fSelectedItemIndex >= 0 && fSelectedItemIndex < CountItems()) {
				BMenuItem* selectedItem = ItemAt(fSelectedItemIndex);
				// Get the bookmark refs
				if (selectedItem->Message()->FindRef("refs", &ref) != B_OK)
					break;

				BEntry entry(&ref);
				if (!entry.Exists())
					break;

				node_ref node;
				entry.GetNodeRef(&node);

				BEntry parent;
				entry.GetParent(&parent);
				entry_ref parentRef;
				parent.GetRef(&parentRef);

				// Ask Tracker to open the containing folder and select the
				// file inside it.
				BMessenger trackerMessenger("application/x-vnd.Be-TRAK");
				if (trackerMessenger.IsValid()) {
					BMessage message(B_REFS_RECEIVED);
					message.AddRef("refs", &parentRef);
					message.AddData("nodeRefToSelect", B_RAW_TYPE, &node, sizeof(node_ref));
					trackerMessenger.SendMessage(&message);
				}
			}
			break;
		}
		case kAskBookmarkNameMsg:
		{
			// Get the index of the selected item
			int32 index = fSelectedItemIndex;

			// Get the selected item
			if (index >= 0 && index < CountItems()) {
				BMenuItem* selectedItem = ItemAt(index);
				BString oldName = selectedItem->Label();
				BMessage* message = new BMessage(kRenameBookmarkMsg);
				message->AddPointer("item", selectedItem);
				BString request;
				request.SetToFormat(B_TRANSLATE("Old name: %s"), oldName.String());
				// Create a text control to get the new name from the user
				PromptWindow* prompt = new PromptWindow(B_TRANSLATE("Rename bookmark"),
					B_TRANSLATE("New name:"), request, this, message);
				prompt->Show();
				prompt->CenterOnScreen();
			}
			break;
		}
		case kRenameBookmarkMsg:
		{
			// User clicked OK, get the new name
			BString newName = message->FindString("text");
			BMenuItem* selectedItem = NULL;
			message->FindPointer("item", (void**)&selectedItem);

			// Rename the bookmark file
			entry_ref ref;
			if (selectedItem->Message()->FindRef("refs", &ref) == B_OK) {
				BEntry entry(&ref);
				entry.Rename(newName.String());

				// Update the menu item label
				selectedItem->SetLabel(newName);
			}
			break;
		}

		default:
			BMenuBar::MessageReceived(message);
			break;
	}
}


void
BookmarkBar::FrameResized(float width, float height)
{
	int32 count = CountItems();

	// Collect all items (bar + overflow) into a single logical list for sizing
	// Note: We don't use a vector of BMenuItem* to avoid allocations if possible,
	// but direct access logic is complex due to two sources.
	// Since this is UI code on resize, a vector of pointers is cheap enough.
	std::vector<BMenuItem*> allItems;

	bool overflowMenuAttached = (fOverflowMenuOwner.get() == NULL);
	if (overflowMenuAttached)
		count--;

	allItems.reserve(count + fOverflowMenu->CountItems());

	for (int32 k = 0; k < count; k++)
		allItems.push_back(ItemAt(k));
	for (int32 k = 0; k < fOverflowMenu->CountItems(); k++)
		allItems.push_back(fOverflowMenu->ItemAt(k));

	// Calculate how many items fit
	int32 fitCount = 0;
	int32 overflowMenuWidth = 0;
	float currentX = 0;
	bool needsOverflow = false;

	float left, top, right, bottom;
	be_control_look->GetMenuItemInsets(&left, &top, &right, &bottom);
	float itemPadding = left + right;

	// First pass: try to fit everything without overflow menu
	for (size_t k = 0; k < allItems.size(); k++) {
		float itemWidth = 0;
		if (allItems[k]->Menu() == this)
			itemWidth = allItems[k]->Frame().Width();
		else {
			float w, h;
			allItems[k]->GetContentSize(&w, &h);
			itemWidth = w + itemPadding;
		}

		// Assuming standard padding/margins are included in Frame() or negligible for logic check,
		// but actual placement by BMenuBar might differ slightly.
		// We use the accumulated width logic similar to original code's reliance on 'rightmost'.
		// However, without 'rightmost' from actual layout, we sum widths.
		// Original code: rightmost = Frame().right. This includes previous items + this item.
		// So summing widths is the correct approximation if margins are consistent.

		if (currentX + itemWidth > width) {
			needsOverflow = true;
			break;
		}
		currentX += itemWidth;
		fitCount++;
	}

	if (needsOverflow) {
		// Second pass: fit with overflow menu space reserved
		overflowMenuWidth = 32;
		currentX = 0;
		fitCount = 0;
		for (size_t k = 0; k < allItems.size(); k++) {
			float itemWidth = 0;
			if (allItems[k]->Menu() == this)
				itemWidth = allItems[k]->Frame().Width();
			else {
				float w, h;
				allItems[k]->GetContentSize(&w, &h);
				itemWidth = w + itemPadding;
			}

			if (currentX + itemWidth > width - overflowMenuWidth) {
				break;
			}
			currentX += itemWidth;
			fitCount++;
		}
	} else {
		fitCount = (int32)allItems.size();
	}

	// Apply changes efficiently to minimize relayouts
	if (overflowMenuAttached) {
		RemoveItem(fOverflowMenu);
		fOverflowMenuOwner.reset(fOverflowMenu);
	}

	// Move excess items from Bar to Overflow
	// Current items in bar (excluding overflow menu which is now gone)
	int32 currentBarCount = CountItems();

	// Optimized move from Bar to Overflow
	// We first collect the items to be moved (reversed order), then
	// reconstruct the overflow menu to avoid O(N^2) insertions at index 0.
	std::vector<BMenuItem*> newOverflowItems;
	if (currentBarCount > fitCount) {
		newOverflowItems.reserve(currentBarCount - fitCount);

		while (currentBarCount > fitCount) {
			newOverflowItems.push_back(RemoveItem(currentBarCount - 1));
			currentBarCount--;
		}
	}

	if (!newOverflowItems.empty()) {
		std::vector<BMenuItem*> oldOverflowItems;
		int32 overflowCount = fOverflowMenu->CountItems();
		oldOverflowItems.reserve(overflowCount);

		// Remove existing items from the end (efficient)
		while (overflowCount > 0) {
			oldOverflowItems.push_back(fOverflowMenu->RemoveItem(--overflowCount));
		}

		// Add new items (they were removed from end of bar, so reverse iterate to restore order)
		for (int32 i = (int32)newOverflowItems.size() - 1; i >= 0; i--) {
			fOverflowMenu->AddItem(newOverflowItems[i]);
		}

		// Add old items back (they were removed from end of overflow, so reverse iterate)
		for (int32 i = (int32)oldOverflowItems.size() - 1; i >= 0; i--) {
			fOverflowMenu->AddItem(oldOverflowItems[i]);
		}
	}

	// Move fitting items from Overflow to Bar
	while (currentBarCount < fitCount && fOverflowMenu->CountItems() > 0) {
		BMenuItem* item = fOverflowMenu->RemoveItem((int32)0);
		AddItem(item); // Add to end
		currentBarCount++;
	}

	// Re-add overflow menu if needed
	if (needsOverflow || fOverflowMenu->CountItems() > 0) {
		if (fOverflowMenu->CountItems() > 0) {
			AddItem(fOverflowMenu);
			fOverflowMenuOwner.release(); // Relinquish ownership to BMenuBar
		}
	}

	BMenuBar::FrameResized(width, height);
}


BSize
BookmarkBar::MinSize()
{
	BSize size = BMenuBar::MinSize();

	// We only need space to show the "more" button.
	size.width = 32;

	// We need enough vertical space to show bookmark icons.
	if (size.height < 20)
		size.height = 20;

	return size;
}


// #pragma mark - private methods


void
BookmarkBar::_AddItem(ino_t inode, BEntry* entry, bool layout)
{
	if (fItemsMap[inode] != NULL)
		return;

	BMenuItem* item = CreateBookmarkItem(entry, Window());
	if (item == NULL)
		return;

	bool addedToOverflow = false;
	// Optimize batch loading by adding directly to the overflow menu if we
	// know the bar is full. This avoids the O(n^2) behavior of FrameResized
	// moving items one by one to the overflow menu.
	if (!layout && CountItems() > 0) {
		BMenuItem* last = ItemAt(CountItems() - 1);
		float maxRight = Bounds().Width();
		if (IndexOf(fOverflowMenu) != B_ERROR || fOverflowMenu->CountItems() > 0)
			maxRight -= 32;

		if (last->Frame().right > maxRight) {
			if (fOverflowMenu->AddItem(item))
				addedToOverflow = true;
			else {
				delete item;
				return;
			}
		}
	}

	if (!addedToOverflow) {
		int32 count = CountItems();
		if (IndexOf(fOverflowMenu) != B_ERROR)
			count--;

		if (!BMenuBar::AddItem(item, count)) {
			delete item;
			return;
		}
	}

	fItemsMap[inode] = (IconMenuItem*)item;

	// Move the item to the "more" menu if it overflows.
	if (layout) {
		BRect rect = Bounds();
		FrameResized(rect.Width(), rect.Height());
	}
}
