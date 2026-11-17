// Grow Light Timer
//
// Switches an AC grow light on/off on a daily schedule using an Adafruit
// Power Relay FeatherWing. Time comes from NTP; the schedule is set from a
// small web page (http://growlight.local) and persisted in flash.

#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>

#include "Schedule.h"
#include "config.h"
#include "credentials.h"
#include "web_page.h"

static const unsigned long CONTROL_INTERVAL_MS = 1000;
static const unsigned long WIFI_RETRY_MS = 30000;
static const time_t MIN_VALID_EPOCH = 1700000000; // Nov 2023: clock has been set

WebServer server(80);
Preferences prefs;

Schedule schedule;
LightController controller;
bool lightOn = false;
bool timeSynced = false;
bool mdnsStarted = false;

// ---------------------------------------------------------------- schedule

void loadSchedule()
{
  uint16_t defOn = 0, defOff = 0;
  parseTime(DEFAULT_ON_TIME, defOn);
  parseTime(DEFAULT_OFF_TIME, defOff);

  prefs.begin("growlight", true);
  schedule.onMinute = prefs.getUShort("on", defOn);
  schedule.offMinute = prefs.getUShort("off", defOff);
  prefs.end();

  // Guard against corrupted values
  if (schedule.onMinute >= MINUTES_PER_DAY)
    schedule.onMinute = defOn;
  if (schedule.offMinute >= MINUTES_PER_DAY)
    schedule.offMinute = defOff;
}

void saveSchedule()
{
  prefs.begin("growlight", false);
  prefs.putUShort("on", schedule.onMinute);
  prefs.putUShort("off", schedule.offMinute);
  prefs.end();
}

// ------------------------------------------------------------------ relay

void setLight(bool on)
{
  if (on == lightOn)
    return;
  lightOn = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
  Serial.printf("Light %s\n", on ? "ON" : "OFF");
}

bool currentMinuteOfDay(uint16_t &minuteOfDay)
{
  time_t now = time(nullptr);
  if (now < MIN_VALID_EPOCH)
    return false;

  struct tm local;
  localtime_r(&now, &local);
  minuteOfDay = local.tm_hour * 60 + local.tm_min;
  return true;
}

void updateLight()
{
  uint16_t minute;
  if (!currentMinuteOfDay(minute))
  {
    // No valid time yet (just powered up, no NTP): the schedule can't run, so
    // keep the light off unless it was switched on manually.
    setLight(controller.overrideActive() && controller.overrideOn());
    return;
  }

  if (!timeSynced)
  {
    timeSynced = true;
    Serial.println("Time synced");
  }

  setLight(controller.update(scheduledOn(schedule, minute)));
}

// -------------------------------------------------------------------- web

void sendStatus()
{
  char onBuf[6], offBuf[6], nowBuf[32] = "";
  formatTime(schedule.onMinute, onBuf);
  formatTime(schedule.offMinute, offBuf);

  if (timeSynced)
  {
    time_t now = time(nullptr);
    struct tm local;
    localtime_r(&now, &local);
    strftime(nowBuf, sizeof(nowBuf), "%a %b %d %I:%M %p %Z", &local);
  }

  char json[256];
  snprintf(json, sizeof(json),
           "{\"on\":%s,\"override\":%s,\"synced\":%s,\"onTime\":\"%s\",\"offTime\":\"%s\",\"now\":\"%s\"}",
           lightOn ? "true" : "false",
           controller.overrideActive() ? "true" : "false",
           timeSynced ? "true" : "false",
           onBuf, offBuf, nowBuf);
  server.send(200, "application/json", json);
}

void handleSchedule()
{
  uint16_t on, off;
  if (!parseTime(server.arg("onTime").c_str(), on) ||
      !parseTime(server.arg("offTime").c_str(), off))
  {
    server.send(400, "text/plain", "Times must be HH:MM (24h)");
    return;
  }
  if (on == off)
  {
    server.send(400, "text/plain", "On and off times must differ");
    return;
  }

  schedule.onMinute = on;
  schedule.offMinute = off;
  saveSchedule();
  controller.clearOverride();
  updateLight();

  Serial.printf("Schedule set: on %s, off %s\n",
                server.arg("onTime").c_str(), server.arg("offTime").c_str());
  sendStatus();
}

void handleOverride()
{
  String state = server.arg("state");
  if (state == "on")
    controller.setOverride(true);
  else if (state == "off")
    controller.setOverride(false);
  else if (state == "clear")
    controller.clearOverride();
  else
  {
    server.send(400, "text/plain", "state must be on, off or clear");
    return;
  }

  updateLight();
  Serial.printf("Override: %s\n", state.c_str());
  sendStatus();
}

void setupWebServer()
{
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/schedule", HTTP_POST, handleSchedule);
  server.on("/api/override", HTTP_POST, handleOverride);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

// ------------------------------------------------------------------- wifi

void maintainWiFi()
{
  static unsigned long lastAttempt = 0;

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!mdnsStarted && MDNS.begin(HOSTNAME))
    {
      MDNS.addService("http", "tcp", 80);
      mdnsStarted = true;
      Serial.printf("Web UI: http://%s.local (http://%s)\n", HOSTNAME,
                    WiFi.localIP().toString().c_str());
    }
    return;
  }

  // The schedule keeps running from the internal clock while offline.
  if (millis() - lastAttempt >= WIFI_RETRY_MS)
  {
    lastAttempt = millis();
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
}

// ------------------------------------------------------------------- main

void setup()
{
  // Relay off immediately at boot so the light never flickers on.
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  delay(500);
  Serial.println("\nGrow Light Timer starting");

  loadSchedule();
  char onBuf[6], offBuf[6];
  formatTime(schedule.onMinute, onBuf);
  formatTime(schedule.offMinute, offBuf);
  Serial.printf("Schedule: on %s, off %s\n", onBuf, offBuf);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  // SNTP runs in the background and resyncs periodically.
  configTzTime(TIMEZONE, NTP_SERVER_1, NTP_SERVER_2);

  setupWebServer();
}

void loop()
{
  static unsigned long lastControl = 0;

  server.handleClient();
  maintainWiFi();

  if (millis() - lastControl >= CONTROL_INTERVAL_MS)
  {
    lastControl = millis();
    updateLight();
  }
}
