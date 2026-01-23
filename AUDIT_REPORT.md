# Code Audit Summary

## Completed Fixes
1.  **Blocking Alerts in `BrowserWindow.cpp`**:
    -   Converted synchronous `BAlert::Go()` calls to asynchronous `BInvoker` messaging for `CLEAR_HISTORY`, `B_REFS_RECEIVED` (Bookmarks), `SHOW_CERTIFICATE_WINDOW`, and `LoadFailed` (HTTPS fallback).
    -   Prevents main thread deadlocks during shutdown if an alert is open.

2.  **Proxy Password Persistence in `SettingsWindow.cpp`**:
    -   Modified `_RevertSettings` to fetch the proxy password from `BKeyStore` if the settings file has an empty password but authentication is enabled.
    -   Prevents the password from being cleared from the KeyStore when the user opens and saves settings.

## Audited Files (No Critical Issues Found)
The following files were reviewed for memory leaks, race conditions, and logic errors. No further critical issues were identified.

*   **`DownloadWindow.cpp`**:
    -   Thread management for saving settings checks return values correctly (`resume_thread`).
    -   View memory management in `fDownloadViewsLayout` is handled via standard BLayoutItem ownership.
    -   Map iterators in `_DownloadFinished` are handled safely.

*   **`BrowsingHistory.cpp`**:
    -   Uses `std::unique_ptr` for raw buffer management during import.
    -   Uses `atomic_add` for tracking active save threads to ensure clean shutdown.
    -   Thread-safe locking mechanisms are in place.

*   **`TabManager.cpp`**:
    -   `MoveTab` implements robust recovery logic if memory allocation fails during tab reordering, preventing inconsistent state.

*   **`NetworkWindow.cpp`**:
    -   List view item management correctly handles deletion.
    -   Concurrency between `ADD` and `UPDATE` messages is serialized by the window message loop.

*   **`ConsoleWindow.cpp`**:
    -   Message coalescing logic handles repeated lines correctly.
    -   List size limiting correctly manages memory.

*   **`AuthenticationPanel.cpp`**:
    -   Implements a correct modal loop using semaphores and `wait_for_objects_etc` to keep the parent window responsive.
    -   Securely wipes password memory buffers.

*   **`BookmarkBar.cpp`**:
    -   Uses `std::unique_ptr` for overflow menu ownership management.
    -   Threaded loading logic properly hands off ownership of allocated items to the main thread via `BMessage`.
    -   Robustly handles file system changes (`B_ENTRY_MOVED`, etc.).

*   **`CredentialsStorage.cpp`**:
    -   Securely wipes password buffers in `Credentials` destructor.
    -   Uses `SetTo` with string pointer to force deep copy and avoid COW issues with sensitive data.
    -   Correctly uses `BKeyStore` for persistent storage.

*   **`PermissionsWindow.cpp`**:
    -   Correctly cleans up list items in `_ClearPermissions`.
    -   Uses `SitePermissionsManager` singleton for data persistence.

*   **`SitePermissionsManager.cpp`**:
    -   Thread-safe access via `BAutolock`.
    -   Correctly handles case-insensitive domain matching.

*   **`URLInputGroup.cpp`**:
    -   Drag and drop bitmap memory is managed correctly (deleted after `DragMessage`).
    -   Text insertion logic safely handles formatting.

*   **`BrowserApp.cpp`**:
    -   Signal-safe crash handler implementation.
    -   Correctly handles `setenv` for cookies.
    -   Window management loop logic in `QuitRequested` is robust.

*   **`CookieWindow.cpp`**:
    -   RAII used for root domain node.
    -   Tree traversal logic in `_BuildDomainList` appears correct and safe.

*   **`DownloadProgressView.cpp`**:
    -   Correct usage of `watch_node` and `stop_watching`.
    -   Memory management of child views (IconView, buttons) is handled by the layout system.

*   **`Sync.cpp`**:
    -   `ImportCookies` uses `std::unique_ptr` for buffer safety and checks file size limits.
    -   Cookie file parsing is robust against malformed lines.

*   **`SourceWindow.cpp`**:
    -   Syntax highlighting logic uses safe bounds checking (`TextLength()`).
    -   Uses `BTextView` correctly for read-only display.

*   **`TabSearchWindow.cpp`**:
    -   Uses `LockTargetWithTimeout` when accessing `BrowserWindow` state, preventing potential deadlocks if the main window is busy or closing.
