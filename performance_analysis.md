# Performance Analysis: BookmarkBar Initialization

## Issue
The `BookmarkBar` initialization in `AttachedToWindow` has quadratic complexity O(N^2) where N is the number of bookmarks.

### Current Implementation
```cpp
void BookmarkBar::AttachedToWindow() {
    // ...
    while (dir.GetNextEntry(&bookmark, false) == B_OK) {
        _AddItem(ref.node, &bookmark); // Calls FrameResized()
    }
}

void BookmarkBar::_AddItem(...) {
    // ... adds item ...
    FrameResized(rect.Width(), rect.Height()); // O(M) where M is current items
}
```

Each call to `FrameResized` iterates through the existing menu items to check for overflow and update the "More" menu.
- 1st item added: `FrameResized` scans 1 item.
- 2nd item added: `FrameResized` scans 2 items.
- ...
- Nth item added: `FrameResized` scans N items.

Total operations ~= Sum(1..N) = N*(N+1)/2 = O(N^2).

## Proposed Solution
Batch the additions and defer the layout calculation (`FrameResized`) until all items are added.

### Optimized Implementation
```cpp
void BookmarkBar::AttachedToWindow() {
    // ...
    while (dir.GetNextEntry(&bookmark, false) == B_OK) {
        _AddItem(ref.node, &bookmark, false); // Don't layout
    }
    FrameResized(Bounds().Width(), Bounds().Height()); // Call once
}
```

Complexity becomes O(N) for additions + O(N) for the single `FrameResized` call = O(N).

## Verification Plan (Mock)
If the environment permitted compilation and execution of Haiku applications, the following benchmark would be used:

1. **Setup**: Create a temporary directory with 1000 dummy bookmark files.
2. **Benchmark**:
   - Measure time for `BookmarkBar::AttachedToWindow` with original code.
   - Measure time for `BookmarkBar::AttachedToWindow` with optimized code.
3. **Expected Result**:
   - Original: Time increases quadratically with N.
   - Optimized: Time increases linearly with N.

Since we cannot run this, we rely on the algorithmic complexity analysis above.
