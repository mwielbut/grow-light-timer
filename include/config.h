#pragma once

// Adafruit Power Relay FeatherWing signal pin. The wing selects its signal pin
// with a solder jumper -- this must match the jumper you closed.
#define RELAY_PIN 27

// Network name: the web UI is served at http://growlight.local
#define HOSTNAME "growlight"

// POSIX timezone string (Mountain time with automatic DST).
// Other US zones: "PST8PDT,M3.2.0,M11.1.0", "CST6CDT,M3.2.0,M11.1.0",
// "EST5EDT,M3.2.0,M11.1.0", Arizona (no DST): "MST7"
#define TIMEZONE "MST7MDT,M3.2.0,M11.1.0"

#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"

// Default schedule, used until one is saved from the web UI (24h "HH:MM")
#define DEFAULT_ON_TIME "07:00"
#define DEFAULT_OFF_TIME "19:00"
