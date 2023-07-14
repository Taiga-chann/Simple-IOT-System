/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT_EXAMPLE";

// function to convert a float number to a char array
char *gcvtf(float number, int ndigit, char *buf);
char    TEMP[4];
char    HUMID[4];
char    RAIN[4];
// ARRAY VALUE TEMP
float   temp[40]    =   {31.5, 33.2, 36.3, 25.3, 25.4, 37.2, 29.7, 27.6, 34.7, 30.8,
                        32.4, 29.1, 35.5, 34.2, 34.4, 27.0, 25.3, 34.7, 27.4, 25.3,
                        29.2, 35.6, 25.1, 24.3, 26.2, 37.7, 32.5, 33.6, 24.6, 27.7,
                        35.8, 29.6, 27.2, 28.4, 28.7, 30.6, 32.4, 29.7, 27.5, 26.8};
// ARRAY VALUE HUMID
float   humid[40]   =   {24.0, 75.2, 32.4, 43.1, 23.3, 57.6, 42.5, 76.5, 78.1, 41.6,
                        52.2, 40.4, 37.3, 43.3, 57.2, 79.0, 46.1, 47.6, 58.8, 46.5,
                        25.0, 39.1, 29.0, 40.5, 69.3, 47.3, 40.1, 42.5, 46.6, 68.2,
                        71.6, 29.5, 46.8, 72.9, 36.0, 39.5, 55.3, 34.2, 76.0, 24.8};
// ARRAY VALUE RAIN
float rain[40]      =   {90, 112, 86, 93, 101, 80, 100, 97, 96, 96,
                        92, 96, 94, 94, 89, 87, 91, 93, 95, 99,
                        100, 102, 103, 100, 98, 97, 99, 97, 96, 94,
                        92, 91, 90, 91, 89, 87, 86, 87, 89, 91};

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            int i = 0;
            // WHILE AFTER CONNECTED AND SEND VALUE TEMP & HUMID TO MQTT 
            while (1)
            {
                // CONVERT FLOAT VALUE TO STRING VALUE WITH 4 CHAR (string.TEMP = float.temp[i])
                gcvtf(temp[i], 4, TEMP);
                // PUBLISH VALUE TEMP IN TOPIC /smarthome/temp
                msg_id = esp_mqtt_client_publish(client, "/smarthome/temp", TEMP, 0, 0, 0);
                ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
                
                gcvtf(humid[i], 4, HUMID);
                // PUBLISH VALUE HUMID IN TOPIC /smarthome/humid
                msg_id = esp_mqtt_client_publish(client, "/smarthome/humid", HUMID, 0, 0, 0);
                ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

                gcvtf(rain[i], 4, RAIN);
                // PUBLISH VALUE RAIN IN TOPIC /smarthome/rain
                msg_id = esp_mqtt_client_publish(client, "/smarthome/rain", RAIN, 0, 0, 0);
                ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

                // MAKE DELAY 10s TO PUBLISH DATA
                vTaskDelay(10000/portTICK_RATE_MS);
                // RAISE COUNT FOR ARRAY TEMP AND HUMID
                i = (i+1)%40;
            }
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
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = "mqtt.flespi.io",
        .username = "PUT_YOUR_TOKEN_HERE",
        .password = "",
        .port = 1883
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}
