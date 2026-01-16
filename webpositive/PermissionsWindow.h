/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PERMISSIONS_WINDOW_H
#define PERMISSIONS_WINDOW_H

#include <Window.h>

class BListView;
class BButton;
class BCheckBox;
class BTextControl;

class PermissionsWindow : public BWindow {
public:
								PermissionsWindow(BRect frame);
	virtual						~PermissionsWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			void				_LoadPermissions();
			void				_SavePermissions();
			void				_UpdateFields();

private:
			BListView*			fDomainList;
			BCheckBox*			fJSEnabled;
			BCheckBox*			fCookiesEnabled;
			BCheckBox*			fPopupsEnabled;
			BTextControl*		fAddDomainControl;
			BButton*			fAddButton;
			BButton*			fRemoveButton;
};

#endif // PERMISSIONS_WINDOW_H
