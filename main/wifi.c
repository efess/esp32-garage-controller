/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Common functions for protocol examples, to establish Wi-Fi or Ethernet connection.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "dns_server.h"
#include "./wifi.h"

wifi_fscan_status wifi_scan_status = 0;
static const char *TAG = "wifi";
static esp_netif_t *s_sta_netif = NULL;
static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;

static wifi_status_handler* handlers;
static int handlers_count = 0;

#if CONFIG_GARAGE_WIFI_SCAN_METHOD_FAST
#define GARAGE_WIFI_SCAN_METHOD WIFI_FAST_SCAN
#elif CONFIG_GARAGE_WIFI_SCAN_METHOD_ALL_CHANNEL
#define GARAGE_WIFI_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#endif

#if CONFIG_GARAGE_WIFI_CONNECT_AP_BY_SIGNAL
#define GARAGE_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#elif CONFIG_GARAGE_WIFI_CONNECT_AP_BY_SECURITY
#define GARAGE_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#endif

#define DEFAULT_SCAN_LIST_SIZE 15
#define MAX_STATUS_HANDLERS 50

void wifi_start_ap(void);
static int s_retry_num = 0;

void wifi_register_status_callback(wifi_status_handler handler)
{
    if (handlers_count >= MAX_STATUS_HANDLERS) {
        ESP_LOGE(TAG, "Max status handlers reached");
        return;
    }
    handlers[handlers_count++] = handler;
}

void wifi_publish_status(wifi_status status)
{
    for(int i = 0; i < handlers_count; i++) {
        handlers[i](status);
    }
}


static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_OWE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_OWE");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_ENTERPRISE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA3_ENT_192:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_ENT_192");
        break;
    case WIFI_AUTH_WPA3_EXT_PSK:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_EXT_PSK");
        break;
    case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE");
        break;
    default:
        ESP_LOGI(TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}
bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static void handler_on_wifi_scan_done(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    wifi_scan_status = WIFI_SCAN_STATE_DATA_AVAILABLE;
}

static void handler_on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t *)event_data;
    ESP_LOGI(TAG, "STA_DISCONNECTED_REASON: %d", disconnected->reason);
    switch(disconnected->reason) {
        case WIFI_REASON_AUTH_FAIL:
            ESP_LOGI(TAG, "AP Authentication Failed");
            break;
    }
    s_retry_num++;
    if (s_retry_num > 60) {
        ESP_LOGI(TAG, "WiFi Connect failed %d times, starting AP...", s_retry_num);
        /* let wifi_sta_do_connect() return */
        if (s_semph_get_ip_addrs) {
            xSemaphoreGive(s_semph_get_ip_addrs);
        }
        wifi_start_ap();
    } else {
        ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_ERR_WIFI_NOT_STARTED) {
            ESP_ERROR_CHECK(err);
        }
    }
}

static void handler_on_wifi_connect(void *esp_netif, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{

}

static void handler_on_sta_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    s_retry_num = 0;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    
    if (!is_our_netif(CONFIG_GARAGE_WIFI_STATION_NAME, event->esp_netif)) {
        ESP_LOGI(TAG, "IPv4 - Ignoring :  %s", esp_netif_get_desc(event->esp_netif));
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    if (s_semph_get_ip_addrs) {
        xSemaphoreGive(s_semph_get_ip_addrs);
    } else {
        ESP_LOGI(TAG, "- IPv4 address: " IPSTR ",", IP2STR(&event->ip_info.ip));
    }
    wifi_publish_status(WIFI_STATUS_CONNECTED_STATION);
}


void wifi_start(void)
{
    
    wifi_status_handler* handlers = malloc(sizeof(wifi_status_handler) * MAX_STATUS_HANDLERS);
    int handlers_count = 0;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    esp_netif_config.if_desc = CONFIG_GARAGE_WIFI_STATION_NAME;
    esp_netif_config.route_prio = 128;
    s_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);

    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_on_sta_got_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handler_on_wifi_connect, s_sta_netif));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &handler_on_wifi_scan_done, NULL));

    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    // wifi_register_status_callback(wifi_status_callback);
    ESP_ERROR_CHECK(wifi_connect());
}

void wifi_start_ap(void)
{
    esp_wifi_set_default_wifi_ap_handlers();

    esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_GARAGE_WIFI_STATION_NAME,
            .ssid_len = strlen(CONFIG_GARAGE_WIFI_STATION_NAME),
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 12,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    ESP_LOGI(TAG, "Set up softAP with IP: " IPSTR,  IP2STR(&ip_info.ip));

    ESP_LOGI(TAG, "AP initialized with SSID: '%s' ",
             CONFIG_GARAGE_WIFI_STATION_NAME);

    dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*" /* all A queries */, "WIFI_AP_DEF" /* softAP netif ID */);
    start_dns_server(&config);

    wifi_publish_status(WIFI_STATUS_CONNECTED_AP);
}

void wifi_stop(void)
{
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(s_sta_netif));
    esp_netif_destroy(s_sta_netif);
    s_sta_netif = NULL;
}


esp_err_t wifi_sta_do_connect(wifi_config_t wifi_config, bool wait)
{
    if (wait) {
        s_semph_get_ip_addrs = xSemaphoreCreateBinary();
        if (s_semph_get_ip_addrs == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    s_retry_num = 0;

    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connect failed! ret:%x", ret);
        return ret;
    }
    if (wait) {
        ESP_LOGI(TAG, "Waiting for IP(s)");
        xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
    }
    return ESP_OK;
}

esp_err_t wifi_sta_do_disconnect(void)
{
    // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_on_wifi_disconnect));
    // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_on_sta_got_ip));
    // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handler_on_wifi_connect));

    if (s_semph_get_ip_addrs) {
        vSemaphoreDelete(s_semph_get_ip_addrs);
    }

    wifi_publish_status(WIFI_STATUS_DISCONNECTED);
    return esp_wifi_disconnect();
}

void wifi_shutdown(void)
{
    wifi_sta_do_disconnect();
    wifi_stop();
}

esp_err_t wifi_connect(void)
{
    ESP_LOGI(TAG, "Start connect.");
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .scan_method = GARAGE_WIFI_SCAN_METHOD,
            .sort_method = GARAGE_WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };


    return wifi_sta_do_connect(wifi_config, true);
}

esp_err_t wifi_start_scan() 
{
    esp_err_t result = esp_wifi_scan_start(NULL, false);
    if (result == ESP_OK) {
        wifi_scan_status = WIFI_SCAN_STATE_SCANNING;
    }
    return result;
}

esp_err_t wifi_scan_record_count(uint16_t* ap_count) {
    return  esp_wifi_scan_get_ap_num(&ap_count);
}

esp_err_t wifi_scan_read_entry(cJSON *ap_entry) {
    
    wifi_ap_record_t ap_info;

    memset(&ap_info, 0, sizeof(ap_info));
    if(esp_wifi_scan_get_ap_record(&ap_info) != ESP_OK) {
        return ESP_FAIL;
    }
    
    cJSON_AddStringToObject(ap_entry, "ssid", (char*)ap_info.ssid);
    cJSON_AddNumberToObject(ap_entry, "signal", ap_info.rssi);
    cJSON_AddNumberToObject(ap_entry, "authmode", ap_info.authmode);
    
    return ESP_OK;
}