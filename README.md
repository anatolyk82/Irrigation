
# Requirments

 - [ESP Open SDK](https://github.com/pfalcon/esp-open-sdk/) - Install and compile Xtensa compilers following the instruction in the description
 - [esptool.py](https://github.com/espressif/esptool) - a tool to flash ESP8266
 - [ESP Open RTOS SDK](https://github.com/SuperHouse/esp-open-rtos) - FreeRTOS SDK
 - WiFi SSID and password must be specified in the file `private_ssid_config.h`
 - MQTT login and password must be specified in the file `private_mqtt_config.h`

# Architecture

 Currently the application has 4 tasks:
  - WiFi task - connects to a WiFi AP
  - MQTT task - connects to an MQTT server and sends. messages which it takes from a queue from other tasks in the app.
  - Beat task - report extra information about the device.
  - Switch task - performs switching of the pump and sends a confirmation as an mqtt message.

# Home Assistant configuration

It can be configured as a switch in Home Assistant. All mqtt topics can be configured in the file `irrigation.h`

```
- platform: mqtt
  name: "Pump"
  icon: mdi:flower
  state_topic: "irrigation/controller1"
  state_on: "ON"
  state_off: "OFF"
  command_topic: "irrigation/controller1/set"
  availability_topic: "irrigation/controller1/status"
  payload_available: "online"
  payload_not_available: "offline"
  optimistic: false
  json_attributes_topic: "irrigation/controller1/attributes"
  ```

# Plans

Add a water level sensor support
