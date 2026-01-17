#ifndef MESSAGE_RUNNER_H
#define MESSAGE_RUNNER_H

#include <Messenger.h>
#include <Message.h>
#include <OS.h>

class BMessageRunner {
public:
	BMessageRunner(BMessenger target, const BMessage* message, bigtime_t interval, int32 count = -1) {}
};

#endif
