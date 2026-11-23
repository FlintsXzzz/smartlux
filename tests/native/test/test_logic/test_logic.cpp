// Host tests for the pure SMARTLUX rules in firmware/SMARTLUX/logic.h.
// No hardware: these run with `pio test -d tests/native -e native`.
#include <unity.h>
#include "logic.h"

// --- lamp on/off decision (applyControl) -----------------------------------
void test_warmup_forces_off(void) {
  // PIR warm-up overrides every mode, including manual ON.
  TEST_ASSERT_FALSE(lampShouldBeOn(0, false, true, true));
  TEST_ASSERT_FALSE(lampShouldBeOn(1, false, true, true));
  TEST_ASSERT_FALSE(lampShouldBeOn(2, false, true, true));
}

void test_manual_modes(void) {
  TEST_ASSERT_TRUE(lampShouldBeOn(1, true, false, false));  // MANUAL_ON
  TEST_ASSERT_FALSE(lampShouldBeOn(2, true, true, true));   // MANUAL_OFF
}

void test_auto_requires_dark_and_motion(void) {
  TEST_ASSERT_TRUE(lampShouldBeOn(0, true, true, true));
  TEST_ASSERT_FALSE(lampShouldBeOn(0, true, false, true));
  TEST_ASSERT_FALSE(lampShouldBeOn(0, true, true, false));
  TEST_ASSERT_FALSE(lampShouldBeOn(0, true, false, false));
}

// --- button cycle: AUTO -> ON -> OFF -> AUTO ------------------------------
void test_cycle_mode(void) {
  TEST_ASSERT_EQUAL_INT(1, cycleModeValue(0));
  TEST_ASSERT_EQUAL_INT(2, cycleModeValue(1));
  TEST_ASSERT_EQUAL_INT(0, cycleModeValue(2));
}

// --- /api/override mode parsing (validated before the rate gate) -----------
void test_parse_mode(void) {
  TEST_ASSERT_EQUAL_INT(0, parseMode("auto"));
  TEST_ASSERT_EQUAL_INT(1, parseMode("on"));
  TEST_ASSERT_EQUAL_INT(2, parseMode("off"));
  TEST_ASSERT_EQUAL_INT(-1, parseMode("ON"));
  TEST_ASSERT_EQUAL_INT(-1, parseMode(""));
  TEST_ASSERT_EQUAL_INT(-1, parseMode(nullptr));
}

void test_mode_key(void) {
  TEST_ASSERT_EQUAL_STRING("auto", modeKeyFor(0));
  TEST_ASSERT_EQUAL_STRING("on", modeKeyFor(1));
  TEST_ASSERT_EQUAL_STRING("off", modeKeyFor(2));
  TEST_ASSERT_EQUAL_STRING("auto", modeKeyFor(99));
}

// --- LDR hysteresis and mapping -------------------------------------------
void test_light_hysteresis(void) {
  TEST_ASSERT_EQUAL_INT(0, lightDecision(1499, 1500, 1800));
  TEST_ASSERT_EQUAL_INT(-1, lightDecision(1500, 1500, 1800));  // keep
  TEST_ASSERT_EQUAL_INT(-1, lightDecision(1800, 1500, 1800));  // keep
  TEST_ASSERT_EQUAL_INT(1, lightDecision(1801, 1500, 1800));
}

void test_light_percent(void) {
  TEST_ASSERT_EQUAL_INT(0, lightPercent(-5));
  TEST_ASSERT_EQUAL_INT(0, lightPercent(0));
  TEST_ASSERT_EQUAL_INT(50, lightPercent(2048));
  TEST_ASSERT_EQUAL_INT(100, lightPercent(4095));
  TEST_ASSERT_EQUAL_INT(100, lightPercent(5000));
}

// --- delay-off window ------------------------------------------------------
void test_motion_active(void) {
  TEST_ASSERT_TRUE(motionActive(1000, 900, 200));    // 100 < 200
  TEST_ASSERT_FALSE(motionActive(1100, 900, 200));   // 200 == 200 -> closed
  TEST_ASSERT_FALSE(motionActive(1400, 900, 200));   // 500 > 200
}

// --- energy integration ----------------------------------------------------
void test_energy_increment(void) {
  // 5 V, 1000 mA for one hour = 5 Wh.
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, energyIncrementWh(5.0f, 1000.0f, 3600000UL));
  // 1 s at the same load = 5/3600 Wh.
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0 / 3600.0,
                            energyIncrementWh(5.0f, 1000.0f, 1000UL));
  // Zero current adds nothing.
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, energyIncrementWh(5.0f, 0.0f, 1000UL));
}

// --- savings vs an always-on lamp -----------------------------------------
void test_savings(void) {
  // 3 W baseline over 1 h = 3 Wh; lamp used 1.2 Wh -> 60 %.
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 60.0, computeSavingsPct(3.0, 1.2, 3600000UL));
  // No elapsed time -> 0 (avoids divide-by-zero).
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, computeSavingsPct(3.0, 1.0, 0UL));
  // Zero power -> 0.
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, computeSavingsPct(0.0, 0.0, 3600000UL));
  // Usage above baseline clamps to 0.
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, computeSavingsPct(1.0, 2.0, 3600000UL));
  // Zero lamp energy clamps to 100.
  TEST_ASSERT_DOUBLE_WITHIN(1e-12, 100.0, computeSavingsPct(1.0, 0.0, 3600000UL));
}

// --- GPIO32 override button: 50 ms debounce -------------------------------
void test_button_debounce_press_edge(void) {
  // Pin level: true = HIGH = released, false = LOW = pressed.
  TEST_ASSERT_TRUE(stableButtonState(true, true, 1000, 1000, 50));   // unchanged
  TEST_ASSERT_TRUE(stableButtonState(false, true, 1049, 1000, 50));  // 49 ms: keep old
  TEST_ASSERT_FALSE(stableButtonState(false, true, 1050, 1000, 50)); // 50 ms: accept press
}

void test_button_debounce_release_edge(void) {
  TEST_ASSERT_FALSE(stableButtonState(false, false, 2000, 2000, 50));
  TEST_ASSERT_FALSE(stableButtonState(true, false, 2049, 2000, 50)); // 49 ms: keep pressed
  TEST_ASSERT_TRUE(stableButtonState(true, false, 2050, 2000, 50));  // 50 ms: accept release
}

void test_button_debounce_bounce_rejected(void) {
  // Raw toggles faster than the window, so the debounced level never changes.
  TEST_ASSERT_TRUE(stableButtonState(false, true, 1010, 1010, 50));
  TEST_ASSERT_TRUE(stableButtonState(true, true, 1049, 1020, 50));   // 29 ms: still released
  TEST_ASSERT_TRUE(stableButtonState(true, true, 1070, 1020, 50));   // 50 ms: accept release
}

void test_button_debounce_zero_window(void) {
  TEST_ASSERT_FALSE(stableButtonState(false, true, 1000, 1000, 0));  // immediate
}

void test_lux_estimate(void) {
  // Out-of-range ADC is clamped, so powf never sees a negative base.
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 0.0f, luxEstimate(-5, 1000.0f));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1000.0f, luxEstimate(4095, 1000.0f));
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 1000.0f, luxEstimate(5000, 1000.0f));
}

// --- ADC median filter (R1) --------------------------------------------------
void test_adc_median_empty(void) {
  TEST_ASSERT_EQUAL_INT(0, adcMedian5(nullptr, 0));
  int s[1] = {123};
  TEST_ASSERT_EQUAL_INT(0, adcMedian5(s, -1));
}

void test_adc_median_small_windows(void) {
  int one[1] = {1500};
  TEST_ASSERT_EQUAL_INT(1500, adcMedian5(one, 1));
  int three[3] = {30, 10, 20};
  TEST_ASSERT_EQUAL_INT(20, adcMedian5(three, 3));
  int four[4] = {40, 10, 30, 20};
  TEST_ASSERT_EQUAL_INT(30, adcMedian5(four, 4));  // upper middle for even n
}

void test_adc_median_rejects_spike(void) {
  int s[5] = {1500, 1520, 4095, 1490, 1510};
  TEST_ASSERT_EQUAL_INT(1510, adcMedian5(s, 5));
  int t[5] = {900, 100, 4000, 1500, 1600};
  TEST_ASSERT_EQUAL_INT(1500, adcMedian5(t, 5));
}

void test_adc_median_clamps_to_five(void) {
  int s[7] = {5, 1, 3, 2, 4, 9999, -9999};
  TEST_ASSERT_EQUAL_INT(3, adcMedian5(s, 7));  // only first 5 considered
}

// --- two-level lamp brightness (R4) ------------------------------------------
void test_brightness_off_is_zero(void) {
  TEST_ASSERT_EQUAL_INT(0, lampBrightness(false, 0, 10000UL, 0UL, 30000UL, 15000UL));
  TEST_ASSERT_EQUAL_INT(0, lampBrightness(false, 1, 10000UL, 0UL, 30000UL, 15000UL));
}

void test_brightness_manual_on_full(void) {
  // MANUAL_ON ignores the motion timer entirely.
  TEST_ASSERT_EQUAL_INT(255, lampBrightness(true, 1, 999999UL, 0UL, 30000UL, 15000UL));
}

void test_brightness_auto_levels(void) {
  // remaining = delay - elapsed; dim when remaining <= dimMs.
  TEST_ASSERT_EQUAL_INT(255, lampBrightness(true, 0, 10000UL, 0UL, 30000UL, 15000UL));
  TEST_ASSERT_EQUAL_INT(255, lampBrightness(true, 0, 14999UL, 0UL, 30000UL, 15000UL));
  TEST_ASSERT_EQUAL_INT(128, lampBrightness(true, 0, 15000UL, 0UL, 30000UL, 15000UL));
  TEST_ASSERT_EQUAL_INT(128, lampBrightness(true, 0, 29999UL, 0UL, 30000UL, 15000UL));
  TEST_ASSERT_EQUAL_INT(0, lampBrightness(true, 0, 30000UL, 0UL, 30000UL, 15000UL));
}

void test_brightness_motion_resets_timer(void) {
  TEST_ASSERT_EQUAL_INT(255, lampBrightness(true, 0, 26000UL, 25000UL, 30000UL, 15000UL));
}

// --- dashboard auth helpers -------------------------------------------------
void test_format_mac_token(void) {
  char out[16];
  TEST_ASSERT_TRUE(formatMacToken(0ULL, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("000000000000", out);
  TEST_ASSERT_TRUE(formatMacToken(0xAABBCCDDEEFFULL, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("aabbccddeeff", out);
  TEST_ASSERT_FALSE(formatMacToken(0xAABBCCDDEEFFULL, out, 12));  // needs NUL
}

void test_token_equals(void) {
  TEST_ASSERT_TRUE(tokenEquals("abc", "abc"));
  TEST_ASSERT_FALSE(tokenEquals("abc", "abd"));       // last char differs
  TEST_ASSERT_FALSE(tokenEquals("abc", "abcd"));      // length differs
  TEST_ASSERT_TRUE(tokenEquals("", ""));
  TEST_ASSERT_FALSE(tokenEquals(nullptr, "abc"));
  TEST_ASSERT_FALSE(tokenEquals("abc", nullptr));
}

void test_valid_cookie_value(void) {
  TEST_ASSERT_TRUE(validCookieValue("aabbccddeeff", 64));
  TEST_ASSERT_TRUE(validCookieValue("Ab-9_x.y", 64));
  TEST_ASSERT_FALSE(validCookieValue("", 64));
  TEST_ASSERT_FALSE(validCookieValue("a;b", 64));
  TEST_ASSERT_FALSE(validCookieValue("a b", 64));
  TEST_ASSERT_FALSE(validCookieValue("a=b", 64));
  TEST_ASSERT_FALSE(validCookieValue("a,b", 64));
  TEST_ASSERT_FALSE(validCookieValue("0123456789", 5));
}

void test_find_cookie_value(void) {
  char out[SLX_TOKEN_MAX_LEN + 1];
  TEST_ASSERT_TRUE(findCookieValue("smartlux_auth=aabbccddeeff", "smartlux_auth",
                                   out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("aabbccddeeff", out);
  TEST_ASSERT_TRUE(findCookieValue("foo=1; smartlux_auth=aabbcc ; bar=2",
                                   "smartlux_auth", out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("aabbcc", out);
  TEST_ASSERT_FALSE(findCookieValue("smartlux_authx=aabbcc", "smartlux_auth",
                                    out, sizeof(out)));
  TEST_ASSERT_FALSE(findCookieValue("foo=1", "smartlux_auth", out, sizeof(out)));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_warmup_forces_off);
  RUN_TEST(test_manual_modes);
  RUN_TEST(test_auto_requires_dark_and_motion);
  RUN_TEST(test_cycle_mode);
  RUN_TEST(test_parse_mode);
  RUN_TEST(test_mode_key);
  RUN_TEST(test_light_hysteresis);
  RUN_TEST(test_light_percent);
  RUN_TEST(test_motion_active);
  RUN_TEST(test_energy_increment);
  RUN_TEST(test_savings);
  RUN_TEST(test_lux_estimate);
  RUN_TEST(test_adc_median_empty);
  RUN_TEST(test_adc_median_small_windows);
  RUN_TEST(test_adc_median_rejects_spike);
  RUN_TEST(test_adc_median_clamps_to_five);
  RUN_TEST(test_brightness_off_is_zero);
  RUN_TEST(test_brightness_manual_on_full);
  RUN_TEST(test_brightness_auto_levels);
  RUN_TEST(test_brightness_motion_resets_timer);
  RUN_TEST(test_button_debounce_press_edge);
  RUN_TEST(test_button_debounce_release_edge);
  RUN_TEST(test_button_debounce_bounce_rejected);
  RUN_TEST(test_button_debounce_zero_window);
  RUN_TEST(test_format_mac_token);
  RUN_TEST(test_token_equals);
  RUN_TEST(test_valid_cookie_value);
  RUN_TEST(test_find_cookie_value);
  return UNITY_END();
}
