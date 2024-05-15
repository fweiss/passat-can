/**
 * Inject the define from build to configure the type of CAN interface.
 * Note that TWAI is the name ESP32 uses for CAN 2.0B
 */
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

    // the CAN class is configured at build time
    CAN_CLASS app;

    app.init();

    // should be run() since it doesn't normally return
    app.start();
}
