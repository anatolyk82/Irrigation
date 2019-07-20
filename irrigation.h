#ifndef MYTASKS_H
#define MYTASKS_H

#include <task.h>
#include <semphr.h>
#include <arpa/inet.h>

#define CONTROLLER_ID "1"

#define PUMP_PIN 2

extern bool pumpState;

extern SemaphoreHandle_t wifi_alive;
extern QueueHandle_t publish_queue;
extern TaskHandle_t switchTaskHandle;
extern TaskHandle_t mqttTaskHandle;

extern void wifi_task(void *pvParameters);
extern void beat_task(void *pvParameters);
extern void mqtt_task(void *pvParameters);
extern void switch_task(void *pvParameters);

#define MQTT_HOST "192.168.1.2"
#define MQTT_PORT 1883
#define MQTT_SUBSCRIBE_TOPIC "irrigation/controller" CONTROLLER_ID "/set"
#define MQTT_STATE_TOPIC "irrigation/controller" CONTROLLER_ID
#define MQTT_STATUS_TOPIC "irrigation/controller" CONTROLLER_ID "/status"
#define MQTT_ATTRIBUTES_TOPIC "irrigation/controller" CONTROLLER_ID "/attributes"
#define MQTT_STATUS_ONLINE "online"
#define MQTT_STATUS_OFFLINE "offline"

#define MQTT_STATE_PUBLISH_PERIOD 10*60*1000 //ms

#define BEAT_MSG_LEN 64

#endif //MYTASKS_H

