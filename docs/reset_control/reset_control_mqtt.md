# Reset Control via MQTT

This document describes how to control device resets remotely using MQTT commands in the Publisher project.

## Overview

The Publisher device supports remote reset functionality through MQTT messages. This allows for different types of resets to be triggered remotely, from a simple restart to clearing WiFi credentials and NVM settings.

## MQTT Topic Structure

The device subscribes to the following MQTT topic for reset commands:

```
[mqtt_prefix]/[hostname]/module command
```

Where:
- `mqtt_prefix`: Configured MQTT topic root (from NVM settings)
- `hostname`: Device hostname (derived from WiFi module details)

## Reset Command Format

Reset commands are sent as JSON messages with the following structure:

```json
{
  "reset": <reset_type>
}
```

Where `<reset_type>` is an integer representing the type of reset to perform.

## Reset Types

The following reset types are supported:

| Reset Type | Value | Description |
|------------|-------|-------------|
| `rstTypeNone` | 0 | No reset (cancels pending reset) |
| `rstTypeReset` | 1 | Simple device restart |
| `rstTypeResetWiFi` | 2 | Restart + clear WiFi settings |
| `rstTypeResetWiFiNvm` | 3 | Restart + clear WiFi + NVM settings |

## Reset Behavior

### Simple Reset (`rstTypeReset` = 1)
- Performs immediate device restart
- Preserves all configuration settings
- Device will reconnect with existing settings

### WiFi Reset (`rstTypeResetWiFi` = 2)
- Clears WiFi configuration settings
- Performs device restart
- Device will enter WiFi configuration mode on next boot
- NVM settings are preserved

### WiFi + NVM Reset (`rstTypeResetWiFiNvm` = 3)
- Clears all WiFi configuration settings
- Clears all NVM (Non-Volatile Memory) settings
- Performs device restart
- Device will enter initial setup mode

## Example Commands

### Simple Restart
```bash
# Using mosquitto_pub
mosquitto_pub -h <mqtt_broker> -t "devices/publisher001/module command" -m '{"reset": 1}'
```

### Clear WiFi and Restart
```bash
mosquitto_pub -h <mqtt_broker> -t "devices/publisher001/module command" -m '{"reset": 2}'
```

### Factory Reset (Clear WiFi + NVM)
```bash
mosquitto_pub -h <mqtt_broker> -t "devices/publisher001/module command" -m '{"reset": 3}'
```

### Cancel Pending Reset
```bash
mosquitto_pub -h <mqtt_broker> -t "devices/publisher001/module command" -m '{"reset": 0}'
```

## Reset State Machine

The reset controller implements a state machine with the following states:

- **stmResetIdle**: Normal operation, waiting for reset requests
- **stmReset**: Reset type 1 requested
- **stmResetWiFi**: Reset type 2 requested  
- **stmResetWiFiNvm**: Reset type 3 requested
- **stmResetAllowCancel**: Cancellation period before executing reset
- **stmResetAction**: Performing the actual reset

## Physical Reset Switch Integration

The system also supports physical reset switch functionality:

- **2 seconds**: Triggers WiFi reset (`rstTypeResetWiFi`)
- **8 seconds**: Triggers WiFi + NVM reset (`rstTypeResetWiFiNvm`)
- Physical switch can be enabled/disabled via configuration

## Cancellation Window

When a reset is requested via MQTT, there is a cancellation period before the reset is executed:

- **Fast cancel**: 2 seconds for switch-triggered resets
- **Slow cancel**: 5 seconds for MQTT-triggered resets

During this window, sending `{"reset": 0}` will cancel the pending reset.

## Debug Information

Reset operations generate debug messages that can be monitored:

```
resetCtrl: State change to <state> (request type <type>)
resetCtrl: Rebooting unit + clearing WiFi settings...
resetCtrl: Reset will occur in Xs
```

## Priority Handling

The reset controller processes requests in the following priority order:

1. **OTA Reboot**: Highest priority when OTA update requires restart
2. **MQTT Reset Request**: Medium priority for remote reset commands
3. **Physical Switch**: Lowest priority for manual reset operations

## Error Handling

- Invalid reset type values are ignored
- Reset requests are only accepted when the state machine is in idle state
- Message buffer overflow protection prevents malformed commands
- JSON parsing errors are logged and rejected

## Security Considerations

- MQTT authentication should be enabled on the broker
- Consider using TLS/SSL for MQTT connections
- Physical access to the reset switch provides full device control
- WiFi + NVM reset will require device reconfiguration

## Integration Example

Here's a Python example for sending reset commands:

```python
import paho.mqtt.client as mqtt
import json

def send_reset_command(mqtt_client, device_hostname, reset_type):
    """
    Send a reset command to a device
    
    Args:
        mqtt_client: Connected MQTT client
        device_hostname: Target device hostname
        reset_type: Reset type (0-3)
    """
    topic = f"devices/{device_hostname}/module command"
    payload = json.dumps({"reset": reset_type})
    
    mqtt_client.publish(topic, payload)
    print(f"Sent reset command {reset_type} to {device_hostname}")

# Usage
client = mqtt.Client()
client.connect("mqtt_broker_address", 1883, 60)

# Simple restart
send_reset_command(client, "publisher001", 1)

# Factory reset
send_reset_command(client, "publisher001", 3)
```

## Related Modules

- **MQTT Module**: Handles message reception and topic subscription
- **WiFi Module**: Manages WiFi configuration and connection
- **NVM Module**: Handles non-volatile memory operations
- **Debug Module**: Provides logging and status reporting
- **Status Controller**: Manages LED status indicators during reset operations
