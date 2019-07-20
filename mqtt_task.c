#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>

#include <semphr.h>

#include "irrigation.h"
#include "private_mqtt_config.h"

char myMAC[18]; //AA:BB:CC:DD:EE:FF

void beat_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    mqtt_message_data_t message_data;

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, MQTT_STATE_PUBLISH_PERIOD / portTICK_PERIOD_MS);

        /* Calculate uptime */
        TickType_t c = xTaskGetTickCount();
        unsigned long uptime = c * portTICK_PERIOD_MS / 1000;
        uint8_t seconds = uptime % 60;
        uint8_t minutes = uptime >= 60 ? ((uptime - uptime % 60) / 60) % 60 : 0;
        uint8_t hours = uptime >= 3600 ? ((uptime - uptime % 3600) / 3600) % 24 : 0;
        uint8_t days = uptime >= 86400 ? (uptime - uptime % 86400) / 86400 : 0;

        struct ip_info info;
        char myIPaddr[INET_ADDRSTRLEN];
        if ( sdk_wifi_get_ip_info(STATION_IF, &info) ) {
            inet_ntop(AF_INET, &(info.ip), myIPaddr, INET_ADDRSTRLEN);
        }

        char* msg = (char*)malloc(BEAT_MSG_LEN + 1);
        if (msg == NULL) {
            printf("Not enough memory to create a payload string in an attribute message");
            continue;
        }

        char* topic = (char*)malloc(strlen(MQTT_ATTRIBUTES_TOPIC) + 1);
        if (topic == NULL) {
            printf("Not enough memory to create a topic string in an attribute message");
            free(msg);
            continue;
        }
        topic[strlen(MQTT_ATTRIBUTES_TOPIC)] = '\0';
        strncpy( topic, MQTT_ATTRIBUTES_TOPIC, strlen(MQTT_ATTRIBUTES_TOPIC) );

        memset(msg, 0, (BEAT_MSG_LEN + 1));
        //snprintf(msg, BEAT_MSG_LEN, "{\"mac\":\"%s\",\"uptime\":\"%dT%02d:%02d:%02d\",\"ip\":\"%s\"}", myMAC, days, hours, minutes, seconds, myIPaddr);
        snprintf(msg, BEAT_MSG_LEN, "{\"uptime\":\"%dT%02d:%02d:%02d\",\"ip\":\"%s\"}", days, hours, minutes, seconds, myIPaddr);
        printf("State: %s\n", msg);

        mqtt_string_t *message_topic = (mqtt_string_t*)malloc( sizeof(mqtt_string_t) );
        if ( message_topic == NULL ) {
            free(msg);
            free(topic);
            printf("Not enough memory to create a topic in an attribute message\n");
            continue;
        }

        mqtt_message_t *message = (mqtt_message_t*)malloc( sizeof(mqtt_message_t) );
        if ( message != NULL ) {
            message->payload = msg;
            message->payloadlen = strlen(msg);
            message->dup = 0;
            message->qos = MQTT_QOS0;
            message->retained = 0;

            message_topic->lenstring.data = topic;
            message_topic->lenstring.len = strlen(MQTT_ATTRIBUTES_TOPIC);

            message_data.topic = message_topic;
            message_data.message = message;

            if (xQueueSend(publish_queue, &message_data, 0) == pdFALSE) {
                printf("Publish queue overflow.\n");
            }
        } else {
            free(msg);
            free(topic);
            free(message_topic);
            printf("Not enough memory to create an attribute message\n");
            continue;
        }
    }
}


void topic_received(mqtt_message_data_t *md)
{
    size_t expectedTopicLength = strlen(MQTT_SUBSCRIBE_TOPIC);

    if (md->topic->lenstring.len == expectedTopicLength) {
        char *receivedTopic = (char*)malloc( strlen(MQTT_SUBSCRIBE_TOPIC) + 1 );
        receivedTopic[expectedTopicLength] = '\0';
        strncpy(receivedTopic, md->topic->lenstring.data, expectedTopicLength);
        if (strcmp(receivedTopic, MQTT_SUBSCRIBE_TOPIC) == 0) {
            char *receivedPayload = (char*)malloc(4);
            memset(receivedPayload, 0, 4);
            if ( (md->message->payloadlen == 2) || (md->message->payloadlen == 3) ) {
                strncpy(receivedPayload, md->message->payload, md->message->payloadlen);
                bool newState, newStateValid = false;
                if ( strcmp(receivedPayload, "ON") == 0 ) {
                    newState = true;
                    newStateValid = true;
                } else if ( strcmp(receivedPayload, "OFF") == 0 ) {
                    newState = false;
                    newStateValid = true;
                } else {
                    newStateValid = false;
                    printf("Unexpected payload: %s\n", receivedPayload);
                }

                if ( newStateValid && (newState != pumpState) ) {
                    pumpState = newState;
                    /* Notify the switch task that there is a new state fr the pump */
                    xTaskNotifyGive( switchTaskHandle );
                }
            } else {
                printf("Unexpected length of the received payload: %d bytes\n", md->message->payloadlen);
            }
            free(receivedPayload);
        } else {
            printf("Received topic: %s\n", receivedTopic);
        }
        free(receivedTopic);
    } else {
        printf("Unexpected length of the received topic: %d bytes\n", md->topic->lenstring.len);
    }
}


void getMyMAC(char* mac)
{
    uint8_t rawMAC[6];

    memset(mac, 0, 18);
    sdk_wifi_get_macaddr(STATION_IF, (uint8_t *)rawMAC);
    sprintf(mac,"%02X:%02X:%02X:%02X:%02X:%02X",rawMAC[0],rawMAC[1],rawMAC[2],rawMAC[3],rawMAC[4],rawMAC[5]);
}


const char *getMyId(void)
{
    // Use MAC address for Station as unique ID
    static char my_id[13];
    static bool my_id_done = false;
    int8_t i;
    uint8_t x;
    if (my_id_done)
        return my_id;
    if (!sdk_wifi_get_macaddr(STATION_IF, (uint8_t *)my_id))
        return NULL;
    for (i = 5; i >= 0; --i)
    {
        x = my_id[i] & 0x0F;
        if (x > 9) x += 7;
        my_id[i * 2 + 1] = x + '0';
        x = my_id[i] >> 4;
        if (x > 9) x += 7;
        my_id[i * 2] = x + '0';
    }
    my_id[12] = '\0';
    my_id_done = true;
    return my_id;
}

void mqtt_task(void *pvParameters)
{
    int ret = 0;
    struct mqtt_network network;
    mqtt_client_t client = mqtt_client_default;
    char mqtt_client_id[20];
    uint8_t mqtt_buf[100];
    uint8_t mqtt_readbuf[100];
    mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;

    mqtt_network_new( &network );
    memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
    strcpy(mqtt_client_id, "ESP-");
    strcat(mqtt_client_id, getMyId());

    getMyMAC(myMAC);

    while(1) {
        xSemaphoreTake(wifi_alive, portMAX_DELAY);
        printf("%s: started\n", __func__);
        printf("%s: (Re)connecting to MQTT server %s ... ", __func__, MQTT_HOST);

        ret = mqtt_network_connect(&network, MQTT_HOST, MQTT_PORT);
        if (ret) {
            printf("error: %d\n", ret);
            taskYIELD();
            continue;
        }
        printf("done\n");

        mqtt_client_new(&client, &network, 5000, mqtt_buf, 100, mqtt_readbuf, 100);

        data.willFlag = 1;
        data.MQTTVersion = 3;
        data.clientID.cstring = mqtt_client_id;
        data.username.cstring = MQTT_USER;
        data.password.cstring = MQTT_PASS;
        data.keepAliveInterval = 10;
        data.cleansession = 0;
        data.will.topicName.cstring = MQTT_STATUS_TOPIC;
        data.will.message.cstring = MQTT_STATUS_OFFLINE;
        data.will.retained = 1;

        printf("Send MQTT connect ... ");
        ret = mqtt_connect(&client, &data);
        if (ret) {
            printf("error: %d\n", ret);
            mqtt_network_disconnect(&network);
            taskYIELD();
            vTaskDelay( 2000 / portTICK_PERIOD_MS );
            continue;
        } else {
            mqtt_message_t message;
            message.payload = MQTT_STATUS_ONLINE;
            message.payloadlen = strlen(MQTT_STATUS_ONLINE);
            message.dup = 0;
            message.qos = MQTT_QOS0;
            message.retained = 1;
            ret = mqtt_publish(&client, MQTT_STATUS_TOPIC, &message);
        }
        printf("done\n");

        mqtt_subscribe(&client, MQTT_SUBSCRIBE_TOPIC, MQTT_QOS1, topic_received);
        xQueueReset(publish_queue);

        mqtt_message_data_t *message_data = (mqtt_message_data_t*)malloc(sizeof(mqtt_message_data_t));
        if (message_data == NULL) {
            printf("Not enough memory to receive a message from the queue\n");
            continue;
        }
        while(1) {
            if(xQueueReceive(publish_queue, (void *)message_data, 0) == pdTRUE) {
                printf("Publish message: %s -> %s\n", (char*)message_data->topic->lenstring.data, (char*)message_data->message->payload);
                ret = mqtt_publish(&client, message_data->topic->lenstring.data, message_data->message);

                if (ret != MQTT_SUCCESS ) {
                    printf("Error while publishing message: %d\n", ret );
                }

                free( message_data->topic->lenstring.data );
                free( message_data->message->payload );
                free( message_data->topic );
                free( message_data->message );
            
                memset(message_data, 0, sizeof(mqtt_message_data_t));
            }

            ret = mqtt_yield(&client, 1000);
            if (ret == MQTT_DISCONNECTED) {
                break;
            }
        }
        free( message_data );
        printf("Connection dropped, request restart\n");
        mqtt_network_disconnect(&network);
        taskYIELD();
    }
}
