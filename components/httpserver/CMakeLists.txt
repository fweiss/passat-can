idf_component_register(
	SRCS httpserver.cpp
	INCLUDE_DIRS inc/httpserver
    REQUIRES esp_http_server spiffs
)