#include "canbus.h"
#include "mcp25625.h"

#ifdef CAN_INTERFACE_ESP_TWAI
    #include "twai_app.h"
    #define CAN_CLASS TwaiApp
#endif
#ifdef CAN_INTERFACE_MCP25625
    #include "app_mcp25625.h"
    #define CAN_CLASS AppMcp25625
#endif

extern "C" {
	void app_main(void);
}

void app_main() {

    CAN_CLASS app;

    app.init();

    app.start();
}
