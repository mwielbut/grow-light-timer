# WiFi Relay

Internet connectivity monitor with automatic modem reset. Monitors internet connectivity and automatically power-cycles the modem if connectivity is lost while WiFi remains connected.

## Hardware

- **Board:** Adafruit Feather ESP32 v2
- **Platform:** espressif32
- **Additional:** Relay module connected to GPIO 27

### Wiring

| Feather ESP32 v2 | Relay Module |
|------------------|--------------|
| GPIO 27          | Signal/IN    |
| 3.3V             | VCC          |
| GND              | GND          |

The relay should be wired to interrupt power to your modem/router.

## How It Works

1. Connects to WiFi using credentials from `credentials.h`
2. Every 24 hours, attempts to connect to google.com on port 80
3. If connection fails while WiFi is still connected:
   - Activates relay (turns off modem power)
   - Waits 10 seconds
   - Deactivates relay (restores modem power)
   - Waits 10 minutes for modem to boot
4. LED blinks continuously to indicate the device is running
5. Built-in LED indicates WiFi connection status

### Timing Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| `checkInterval` | 24 hours | Time between connectivity checks |
| `requestTimeout` | 10 seconds | Timeout for HTTP connection attempt |
| `resetTime` | 10 seconds | Duration relay stays on during reset |
| `modemBootTime` | 10 minutes | Wait time after modem reset |

## Configuration

Create `include/credentials.h` with your WiFi credentials:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

## Building and Flashing

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor serial output
pio device monitor
```

## Dependencies

- WiFi (built into ESP32 Arduino framework)
