#ifndef _MOCK_HANDLER_H
#define _MOCK_HANDLER_H
#include "Message.h"
#include "SupportDefs.h" // NULL

class BLooper;

class BHandler {
public:
    BHandler(const char* name = NULL) {}
    virtual ~BHandler() {}
    virtual void MessageReceived(BMessage* message) {}
    BLooper* Looper() const { return NULL; }
};
#endif
