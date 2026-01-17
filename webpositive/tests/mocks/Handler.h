#ifndef HANDLER_H
#define HANDLER_H

#include <SupportDefs.h>

class BMessage;
class BLooper {
public:
    BLooper() {}
};

class BHandler {
public:
	BHandler(const char* name = NULL) {}
	virtual ~BHandler() {}
	virtual void MessageReceived(BMessage* message) {}

    BLooper* Looper() const { return NULL; }
};

#endif
