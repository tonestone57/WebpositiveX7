#ifndef _MESSENGER_H
#define _MESSENGER_H
#include "Handler.h"
class BMessenger {
public:
    BMessenger(BHandler* handler) {}
    status_t SendMessage(uint32 command) { return B_OK; }
};
#endif
