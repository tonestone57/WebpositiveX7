/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TAB_SEARCH_WINDOW_H
#define TAB_SEARCH_WINDOW_H

#include <Window.h>
#include <Messenger.h>
#include <String.h>

class BListView;
class BTextControl;
class TabManager;

class TabSearchWindow : public BWindow {
public:
								TabSearchWindow(TabManager* manager);
	virtual						~TabSearchWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				SetTabManager(TabManager* manager);
			void				PrepareToQuit();

private:
			void				_UpdateList();
			void				_FilterList();

private:
			TabManager*			fTabManager;
			BMessenger			fTarget;
			BTextControl*		fSearchControl;
			BListView*			fTabList;
			bool				fForceQuit;
};

#endif // TAB_SEARCH_WINDOW_H
