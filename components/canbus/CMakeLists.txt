idf_component_register(
	SRCS 
		canbus.cpp
		spi.cpp
		mcp25625.cpp
        keepalive.cpp

	INCLUDE_DIRS inc inc/canbus
    REQUIRES driver
)