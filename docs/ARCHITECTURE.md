#
 currently a work in progress

## Basic
services:
- page service
- web socket service
- canbus service

core process:
- relay incoming CAN to outgoing WS
- relay incoming WS to outgoing CAN
- try to keep incoming asynchronous
- try to abstract canbus server
- try to abstract websocket server

figure out
- translation service

# translation service
 request (transformat) translation between CAN auto frames and WS 
 
 scenario:
 - CAN message comes in as abstraction on canbus service
 - send it to transFormat<From, To>()
 - send the <To> to the WS service

 what about service errors/events?

 # translation data types
 - stuff that comes from one of the data services
 - use streams, light NodeJS, callbacks
 - but really thinking of one for each end of a translater
 - streams?
 - FreeRTOS tasks and queues

 soooo...
 template<Pair<AType,BType>>
 
 // create a queue for each type
