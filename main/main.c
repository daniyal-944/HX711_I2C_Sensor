#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "HX711.h"
#include "mqtt_client.h"
#include "protocol_examples_common.h"
#include "esp_log.h"

#define AVG_SAMPLES    10
#define GPIO_DATA      GPIO_NUM_15
#define GPIO_SCLK      GPIO_NUM_16
#define PSI_SCALE      1481932.0f // Calibration scale factor for PSI

static const char *TAG = "HX711_PUBLISH";
static esp_mqtt_client_handle_t mqtt_client = NULL;

static void weight_reading_task(void* arg);
static void initialise_weight_sensor(void);

// === MQTT EVENT HANDLER ===
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "esp-tls error: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "tls stack error: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "socket errno: %d (%s)", event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;

        default:
            break;
    }
}

// === MQTT INIT ===
static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://mqtt.thingsboard.cloud",
        .credentials.username = "zPnoaOFKv3cTWKuztZzT",  // Replace with your actual ThingsBoard device token
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// === HX711 SENSOR TASK ===
static void weight_reading_task(void* arg)
{
    HX711_init(GPIO_DATA, GPIO_SCLK, eGAIN_128); 
    HX711_tare();
    HX711_set_scale(PSI_SCALE); // Set calibration scale

    while (1) {
        float psi = HX711_get_units(AVG_SAMPLES);
        ESP_LOGI(TAG, "Pressure = %.2f PSI", psi);

        char payload[64];
        snprintf(payload, sizeof(payload), "{\"pressure_psi\": %.2f}", psi);

        if (mqtt_client != NULL) {
            esp_mqtt_client_publish(mqtt_client, "v1/devices/me/telemetry", payload, 0, 1, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Send every 5 sec
    }
}

// === SENSOR TASK INIT ===
static void initialise_weight_sensor(void)
{
    ESP_LOGI(TAG, "Initializing Pressure Sensor Task...");
    xTaskCreatePinnedToCore(weight_reading_task, "pressure_reading_task", 4096, NULL, 1, NULL, 0);
}

// === MAIN ===
void app_main(void)
{
    ESP_LOGI(TAG, "Startup...");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    // Logging levels
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_WARN);

    // Init system
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Wi-Fi connect
    ESP_ERROR_CHECK(example_connect());

    // Start MQTT
    mqtt_app_start();

    // Start HX711 task
    initialise_weight_sensor();
}
