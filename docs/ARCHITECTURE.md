#
Currently a work in progress

## Services
- html page service
- web socket service
- canbus service

Core processes:
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

 ### translation data types
 - stuff that comes from one of the data services
 - use streams, light NodeJS, callbacks
 - but really thinking of one for each end of a translater
 - streams?
 - FreeRTOS tasks and queues

 soooo...
 template<Pair<AType,BType>>
 
 // create a queue for each type

 ## Websocket ping
 Use the WS ping control frame to check if the client is connected.
 - rfc6455: ping may be used as keepalive or verify remote is still responsive
 - ping should stop when the WS connection is opened
 - ping should stop when the WS connection is closed
 - ping period from 1-60 sec, depending how responsive it needs to be
 - race condition?
 - maybe just use the close frames to manage connection?
 - close frame not received if TCP connection lost

 When close is received, should echo it back.
