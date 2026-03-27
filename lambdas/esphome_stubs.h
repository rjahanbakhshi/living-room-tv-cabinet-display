#pragma once
// ESPHome lambda context stubs — used by clangd only, not compiled into firmware.
//
// Real ESPHome headers are resolved through the build include path set in .clangd:
//   .esphome/build/livingroom-tv-cabinet-display/src
//
// This file only declares what ESPHome generates at build time and what has
// no corresponding header: the id() macro and global variable pointers.

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/automation.h"

// ── id() macro ────────────────────────────────────────────────────────────────
// ESPHome generates per-variable macros like:
//   GlobalsComponent<float> *display_vol;
//   #define display_vol (*display_vol)
// We use a simple identity macro so id(x) → x for all declared vars below.
#define id(x) (x)

// ── Globals (from YAML globals: section) ─────────────────────────────────────
extern esphome::globals::GlobalsComponent<float>  *display_vol_ptr;
extern esphome::globals::GlobalsComponent<bool>   *was_receiver_on_ptr;
extern esphome::globals::GlobalsComponent<float>  *pending_volume_ptr;
extern esphome::globals::GlobalsComponent<int>    *touch_zone_ptr;
extern esphome::globals::GlobalsComponent<int>    *touch_last_y_ptr;
extern esphome::globals::GlobalsComponent<int>    *touch_start_x_ptr;
extern esphome::globals::GlobalsComponent<int>    *touch_start_y_ptr;
extern esphome::globals::GlobalsComponent<bool>   *touch_moved_ptr;
extern esphome::globals::GlobalsComponent<float>  *touch_prev_angle_ptr;
extern esphome::globals::GlobalsComponent<float>  *center_swipe_accum_ptr;
extern esphome::globals::GlobalsComponent<int>    *touch_prev_y_ptr;
extern esphome::globals::GlobalsComponent<bool>   *force_clock_ptr;

#define display_vol        (*display_vol_ptr)
#define was_receiver_on    (*was_receiver_on_ptr)
#define pending_volume     (*pending_volume_ptr)
#define touch_zone         (*touch_zone_ptr)
#define touch_last_y       (*touch_last_y_ptr)
#define touch_start_x      (*touch_start_x_ptr)
#define touch_start_y      (*touch_start_y_ptr)
#define touch_moved        (*touch_moved_ptr)
#define touch_prev_angle   (*touch_prev_angle_ptr)
#define center_swipe_accum (*center_swipe_accum_ptr)
#define touch_prev_y       (*touch_prev_y_ptr)
#define force_clock        (*force_clock_ptr)

// ── Sensors (from YAML sensor:/text_sensor: sections) ────────────────────────
extern esphome::text_sensor::TextSensor *receiver_state_ptr;
extern esphome::text_sensor::TextSensor *receiver_muted_ptr;
extern esphome::sensor::Sensor          *receiver_volume_ptr;

#define receiver_state  (*receiver_state_ptr)
#define receiver_muted  (*receiver_muted_ptr)
#define receiver_volume (*receiver_volume_ptr)

// ── Scripts (from YAML script: section) ──────────────────────────────────────
extern esphome::script::Script<> *send_volume_ptr;
extern esphome::script::Script<> *toggle_mute_ptr;

#define send_volume  (*send_volume_ptr)
#define toggle_mute  (*toggle_mute_ptr)

// ── Time (from YAML time: section) ───────────────────────────────────────────
extern esphome::time::RealTimeClock *ha_time_ptr;
#define ha_time (*ha_time_ptr)

// ── Fonts (from YAML font: section) ──────────────────────────────────────────
// font::Font inherits from display::BaseFont; passed as raw pointers to display printf.
extern esphome::font::Font *font_vol;
extern esphome::font::Font *font_label;
extern esphome::font::Font *font_date;
