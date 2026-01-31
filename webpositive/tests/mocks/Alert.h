#ifndef _MOCK_ALERT_H
#define _MOCK_ALERT_H
#include "SupportDefs.h"

enum button_width { B_WIDTH_AS_USUAL };
enum alert_type { B_STOP_ALERT };

class BInvoker;

class BAlert {
public:
    BAlert(const char* title, const char* text, const char* button1, const char* button2 = NULL, const char* button3 = NULL, button_width width = B_WIDTH_AS_USUAL, alert_type type = B_STOP_ALERT) {}
    virtual ~BAlert() {}
    void SetFlags(uint32 flags) {}
    int32 Go() { return 0; }
    status_t Go(BInvoker* invoker) { return B_OK; }
    void SetShortcut(int32 index, char shortcut) {}
    uint32 Flags() { return 0; }
};
enum { B_CLOSE_ON_ESCAPE = 1 };
#endif
