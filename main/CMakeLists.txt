idf_component_register(SRCS "garageState.c" "homeassistant.c" "mqtt.c" "main.c" "server.c" "filesystem.c" "wifi.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_netif driver esp_wifi vfs mqtt esp_http_server json fatfs nvs_flash dns_server)