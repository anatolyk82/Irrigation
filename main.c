#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <paho_mqtt_c/MQTTClient.h>

#include "irrigation.h"

SemaphoreHandle_t wifi_alive;
QueueHandle_t publish_queue;
TaskHandle_t switchTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;

bool pumpState = false;  /* The current state of the pump */

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    vSemaphoreCreateBinary(wifi_alive);
    publish_queue = xQueueCreate(3, sizeof(mqtt_message_data_t) );

    xTaskCreate(&wifi_task, "wifi_task",  256, NULL, 1, NULL);
    xTaskCreate(&beat_task, "beat_task", 256, NULL, 1, NULL);
    xTaskCreate(&mqtt_task, "mqtt_task", 1024, NULL, 1, &mqttTaskHandle);
    xTaskCreate(&switch_task, "switch_task", 256, NULL, 1, &switchTaskHandle);
}

