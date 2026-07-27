#include "main.h"
#include "extensions.h"

//#include "protocol_examples_common.h"

httpd_handle_t serverHandle = NULL;


static const char *TAG = "ESP32-S3 DevKit";

esp_err_t _http_event_handler(esp_http_client_event_t *evt){
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_HEADERS_COMPLETE:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_STATUS_CODE:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}
// To initiate download from the requester (IP in body of request)
esp_err_t getNewOTAImage(httpd_req_t *req){
    // Handle the request
    esp_ota_handle_t ota_handle;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);


    char buf[1024];
    int received;
    int imageSize = req->content_len;
    if(imageSize >= 1000000){
        return httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, NULL);
    }else{
        // Do things
        while(imageSize > 0){
            received = httpd_req_recv(req, buf, 1024);
            if (received <= 0){

            }
            esp_ota_write(ota_handle, buf, received);
        }
        if(esp_ota_end(ota_handle) != ESP_OK){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    }
    esp_ota_set_boot_partition(update_partition);
    httpd_resp_sendstr(req, "OTA update successful, rebooting");
    esp_restart();
    return ESP_OK;
}


//Function for starting the webserver
httpd_handle_t start_webserver(void){
    // Generate default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Empty handle to http_server
    httpd_handle_t server = NULL;

    httpd_uri_t uriDef = {
        .uri       = "/",
        .method    = HTTP_ANY,
        .handler   = &getNewOTAImage,
        .user_ctx  = NULL
    };
    // Start the httpd server
    if (httpd_start(&server, &config) == ESP_OK) {
    // Register URI handlers
            httpd_register_uri_handler(server, &uriDef);
        }
    // If server failed to start, handle will be NULL
    return server;
}



void httpServerTask(void *pvParameter){


    simple_ota_example_task(NULL);
}

void simple_ota_example_task(void *pvParameter){
    ESP_LOGI(TAG, "Starting OTA example task");
    esp_http_client_config_t config = {
        .url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
        .cert_pem = NULL,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void app_main(void){

    // Sanity check
    ESP_LOGI(TAG, "Starting up the wifi.");

    // Flash initialization
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    
    wifi_config_t wifiConfig = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    wifi_init_config_t defaultConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_wifi_init(&defaultConfig);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    xTaskCreate(&simple_ota_example_task, "ota_example_task", (2000000), NULL, 5, NULL);

}