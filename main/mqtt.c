#include "mqtt_client.h"
#include "esp_log.h"
#include "./mqtt.h"
#include "homeassistant.h"

static const char *TAG = "mqtt";
esp_mqtt_client_handle_t _client;

struct subscription {
    const char *topic;
    mqtt_subscription_callback callback;
    void *arg;
    struct subscription *next;
};

typedef struct subscription subscription;

subscription *subscriptions = NULL;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        homeassistant_init(client);
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        for (subscription *sub = subscriptions; sub != NULL; sub = sub->next) {
            if (strncmp(sub->topic, event->topic, event->topic_len) == 0) {
                char *data = malloc(event->data_len + 1);
                strncpy(data, event->data, event->data_len);
                data[event->data_len] = '\0';
                if (sub->arg) {
                    ESP_LOGI(TAG, "Found subscription, executing callback for garage %d\n", *(int*)(sub->arg));
                } else {
                    ESP_LOGI(TAG, "Found subscription, executing callback");
                }
                sub->callback(event->topic, data, sub->arg);
                free(data);
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

int mqtt_publish(const char *topic, const char *data) {
    return esp_mqtt_client_publish(_client, topic, data, 0, 1, 0);
}

void mqtt_subscribe_callback(char *topic, mqtt_subscription_callback callback, void *arg) {
    
    bool found = false;
    for (subscription *sub = subscriptions; sub != NULL; sub = sub->next) {
        if (strcmp(sub->topic, topic) == 0) {
            found = true;
            break;
        }
    }

    if (!found) {
        subscription *sub = malloc(sizeof(subscription));
        sub->topic = topic;
        sub->callback = callback;
        sub->arg = arg;
        if (subscriptions) {
            sub->next = subscriptions;
        } else {
            sub->next = NULL;
        }

        subscriptions = sub;
    }

    esp_mqtt_client_subscribe(_client, topic, 0);
}

esp_err_t mqtt_init() 
{
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = "mqtt://mqtt.home.lan:1883",
  };

  _client = esp_mqtt_client_init(&mqtt_cfg);

  /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
  esp_mqtt_client_register_event(_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  return esp_mqtt_client_start(_client);
}
