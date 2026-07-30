#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_mac.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_https_ota.h"
#include "esp_wifi.h"
#include "esp_ota_ops.h"
#include "esp_ota_ops.h"
#include "esp_netif_ip_addr.h"
#include "esp_event.h"


#include "string.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <sys/socket.h>
#include "string.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

esp_err_t getNewOTAImage(httpd_req_t *req);
esp_err_t healthCheckFunction(httpd_req_t *req);
void simple_ota_example_task(void *pvParameter);