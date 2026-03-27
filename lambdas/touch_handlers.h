#pragma once
#include "esphome/components/touchscreen/touchscreen.h"

namespace touch_handlers {

using esphome::touchscreen::TouchPoint;
using esphome::touchscreen::TouchPoints_t;

// ── on_touch ─────────────────────────────────────────────────────────────────
// Called once when a finger first makes contact.
inline void on_touch(TouchPoint touch, const TouchPoints_t &) {
  // rotation:90: display_x = touch.y, display_y = 466 - touch.x (IC y-axis inverted)
  int tx = (int)touch.y;
  int ty = 466 - (int)touch.x;
  const int cx = 233, cy = 233;
  int dx = tx - cx;
  int dy_r = ty - cy;
  float r = sqrtf((float)(dx * dx + dy_r * dy_r));

  id(touch_start_x) = tx;
  id(touch_start_y) = ty;
  id(touch_last_y)  = ty;
  id(touch_prev_y)  = ty;
  id(touch_moved)   = false;
  id(center_swipe_accum) = 0.0f;

  // Bottom-edge upward swipe (zone 3): detects face-toggle regardless of AVR state
  if (ty > 380) {
    id(touch_zone) = 3;
    return;
  }

  bool receiver_on = id(receiver_state).has_state() &&
                     id(receiver_state).state != "off" &&
                     id(receiver_state).state != "unavailable" &&
                     id(receiver_state).state != "unknown";
  if (!receiver_on || id(force_clock)) { id(touch_zone) = 0; return; }

  if (r >= 150.0f && r <= 240.0f) {
    // Ring zone — record starting angle for swipe tracking
    id(touch_zone) = 1;
    float ang = atan2f((float)dy_r, (float)dx) * 180.0f / 3.14159265f;
    if (ang < 0.0f) ang += 360.0f;
    id(touch_prev_angle) = ang;
  } else if (r < 150.0f) {
    id(touch_zone) = 2;  // center zone
  } else {
    id(touch_zone) = 0;  // outside — ignore
  }
}

// ── on_update ─────────────────────────────────────────────────────────────────
// Called every update_interval while a finger is held down.
inline void on_update(const TouchPoints_t &touches) {
  if (touches.empty()) return;
  auto &tp = touches[0];
  if (id(touch_zone) == 0) return;

  // rotation:90: display_x = tp.y, display_y = 466 - tp.x
  int tx = (int)tp.y;
  int ty = 466 - (int)tp.x;

  // Always track position and moved-flag, even during release frame
  id(touch_last_y) = ty;
  int mdx = tx - id(touch_start_x);
  int mdy = ty - id(touch_start_y);
  if (abs(mdx) > 8 || abs(mdy) > 8) id(touch_moved) = true;

  if (tp.state & 0x04) return;  // finger releasing: position tracked, skip gesture updates

  if (id(touch_zone) == 3) {
    // Trigger immediately once finger moves 20px upward — don't wait for release
    if (id(touch_start_y) - ty > 20) {
      bool receiver_on = id(receiver_state).has_state() &&
                         id(receiver_state).state != "off" &&
                         id(receiver_state).state != "unavailable" &&
                         id(receiver_state).state != "unknown";
      if (receiver_on) id(force_clock) = !id(force_clock);
      id(touch_zone) = 0;  // consumed, prevent on_release re-triggering
    }
    return;
  }

  if (id(touch_zone) == 1) {
    // ── Ring swipe: angular delta → volume delta ──────────────
    const int cx = 233, cy = 233;
    float ang = atan2f((float)(ty - cy),
                       (float)(tx - cx)) * 180.0f / 3.14159265f;
    if (ang < 0.0f) ang += 360.0f;

    float prev  = id(touch_prev_angle);
    float delta = ang - prev;
    if (delta >  180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;

    // 270° sweep = 100% volume
    float vol_delta = delta / 270.0f;
    ESP_LOGD("vol", "ring swipe: delta=%.2f vol_delta=%.4f has_state=%d",
             delta, vol_delta, id(receiver_volume).has_state());
    if (fabsf(vol_delta) > 0.001f && id(receiver_volume).has_state()) {
      float new_vol = id(receiver_volume).state + vol_delta;
      if (new_vol < 0.0f) new_vol = 0.0f;
      if (new_vol > 1.0f) new_vol = 1.0f;
      id(pending_volume) = new_vol;
      ESP_LOGD("vol", "sending volume %.3f", new_vol);
      id(send_volume).execute();
    }
    id(touch_prev_angle) = ang;

  } else if (id(touch_zone) == 2) {
    // ── Center swipe: vertical drag, 1% per 20 px ─────────────
    float dy = (float)id(touch_prev_y) - (float)ty;  // positive = swipe up
    id(touch_prev_y) = ty;
    id(center_swipe_accum) += dy;
    int ticks = (int)(id(center_swipe_accum) / 20.0f);
    ESP_LOGD("vol", "center swipe: dy=%.0f accum=%.1f ticks=%d", dy, id(center_swipe_accum), ticks);
    if (ticks != 0 && id(receiver_volume).has_state()) {
      id(center_swipe_accum) -= ticks * 20.0f;
      float new_vol = id(receiver_volume).state + ticks * 0.01f;
      if (new_vol < 0.0f) new_vol = 0.0f;
      if (new_vol > 1.0f) new_vol = 1.0f;
      id(pending_volume) = new_vol;
      ESP_LOGD("vol", "center execute send_volume: %.4f", new_vol);
      id(send_volume).execute();
    }
  }
}

// ── on_release ────────────────────────────────────────────────────────────────
// Called when all fingers lift off.
inline void on_release() {
  ESP_LOGD("vol", "on_release: zone=%d moved=%d", id(touch_zone), id(touch_moved));

  if (id(touch_zone) == 3) {
    // Fallback for fast swipes where on_update didn't fire with updated position
    int dy_swipe = id(touch_start_y) - id(touch_last_y);  // positive = swiped up
    if (dy_swipe > 20) {
      bool receiver_on = id(receiver_state).has_state() &&
                         id(receiver_state).state != "off" &&
                         id(receiver_state).state != "unavailable" &&
                         id(receiver_state).state != "unknown";
      if (receiver_on)
        id(force_clock) = !id(force_clock);
    }
  } else if (id(touch_zone) == 2 && !id(touch_moved)) {
    // Center tap → toggle mute
    id(toggle_mute).execute();
  } else if (id(touch_zone) == 1 && !id(touch_moved)) {
    // Ring tap → jump volume to tapped angular position
    const float START = 135.0f, SWEEP = 270.0f;
    const int cx = 233, cy = 233;
    float ang = atan2f((float)(id(touch_start_y) - cy),
                       (float)(id(touch_start_x) - cx)) * 180.0f / 3.14159265f;
    if (ang < 0.0f) ang += 360.0f;
    float dist = fmodf(ang - START + 360.0f, 360.0f);
    ESP_LOGD("vol", "tap: ang=%.1f dist=%.1f -> vol=%.3f has_state=%d",
             ang, dist, dist/SWEEP, (int)id(receiver_volume).has_state());
    if (dist < SWEEP && id(receiver_volume).has_state()) {
      id(pending_volume) = dist / SWEEP;
      ESP_LOGD("vol", "tap execute send_volume: %.4f", id(pending_volume));
      id(send_volume).execute();
    }
  }
  id(touch_zone) = 0;
}

}  // namespace touch_handlers
