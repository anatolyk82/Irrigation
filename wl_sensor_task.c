#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include <espressif/esp_system.h>

#include <string.h>
#include <paho_mqtt_c/MQTTClient.h>

#include "irrigation.h"

#define ADC_THRESHOLD 200

// This level sensor is based on HP4067

void setPinAddress(uint8_t levelNumber) {
    uint8_t pinS0 = levelNumber & 0x01 ? 1 : 0;
    uint8_t pinS1 = levelNumber & 0x02 ? 1 : 0;
    uint8_t pinS2 = levelNumber & 0x04 ? 1 : 0;
    uint8_t pinS3 = levelNumber & 0x08 ? 1 : 0;

    //printf("S3:%d S2:%d S1:%d S0:%d\n", pinS3, pinS2, pinS1, pinS0);
    gpio_write(PIN_HP4067_S0, pinS0);
    gpio_write(PIN_HP4067_S1, pinS1);
    gpio_write(PIN_HP4067_S2, pinS2);
    gpio_write(PIN_HP4067_S3, pinS3);
}


void wl_sensor_task(void *pvParameters)
{
    const TickType_t delay = 1000 / portTICK_PERIOD_MS;
    
    /* Prepare address pins */
    gpio_enable(PIN_HP4067_S0, GPIO_OUTPUT);
    gpio_enable(PIN_HP4067_S1, GPIO_OUTPUT);
    gpio_enable(PIN_HP4067_S2, GPIO_OUTPUT);
    gpio_enable(PIN_HP4067_S3, GPIO_OUTPUT);

    gpio_write(PIN_HP4067_S0, 0);
    gpio_write(PIN_HP4067_S1, 0);
    gpio_write(PIN_HP4067_S2, 0);
    gpio_write(PIN_HP4067_S3, 0);

    mqtt_message_data_t message_data;
    char msg[4];

    while (1) {

        uint8_t localWaterLevelPercentage = 0;
        for (uint8_t lv = 0; lv < 10; lv++) {
            setPinAddress(lv);
            vTaskDelay( 50 / portTICK_PERIOD_MS );
            uint16_t adc_value = sdk_system_adc_read();
            //printf( "Level: %d, ADC voltage is %.3f (%d)\n", lv, 1.0 / 1024 * adc_value, adc_value );
            if (adc_value < ADC_THRESHOLD) {
                break;
            } else {
                localWaterLevelPercentage = (lv + 1) * 10;
            }
        }

        if (localWaterLevelPercentage != waterLevelPercentage) {
            waterLevelPercentage = localWaterLevelPercentage;

            memset(msg, 0, 4);
            snprintf(msg, 3, "%d", waterLevelPercentage);

            put_mqtt_message_to_queue(&message_data, MQTT_WATER_LEVEL_TOPIC, strlen(MQTT_WATER_LEVEL_TOPIC), msg, strlen(msg), MQTT_QOS1);
        }

        vTaskDelay( delay );
    }
}

