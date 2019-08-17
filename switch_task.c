#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

#include <string.h>
#include <paho_mqtt_c/MQTTClient.h>

#include "irrigation.h"


void switch_task(void *pvParameters)
{
    gpio_enable(PUMP_PIN, GPIO_OUTPUT);
    gpio_write(PUMP_PIN, 0);
    mqtt_message_data_t message_data;

    while (1) {
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        printf("Set a new state %d\n\r", pumpState);
        gpio_write(PUMP_PIN, pumpState);

        const char *st = pumpState ? "ON" : "OFF";

        put_mqtt_message_to_queue(&message_data, MQTT_STATE_TOPIC, strlen(MQTT_STATE_TOPIC), st, strlen(st), MQTT_QOS1);
    }
}
