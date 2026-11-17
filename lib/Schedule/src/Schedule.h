#pragma once

#include <stdint.h>

// Pure scheduling logic for the grow light. No Arduino dependencies so it can
// be unit tested natively (pio test -e native).

static const uint16_t MINUTES_PER_DAY = 24 * 60;

struct Schedule
{
  uint16_t onMinute;  // minute of day the light turns on  (0..1439)
  uint16_t offMinute; // minute of day the light turns off (0..1439)
};

// True if the schedule says the light should be on at the given minute of day.
// Handles windows that wrap past midnight (e.g. 20:00 -> 06:00).
// If onMinute == offMinute the window is empty and the light stays off.
bool scheduledOn(const Schedule &schedule, uint16_t minuteOfDay);

// Parses "HH:MM" (24h) into a minute of day. Returns false if invalid.
bool parseTime(const char *text, uint16_t &minuteOfDay);

// Formats a minute of day as "HH:MM". buf must hold at least 6 chars.
void formatTime(uint16_t minuteOfDay, char *buf);

// Manual override that lasts until the next scheduled transition: if you turn
// the light off at 3pm it stays off until the schedule next changes state
// (7pm off -> 7am on), at which point the schedule takes over again.
class LightController
{
public:
  // Feed the current scheduled state; returns whether the light should be on.
  bool update(bool scheduledNow);

  void setOverride(bool on) { overrideActive_ = true; overrideOn_ = on; }
  void clearOverride() { overrideActive_ = false; }
  bool overrideActive() const { return overrideActive_; }
  bool overrideOn() const { return overrideOn_; }

private:
  bool initialized_ = false;
  bool lastScheduled_ = false;
  bool overrideActive_ = false;
  bool overrideOn_ = false;
};
