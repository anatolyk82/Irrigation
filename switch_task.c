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

        char* msg = (char*)malloc(4);
        if (msg == NULL) {
            printf("Not enough memory to create a payload string in a state message");
            continue;
        }
        memset(msg, 0, 4);

        char* topic = (char*)malloc(strlen(MQTT_STATE_TOPIC) + 1);
        if (topic == NULL) {
            printf("Not enough memory to create a topic string in a state message");
            free(msg);
            continue;
        }
        memset(topic, 0, (strlen(MQTT_STATE_TOPIC) + 1));

        strncpy( topic, MQTT_STATE_TOPIC, strlen(MQTT_STATE_TOPIC) );
        strcpy( msg, (pumpState ? "ON" : "OFF"));

        mqtt_string_t *message_topic = (mqtt_string_t*)malloc( sizeof(mqtt_string_t) );
        if ( message_topic == NULL ) {
            free(msg);
            free(topic);
            printf("Not enough memory to create a topic in a state message\n");
            continue;
        }

        mqtt_message_t *message = (mqtt_message_t*)malloc( sizeof(mqtt_message_t) );
        if ( message != NULL ) {
            message->payload = msg;
            message->payloadlen = strlen(msg);
            message->dup = 0;
            message->qos = MQTT_QOS1;
            message->retained = 0;

            message_topic->lenstring.data = topic;
            message_topic->lenstring.len = strlen(MQTT_STATE_TOPIC);

            message_data.topic = message_topic;
            message_data.message = message;

            if (xQueueSend(publish_queue, &message_data, 0) == pdFALSE) {
                printf("Publish queue overflow.\n");
            }
        } else {
            free(msg);
            free(topic);
            free(message_topic);
            printf("Not enough memory to create a state message\n");
        }
    }
}
