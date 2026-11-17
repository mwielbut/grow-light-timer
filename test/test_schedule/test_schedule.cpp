#include <unity.h>

#include "Schedule.h"

static uint16_t at(int h, int m) { return h * 60 + m; }

void setUp() {}
void tearDown() {}

void test_daytime_window()
{
  Schedule s{at(7, 0), at(19, 0)};
  TEST_ASSERT_FALSE(scheduledOn(s, at(6, 59)));
  TEST_ASSERT_TRUE(scheduledOn(s, at(7, 0)));
  TEST_ASSERT_TRUE(scheduledOn(s, at(12, 0)));
  TEST_ASSERT_TRUE(scheduledOn(s, at(18, 59)));
  TEST_ASSERT_FALSE(scheduledOn(s, at(19, 0)));
  TEST_ASSERT_FALSE(scheduledOn(s, at(0, 0)));
}

void test_window_wrapping_midnight()
{
  Schedule s{at(20, 0), at(6, 0)};
  TEST_ASSERT_TRUE(scheduledOn(s, at(20, 0)));
  TEST_ASSERT_TRUE(scheduledOn(s, at(23, 59)));
  TEST_ASSERT_TRUE(scheduledOn(s, at(0, 0)));
  TEST_ASSERT_TRUE(scheduledOn(s, at(5, 59)));
  TEST_ASSERT_FALSE(scheduledOn(s, at(6, 0)));
  TEST_ASSERT_FALSE(scheduledOn(s, at(12, 0)));
}

void test_equal_times_means_always_off()
{
  Schedule s{at(8, 0), at(8, 0)};
  TEST_ASSERT_FALSE(scheduledOn(s, at(8, 0)));
  TEST_ASSERT_FALSE(scheduledOn(s, at(20, 0)));
}

void test_parse_time()
{
  uint16_t m = 0;
  TEST_ASSERT_TRUE(parseTime("07:00", m));
  TEST_ASSERT_EQUAL_UINT16(420, m);
  TEST_ASSERT_TRUE(parseTime("23:59", m));
  TEST_ASSERT_EQUAL_UINT16(1439, m);
  TEST_ASSERT_TRUE(parseTime("0:05", m));
  TEST_ASSERT_EQUAL_UINT16(5, m);

  TEST_ASSERT_FALSE(parseTime("24:00", m));
  TEST_ASSERT_FALSE(parseTime("12:60", m));
  TEST_ASSERT_FALSE(parseTime("noon", m));
  TEST_ASSERT_FALSE(parseTime("12:00pm", m));
  TEST_ASSERT_FALSE(parseTime("", m));
  TEST_ASSERT_FALSE(parseTime(nullptr, m));
}

void test_format_time()
{
  char buf[6];
  formatTime(at(7, 5), buf);
  TEST_ASSERT_EQUAL_STRING("07:05", buf);
  formatTime(at(19, 0), buf);
  TEST_ASSERT_EQUAL_STRING("19:00", buf);
}

void test_controller_follows_schedule()
{
  LightController c;
  TEST_ASSERT_FALSE(c.update(false));
  TEST_ASSERT_TRUE(c.update(true));
  TEST_ASSERT_FALSE(c.update(false));
}

void test_override_holds_until_next_transition()
{
  LightController c;
  TEST_ASSERT_TRUE(c.update(true)); // daytime

  c.setOverride(false); // manually turn off at 3pm
  TEST_ASSERT_FALSE(c.update(true));
  TEST_ASSERT_FALSE(c.update(true));
  TEST_ASSERT_TRUE(c.overrideActive());

  // 7pm: schedule turns off -> override cleared, schedule resumes
  TEST_ASSERT_FALSE(c.update(false));
  TEST_ASSERT_FALSE(c.overrideActive());

  // 7am: schedule turns back on normally
  TEST_ASSERT_TRUE(c.update(true));
}

void test_override_on_at_night()
{
  LightController c;
  c.update(false);
  c.setOverride(true);
  TEST_ASSERT_TRUE(c.update(false));
  TEST_ASSERT_TRUE(c.update(true)); // transition clears override, schedule says on
  TEST_ASSERT_FALSE(c.overrideActive());
}

void test_clear_override()
{
  LightController c;
  c.update(true);
  c.setOverride(false);
  TEST_ASSERT_FALSE(c.update(true));
  c.clearOverride();
  TEST_ASSERT_TRUE(c.update(true));
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_daytime_window);
  RUN_TEST(test_window_wrapping_midnight);
  RUN_TEST(test_equal_times_means_always_off);
  RUN_TEST(test_parse_time);
  RUN_TEST(test_format_time);
  RUN_TEST(test_controller_follows_schedule);
  RUN_TEST(test_override_holds_until_next_transition);
  RUN_TEST(test_override_on_at_night);
  RUN_TEST(test_clear_override);
  return UNITY_END();
}
