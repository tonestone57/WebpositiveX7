#ifndef _MOCK_INVOKER_H
#define _MOCK_INVOKER_H
#include "Message.h"
#include "Handler.h"
#include "Looper.h"

class BInvoker {
public:
    BInvoker(BMessage* message, BHandler* handler, BLooper* looper = NULL) {}
    virtual ~BInvoker() {}
};
#endif
