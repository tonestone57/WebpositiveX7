/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DOWNLOAD_WINDOW_H
#define DOWNLOAD_WINDOW_H


#include <map>

#include <String.h>
#include <Window.h>

#include <memory>

class BButton;
class BFile;
class BGroupLayout;
class BMessageRunner;
class BScrollView;
class BWebDownload;
class SettingsMessage;


class DownloadWindow : public BWindow {
public:
								DownloadWindow(BRect frame, bool visible,
									SettingsMessage* settings);
	virtual						~DownloadWindow();

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* target);
	virtual void				FrameResized(float newWidth, float newHeight);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				PrepareToQuit();

			bool				DownloadsInProgress();
			void				SetMinimizeOnClose(bool minimize);

private:
			void				_DownloadStarted(BWebDownload* download);
			void				_DownloadFinished(BWebDownload* download);
			void				_RemoveFinishedDownloads();
			void				_RemoveMissingDownloads();
			void				_ValidateButtonStatus();
			void				_SaveSettings();
			void				_ScheduleSaveSettings();
			void				_PerformSaveSettings(bool wait = false);
			void				_LoadSettings();
			bool				_OpenSettingsFile(BFile& file, uint32 mode);

private:
			std::map<BWebDownload*, DownloadProgressView*> fDownloadsMap;
			BScrollView*		fDownloadsScrollView;
			BGroupLayout*		fDownloadViewsLayout;
			BButton*			fRemoveFinishedButton;
			BButton*			fRemoveMissingButton;
			BButton*			fOpenFolderButton;
			BString				fDownloadPath;
			bool				fMinimizeOnClose;
			bool				fQuitting;
			bool				fSettingsDirty;
			std::unique_ptr<BMessageRunner>		fSaveSettingsRunner;
};

#endif // DOWNLOAD_WINDOW_H
