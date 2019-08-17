#ifndef MYTASKS_H
#define MYTASKS_H

#include <task.h>
#include <semphr.h>
#include <arpa/inet.h>
#include <paho_mqtt_c/MQTTClient.h>

#define CONTROLLER_ID "1"

#define PUMP_PIN 15 //D8

extern bool pumpState;
uint8_t waterLevelPercentage;

extern SemaphoreHandle_t wifi_alive;
extern QueueHandle_t publish_queue;
extern TaskHandle_t switchTaskHandle;
extern TaskHandle_t mqttTaskHandle;

extern void wifi_task(void *pvParameters);
extern void beat_task(void *pvParameters);
extern void mqtt_task(void *pvParameters);
extern void switch_task(void *pvParameters);
extern void wl_sensor_task(void *pvParameters);

extern bool put_mqtt_message_to_queue(mqtt_message_data_t *mqtt_message, const char* _topic, uint16_t topic_length, const char* _payload, uint16_t payload_length, char qos);

#define MQTT_HOST "192.168.1.2"
#define MQTT_PORT 1883
#define MQTT_SUBSCRIBE_TOPIC "irrigation/controller" CONTROLLER_ID "/set"
#define MQTT_STATE_TOPIC "irrigation/controller" CONTROLLER_ID
#define MQTT_STATUS_TOPIC "irrigation/controller" CONTROLLER_ID "/status"
#define MQTT_ATTRIBUTES_TOPIC "irrigation/controller" CONTROLLER_ID "/attributes"
#define MQTT_STATUS_ONLINE "online"
#define MQTT_STATUS_OFFLINE "offline"
#define MQTT_WATER_LEVEL_TOPIC "irrigation/controller" CONTROLLER_ID "/waterlevel"

#define MQTT_ATTRIBUTES_PUBLISH_PERIOD 10*60*1000 //ms

#define BEAT_MSG_LEN 64


#define PIN_HP4067_S0 5
#define PIN_HP4067_S1 4
#define PIN_HP4067_S2 0
#define PIN_HP4067_S3 2

#endif //MYTASKS_H

