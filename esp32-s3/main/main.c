#include "main.h"
#include "extensions.h"

httpd_handle_t serverHandle = NULL;
esp_netif_t* wifiAP;

static const char *TAG = "ESP32-S3 DevKit";

// To initiate download from the requester (IP in body of request)
esp_err_t getNewOTAImage(httpd_req_t *req){
    // Handle the request
    esp_ota_handle_t otaHandle;
    const esp_partition_t *updatePartition = esp_ota_get_next_update_partition(NULL);

    esp_ota_begin(updatePartition, OTA_SIZE_UNKNOWN, &otaHandle);
    char buf[1024];
    int received;
    int imageSize = req->content_len;
    ESP_LOGI(TAG, "Recieved request with %d of data.", imageSize);
    if(imageSize >= 1000000){
        return httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, NULL);
    }else{
        // Do things
        while(imageSize > 0){
            received = httpd_req_recv(req, buf, 1024);
            if (received <= 0){

            }
            esp_err_t writeCode = esp_ota_write(otaHandle, buf, received);
            if(writeCode != ESP_OK){
                if(writeCode == ESP_ERR_OTA_VALIDATE_FAILED)
                    httpd_resp_send_err(req, 500, "Unnable to validate image.");
                else
                    httpd_resp_send_err(req, 500, NULL);
                esp_ota_abort(otaHandle);
                return ESP_FAIL;
            }
            imageSize=imageSize - 1024;
        }
        if(esp_ota_end(otaHandle) != ESP_OK){
            httpd_resp_send_500(req);
            esp_ota_abort(otaHandle);
            return ESP_FAIL;
        }
    }
    httpd_resp_sendstr(req, "OTA update successful, rebooting");
    ESP_LOGI(TAG, "OTA update successful, rebooting");
    esp_ota_set_boot_partition(updatePartition);
    esp_restart();
    return ESP_OK;
}

esp_err_t healthCheckFunction(httpd_req_t *req){
    httpd_resp_send(req, "GOOD", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

//Function for starting the webserver
httpd_handle_t start_webserver(void){
    // Generate default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = SERVER_PORT;
    // Empty handle to http_server
    httpd_uri_t uriDef = {
        .uri       = "/update",
        .method    = HTTP_POST,
        .handler   = &getNewOTAImage,
        .user_ctx  = NULL
    };

    httpd_uri_t healthCheck = {
        .uri       = "/health",
        .method    = HTTP_GET,
        .handler   = &healthCheckFunction,
        .user_ctx  = NULL
    };
    // Start the httpd server
    if (httpd_start(&serverHandle, &config) == ESP_OK) {
            // Register URI handlers
            httpd_register_uri_handler(serverHandle, &uriDef);
            httpd_register_uri_handler(serverHandle, &healthCheck);
        }
    else{
        ESP_LOGE(TAG, "Unnable to open server.");
    }
    // If server failed to start, handle will be NULL
    return serverHandle;
}



void httpServerTask(void *pvParameter){
    serverHandle = start_webserver();
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
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t defaultConfig = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&defaultConfig);
    esp_wifi_set_config(WIFI_IF_AP, &wifiConfig);
    wifiAP=esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_start());
    xTaskCreate(&httpServerTask, "httpServerTask", (4096*2), NULL, 5, NULL);

}