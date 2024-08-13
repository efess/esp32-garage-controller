#include <stdio.h>
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "./mqtt.h"
#include "./server.h"
#include "./filesystem.h"
#include "./wifi.h"
#include "garageState.h"

void wifi_status_callback(wifi_status status)
{
    if (status == WIFI_STATUS_CONNECTED_STATION)
    {
        printf("Connected to WiFi\n");
    }
    else if (status == WIFI_STATUS_DISCONNECTED)
    {
        printf("Disconnected from WiFi\n");
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    init_garage_state();
    wifi_start();
    ESP_ERROR_CHECK(mqtt_init());
    init_fs();
    start_rest_server(CONFIG_GARAGE_WEB_MOUNT_POINT);
}