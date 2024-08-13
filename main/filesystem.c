#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "./filesystem.h"

static const char *TAG = "fs";

//#if CONFIG_EXAMPLE_WEB_DEPLOY_SEMIHOST
esp_err_t init_fs(void)
{
    esp_err_t ret = esp_vfs_semihost_register(CONFIG_GARAGE_WEB_MOUNT_POINT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register semihost driver (%s)!", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    return ESP_OK;
}

// #endif

// #if CONFIG_EXAMPLE_WEB_DEPLOY_SF
// esp_err_t init_fs(void)
// {
//     esp_vfs_spiffs_conf_t conf = {
//         .base_path = CONFIG_EXAMPLE_WEB_MOUNT_POINT,
//         .partition_label = NULL,
//         .max_files = 5,
//         .format_if_mount_failed = false
//     };
//     esp_err_t ret = esp_vfs_spiffs_register(&conf);

//     if (ret != ESP_OK) {
//         if (ret == ESP_FAIL) {
//             ESP_LOGE(TAG, "Failed to mount or format filesystem");
//         } else if (ret == ESP_ERR_NOT_FOUND) {
//             ESP_LOGE(TAG, "Failed to find SPIFFS partition");
//         } else {
//             ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
//         }
//         return ESP_FAIL;
//     }

//     size_t total = 0, used = 0;
//     ret = esp_spiffs_info(NULL, &total, &used);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
//     } else {
//         ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
//     }
//     return ESP_OK;
// }
// #endif