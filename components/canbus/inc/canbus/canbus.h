#pragma once

#include <functional>

#include "driver/can.h"

class CanBus {
public:
    CanBus();
    virtual ~CanBus() {};

    void init();

    void onRecvFrame(std::function<void(can_message_t & message)>);
    void sendFrame(can_message_t & message);
    std::string messageToString(can_message_t & message);
    void triggerRead();
private:
    std::function<void(can_message_t & message)> recvCallback;
};
