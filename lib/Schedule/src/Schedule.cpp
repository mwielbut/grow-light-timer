#include "Schedule.h"

#include <stdio.h>

bool scheduledOn(const Schedule &schedule, uint16_t minuteOfDay)
{
  const uint16_t on = schedule.onMinute;
  const uint16_t off = schedule.offMinute;

  if (on == off)
    return false;
  if (on < off)
    return minuteOfDay >= on && minuteOfDay < off;
  // Window wraps past midnight
  return minuteOfDay >= on || minuteOfDay < off;
}

bool parseTime(const char *text, uint16_t &minuteOfDay)
{
  if (text == nullptr)
    return false;

  unsigned hours = 0, minutes = 0;
  char trailing = 0;
  if (sscanf(text, "%u:%u%c", &hours, &minutes, &trailing) != 2)
    return false;
  if (hours > 23 || minutes > 59)
    return false;

  minuteOfDay = hours * 60 + minutes;
  return true;
}

void formatTime(uint16_t minuteOfDay, char *buf)
{
  minuteOfDay %= MINUTES_PER_DAY;
  snprintf(buf, 6, "%02u:%02u", minuteOfDay / 60, minuteOfDay % 60);
}

bool LightController::update(bool scheduledNow)
{
  // A scheduled transition ends any manual override.
  if (initialized_ && scheduledNow != lastScheduled_)
    overrideActive_ = false;

  initialized_ = true;
  lastScheduled_ = scheduledNow;

  return overrideActive_ ? overrideOn_ : scheduledNow;
}
