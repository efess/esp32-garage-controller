#include "esp_wifi_types.h"

typedef void (*mqtt_subscription_callback)(const char *topic, const char *data, void *arg);

void mqtt_subscribe_callback(char *topic, mqtt_subscription_callback callback, void *arg); 
esp_err_t mqtt_init();
esp_err_t mqtt_publish(const char *topic, const char *data);