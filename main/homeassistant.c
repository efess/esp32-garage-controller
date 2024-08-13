#include <unistd.h>
#include "cJSON.h"
#include "esp_log.h"
#include "./garageState.h"
#include "./mqtt.h"
#include "./homeassistant.h"

#define TAG "homeassistant"

void homeassistant_garage_set(const char *topic, const char *data, void *arg) {
    int garage = *(int *)arg;
    ESP_LOGI(TAG, "Received set command for garage %d", garage);
    
    garage_trigger_door_signal(garage);
}

void homeassistant_garage_state_changed(int garage, garage_door_state_t state) {
    char buffer[50];
    sprintf(buffer, "homeassistant/cover/garage_%d/state", garage);
    const char *stateString;
    switch(state) {
        case GARAGE_STATE_CLOSED:
            stateString = "closed";
            break;
        case GARAGE_STATE_CLOSING:
            stateString = "closing";
            break;
        case GARAGE_STATE_OPENING:
            stateString = "opening";
            break;
        case GARAGE_STATE_OPEN:
            stateString = "open";
            break;
        default: return;
    }

    int result = mqtt_publish(buffer, stateString);
    if (result < 0) {
        ESP_LOGI(TAG, "Failed to publish garage state, result: %d", result);
    } else {
        ESP_LOGI(TAG, "Published %s for garage %d", stateString, garage);
    }
}

void homeassistant_garage_available(int garage) {
    char buffer[50];
    sprintf(buffer, "homeassistant/cover/garage_%d/availability", garage);
    int result = mqtt_publish(buffer, "online");
    
    if (result < 0) {
        ESP_LOGI(TAG, "Failed to publish garage availability, result: %d", result);
    } else {
        ESP_LOGI(TAG, "Published online for garage %d", garage);
    }
}

void homeassistant_subscribe_garage(int garageNumber) {
    char buffer[50];
    sprintf(buffer, "homeassistant/cover/garage_%d/set", garageNumber);
    
    char* topic = malloc(strlen(buffer) + 1);
    strcpy(topic, buffer);
    int *grageNum = malloc(sizeof(int));
    *grageNum = garageNumber;
    mqtt_subscribe_callback(topic, homeassistant_garage_set, grageNum);
    // int msg_id = esp_mqtt_client_subscribe(mqtt_client, buffer, 0);
    // printf("Subscribe to %s, msg_id=%d\n", buffer, msg_id);
}

void homeassistant_broadcast_garage(int garageNumber) {
    char buffer[50];

    cJSON *root = cJSON_CreateObject();
  
    cJSON_AddStringToObject(root, "device_class", "garage");
    sprintf(buffer, "#%d", garageNumber);
    cJSON_AddStringToObject(root, "name", buffer);

    sprintf(buffer, "garage-door-7134-%d", garageNumber);
    cJSON_AddStringToObject(root, "unique_id", buffer);

    sprintf(buffer, "homeassistant/cover/garage_%d/set", garageNumber);
    cJSON_AddStringToObject(root, "command_topic", buffer);
    
    sprintf(buffer, "homeassistant/cover/garage_%d/state", garageNumber);
    cJSON_AddStringToObject(root, "state_topic", buffer);

    cJSON *availabililty = cJSON_AddObjectToObject(root, "availability");
    sprintf(buffer, "homeassistant/cover/garage_%d/availability", garageNumber);
    cJSON_AddStringToObject(availabililty, "topic", buffer);
    
    cJSON_AddNumberToObject(root, "qos", 1);
    cJSON_AddBoolToObject(root, "retain", false);
    cJSON_AddStringToObject(root, "payload_open", "OPEN");
    cJSON_AddStringToObject(root, "payload_stop", "");
    cJSON_AddStringToObject(root, "payload_close", "CLOSE");
    cJSON_AddStringToObject(root, "state_open", "open");
    cJSON_AddStringToObject(root, "state_opening", "opening");
    cJSON_AddStringToObject(root, "state_closed", "closed");
    cJSON_AddStringToObject(root, "state_closing", "closing");
    cJSON_AddStringToObject(root, "payload_available", "online");
    cJSON_AddStringToObject(root, "payload_not_available", "offline");
    cJSON_AddBoolToObject(root, "optimistic", false);

  
    cJSON *device = cJSON_AddObjectToObject(root, "device");
    cJSON_AddStringToObject(device, "name", "Garage Doors");
    cJSON *identifiers = cJSON_AddArrayToObject(device, "identifiers");
    
    cJSON_AddItemToArray(identifiers, cJSON_CreateString("garage-door-7134"));

    sprintf(buffer, "homeassistant/cover/garage_%d/config", garageNumber);
    char *message = cJSON_Print(root);
    
    int msg_id = mqtt_publish(buffer, message);
    if (msg_id < 0) {
        ESP_LOGI(TAG, "Failed to publish garage availability, result: %d", msg_id);
    } else {
        ESP_LOGI(TAG, "Send home assistant discovery message, msg_id=%d", msg_id);
    }
    
    cJSON_Delete(root);
}

void homeassistant_publish_discovery() {
    for (int i = 0; i < GARAGE_COUNT; i++) {
        homeassistant_subscribe_garage(i);
        homeassistant_broadcast_garage(i);
        
        usleep(1000000UL);

        homeassistant_garage_state_changed(i, garage_state[i].state);
        homeassistant_garage_available(i);
    }
}

void homeassistant_on_birth(const char *topic, const char *data, void *arg) {
    ESP_LOGI(TAG, "callback received data: %s", data);

    if (!strcmp(data, "online")) {
        ESP_LOGI(TAG, "Home Assistant is back online.");     
        homeassistant_publish_discovery();
    } else {
        ESP_LOGI(TAG, "Home Assistant is offline.");     
    }
}

void homeassistant_subscribe_birth() {
  mqtt_subscribe_callback("homeassistant/status", homeassistant_on_birth, NULL);
}


void homeassistant_init() {
    homeassistant_publish_discovery();
    homeassistant_subscribe_birth();
    garage_state_changed = homeassistant_garage_state_changed;
}