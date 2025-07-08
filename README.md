# Publisher

IoT gadget that sends / receives / acts on MQTT messages.

Using arduino library and PlatformIO to allow easy porting between ESP8266 / ESP32 / Raspberry Pi Pico.

Applications:

1. Alarm interface
* PIR states.
* Remote arm / disarm.

2. Garage door
* Door state.
* Open / Close.

3. Tank Water Level
* Water level.

## Documentation

Detailed technical documentation for various system components and features:

### Device Control & Management
- **[Reset Control via MQTT](docs/reset_control/reset_control_mqtt.md)** - Remote device reset functionality including WiFi and NVM clearing

### Hawkbit
- **[Hawkbit Client Flow](docs/hawkbit/hawkbit_client_flow.svg)** - Over-the-air (OTA) update client flow diagram showing device update process with hawkbit

### Communication Protocols
- *(coming soon)*

### Hardware Interfaces  
- *(coming soon)*

### System Features
- *(coming soon)*

### Development & Debugging
- *(coming soon)*

### Integration Examples
- *(coming soon)*

## Software Architecture
![Software Architecture](architecture.svg "Software Architecture")
