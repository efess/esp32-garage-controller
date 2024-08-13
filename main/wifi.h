#include "esp_wifi_types.h"
#include "cJSON.h"

typedef enum {
    WIFI_STATUS_DISCONNECTED                        = 0,
    WIFI_STATUS_CONNECTED_STATION                   = 1,
    WIFI_STATUS_CONNECTED_AP                        = 2
} wifi_status;

typedef enum {
    WIFI_SCAN_STATE_NONE                            = 0,
    WIFI_SCAN_STATE_SCANNING                        = 1,
    WIFI_SCAN_STATE_DATA_AVAILABLE                  = 2
} wifi_fscan_status;

typedef struct {
    char* ssid;
    wifi_auth_mode_t authmode;
} wifi_ap_info;

extern wifi_fscan_status wifi_scan_status;
typedef void (*wifi_status_handler)(wifi_status status);

void wifi_start(void);
void wifi_stop(void);
esp_err_t wifi_sta_do_connect(wifi_config_t wifi_config, bool wait);
esp_err_t wifi_sta_do_disconnect(void);
esp_err_t wifi_connect(void);
void wifi_shutdown(void);
void wifi_scan(void);
void wifi_register_status_callback(wifi_status_handler handler);
esp_err_t wifi_scan_record_count(uint16_t *count);
esp_err_t wifi_scan_read_entry(cJSON *ap);
esp_err_t wifi_start_scan();