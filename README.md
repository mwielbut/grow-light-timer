# Grow Light Timer

Turns an AC grow light (for a fiddle leaf fig) on and off on a daily schedule. The time comes from NTP, and you set the schedule from a small web page on your local network.

## Hardware

- **Board:** Adafruit Feather ESP32 v2
- **Relay:** [Adafruit Power Relay FeatherWing](https://www.adafruit.com/product/3191), signal jumper set to **GPIO 27**
- **Load:** AC grow light

### Wiring

The FeatherWing stacks on the Feather, so it needs no signal wiring. Its signal pin is chosen with a solder jumper on the wing. Close the jumper for pin 27, or change `RELAY_PIN` in [include/config.h](include/config.h) to match the jumper you closed.

Put the relay in series with the light's **hot** wire:

| Relay terminal | Connection |
|----------------|------------|
| COM            | Hot (line) from the wall |
| NO             | Hot to the grow light |
| NC             | Unused |

Neutral and ground go straight through to the light without touching the relay. With NO wiring, the light stays **off** whenever the board is unpowered, rebooting, or waiting for the time.

> ⚠️ **Mains voltage.** Put the FeatherWing in a closed, non-conductive enclosure with strain relief on the cords. Stay within the relay's rating (the light's draw is far below it). If you'd rather not handle mains wiring at all, an enclosed relay outlet box such as the Adafruit "Controllable Four Outlet Power Relay Module" (IoT Power Relay) works with this same firmware. Wire its control input to GPIO 27 and GND.

## Features

- **Daily schedule.** Defaults to **7:00 AM – 7:00 PM** (12 hours). Windows that cross midnight (e.g. 20:00 – 06:00) also work.
- **Web UI** at **http://growlight.local** (or the IP address printed on the serial monitor):
  - shows whether the light is on and the device's current time
  - edits the on/off times; the schedule is saved to flash and survives reboots and power loss
  - **Turn on / Turn off** manual override, which lasts until the next scheduled change and then the schedule takes over again
  - **Resume schedule** cancels an override immediately
- **Timezone:** US Mountain with automatic DST (`TIMEZONE` in [include/config.h](include/config.h))
- **Fail-safe:** after a power cycle the light stays off until NTP sets the clock. Once the clock is set, the schedule keeps running from the internal clock even if WiFi drops, and the board reconnects in the background.
- **Status LED:** the built-in red LED shows the relay state.

### HTTP API

| Method | Path | Params | Description |
|--------|------|--------|-------------|
| GET  | `/api/status`   | – | JSON: `on`, `override`, `synced`, `onTime`, `offTime`, `now` |
| POST | `/api/schedule` | `onTime`, `offTime` (`HH:MM`, 24h) | Save a new schedule |
| POST | `/api/override` | `state` = `on` \| `off` \| `clear` | Manual override |

```bash
curl -d state=on http://growlight.local/api/override
```

## Configuration

Copy `src/credentials.h.example` to `src/credentials.h` and fill in your WiFi details. The real file is gitignored.

Everything else is in [include/config.h](include/config.h): relay pin, hostname, timezone, NTP servers, and the default schedule.

## Building and Flashing

```bash
pio run                   # build
pio run -t upload         # flash
pio device monitor        # serial output
pio test -e native        # unit tests for the schedule logic (runs on your computer)
```

## Project Layout

| Path | Purpose |
|------|---------|
| [src/main.cpp](src/main.cpp) | WiFi, NTP, relay control, web server |
| [src/web_page.h](src/web_page.h) | Web UI (HTML/JS stored in flash) |
| [lib/Schedule/](lib/Schedule/) | Schedule logic and manual override with no hardware dependencies |
| [test/test_schedule/](test/test_schedule/) | Native Unity tests for `lib/Schedule` |
| [include/config.h](include/config.h) | User configuration |
