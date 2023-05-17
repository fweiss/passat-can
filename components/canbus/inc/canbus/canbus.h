#pragma once

#include <functional>
#include <string>

#include "driver/twai.h"

class CanBus {
public:
    CanBus();
    virtual ~CanBus() {};

    void init();

    void onRecvFrame(std::function<void(twai_message_t & message)>);
    void sendFrame(twai_message_t & message);
    std::string messageToString(twai_message_t & message);
    void triggerRead();
private:
    std::function<void(twai_message_t & message)> recvCallback;
};
