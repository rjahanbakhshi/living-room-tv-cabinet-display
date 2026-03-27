#pragma once
#include "esphome/components/display/display.h"

namespace display_face {

inline void render(esphome::display::Display &it) {
  const int cx = 233, cy = 233;
  auto deg2rad = [](float d) { return d * 3.14159265f / 180.0f; };

  // ── Determine state ───────────────────────────────────────────
  bool receiver_on = id(receiver_state).has_state() &&
                     id(receiver_state).state != "off" &&
                     id(receiver_state).state != "unavailable" &&
                     id(receiver_state).state != "unknown";
  bool show_volume = receiver_on && !id(force_clock);
  bool is_muted    = id(receiver_muted).has_state() &&
                     (id(receiver_muted).state == "True" || id(receiver_muted).state == "true" ||
                      id(receiver_muted).state == "on"   || id(receiver_muted).state == "1");

  // Reset animation when receiver first powers on
  if (!id(was_receiver_on) && receiver_on)
    id(display_vol) = 0.0f;
  id(was_receiver_on) = receiver_on;

  // Actual target volume
  float target_vol = 0.0f;
  if (receiver_on && id(receiver_volume).has_state() && !isnan(id(receiver_volume).state)) {
    target_vol = id(receiver_volume).state;
    target_vol = target_vol < 0.0f ? 0.0f : (target_vol > 1.0f ? 1.0f : target_vol);
  }

  // Ease display_vol toward target (35% per frame → ~99% in 12 frames)
  float diff = target_vol - id(display_vol);
  if (fabsf(diff) > 0.004f)
    id(display_vol) += diff * 0.35f;
  else
    id(display_vol) = target_vol;
  float vol = id(display_vol);

  // True black background (AMOLED: black = pixels off)
  it.fill(Color(0, 0, 0));

  if (show_volume) {
    // ── RADIAL BAR VOLUME RING ────────────────────────────────
    // 45 bars every 8°, active zone sweeps 270° from 7:30 o'clock
    const float START = 135.0f;
    const float SWEEP = 270.0f;
    const int   BARS  = 45;

    // Draw one radial bar. hw: 0=1px, 1=3px (±1.5), 2=5px (+±3.0 half), 3=7px (+±4.5 quarter)
    auto draw_bar = [&](float deg, int r_in, int r_out, Color col, int hw) {
      float a = deg2rad(deg);
      float ca = cosf(a), sa = sinf(a);
      auto dl = [&](float off, Color c) {
        float px = -sa * off, py = ca * off;
        it.line(cx+(int)(r_in*ca+px), cy+(int)(r_in*sa+py),
                cx+(int)(r_out*ca+px), cy+(int)(r_out*sa+py), c);
      };
      dl(0, col);
      if (hw >= 1) { dl( 1.5f, col); dl(-1.5f, col); }
      if (hw >= 2) {
        Color gc(col.r >> 1, col.g >> 1, (uint8_t)0);
        dl( 3.0f, gc); dl(-3.0f, gc);
      }
      if (hw >= 3) {
        Color gc2(col.r >> 2, col.g >> 2, (uint8_t)0);
        dl( 4.5f, gc2); dl(-4.5f, gc2);
      }
    };

    for (int i = 0; i < BARS; i++) {
      float deg  = 360.0f * i / BARS;
      // Angular distance clockwise from active-zone start
      float dist = fmodf(deg - START + 360.0f, 360.0f);
      bool in_gap = dist >= SWEEP;               // 90° gap at bottom
      bool lit    = !in_gap && dist < vol * SWEEP;

      if (lit) {
        // Yellow → red gradient: full red at 60% volume (frac=0.6)
        float frac = dist / SWEEP;
        uint8_t g  = (frac < 0.6f) ? (uint8_t)(255.0f * (1.0f - frac / 0.6f)) : 0;
        draw_bar(deg, 175, 220, Color(255, g, 0), 3);
      } else if (!in_gap) {
        // Inactive portion: same outer radius as active, dim amber, 3px thick
        draw_bar(deg, 190, 218, Color(0x50, 0x3e, 0x00), 1);
      } else {
        // Gap zone: very short, barely visible
        draw_bar(deg, 210, 214, Color(0x28, 0x20, 0x00), 0);
      }
    }

    // Separation ring (3px thick) then black fill for clean text area
    it.circle(cx, cy, 173, Color(0x44, 0x36, 0x00));
    it.circle(cx, cy, 172, Color(0x66, 0x50, 0x00));
    it.circle(cx, cy, 171, Color(0x44, 0x36, 0x00));
    it.filled_circle(cx, cy, 170, Color(0, 0, 0));

    if (is_muted) {
      // Dimmed volume number with a bold red diagonal slash
      it.printf(cx, cy - 10, font_vol, Color(0x33, 0x33, 0x33), TextAlign::CENTER,
                "%d", (int)(target_vol * 100.0f + 0.5f));
      // Three parallel lines for a thick slash
      for (int w = -2; w <= 2; w++) {
        it.line(cx - 50, cy - 55 + w, cx + 50, cy + 45 + w, Color(0xff, 0x20, 0x00));
      }
      it.printf(cx, cy + 75, font_label, Color(0x99, 0x20, 0x00), TextAlign::CENTER,
                "VOLUME");
    } else {
      // Normal: show volume number and label
      it.printf(cx, cy - 10, font_vol, Color(0xcc, 0xb8, 0x88), TextAlign::CENTER,
                "%d", (int)(target_vol * 100.0f + 0.5f));
      it.printf(cx, cy + 75, font_label, Color(0xaa, 0x80, 0x00), TextAlign::CENTER,
                "VOLUME");
    }

  } else {
    // ── MODERN ANALOG CLOCK ──────────────────────────────────
    // Thin bezel ring
    it.circle(cx, cy, 222, Color(0x14, 0x10, 0x00));
    it.circle(cx, cy, 221, Color(0x1c, 0x16, 0x00));
    it.circle(cx, cy, 220, Color(0x22, 0x1c, 0x00));

    auto now = id(ha_time).now();

    if (!now.is_valid()) {
      // Connecting: bright ring + status text
      it.circle(cx, cy, 222, Color(0xff, 0xb3, 0x00));
      it.circle(cx, cy, 221, Color(0xff, 0xb3, 0x00));
      it.circle(cx, cy, 220, Color(0xff, 0xb3, 0x00));
      it.printf(cx, cy - 15, font_date, Color(0xff, 0xb3, 0x00),
                TextAlign::CENTER, "Connecting...");
      it.printf(cx, cy + 25, font_date, Color(0x55, 0x55, 0x55),
                TextAlign::CENTER, "to Home Assistant");
    } else {
      // Hour markers: larger dot at 12/3/6/9, small elsewhere
      for (int h = 0; h < 12; h++) {
        float ha = deg2rad(-90.0f + h * 30.0f);
        bool major = (h % 3 == 0);
        it.filled_circle(
          cx + (int)(205 * cosf(ha)), cy + (int)(205 * sinf(ha)),
          major ? 6 : 3,
          major ? Color(0x77, 0x60, 0x00) : Color(0x44, 0x38, 0x00)
        );
      }

      // Minute tick marks: small filled dot
      for (int m = 0; m < 60; m++) {
        if (m % 5 != 0) {
          float ma = deg2rad(-90.0f + m * 6.0f);
          it.filled_circle(cx+(int)(213*cosf(ma)), cy+(int)(213*sinf(ma)),
                           1, Color(0x33, 0x2a, 0x00));
        }
      }

      // Smooth hand angles (12 o'clock = -90°)
      float sec_a  = deg2rad(-90.0f + now.second * 6.0f);
      float min_a  = deg2rad(-90.0f + now.minute * 6.0f  + now.second * 0.1f);
      float hour_a = deg2rad(-90.0f + (now.hour % 12) * 30.0f + now.minute * 0.5f);

      // Draw a hand as (2*hw+1) parallel lines for thickness
      auto draw_hand = [&](float a, int r_tail, int r_tip, int hw, Color col) {
        for (int w = -hw; w <= hw; w++) {
          float px = -sinf(a) * w, py = cosf(a) * w;
          it.line(cx+(int)(r_tail*cosf(a)+px), cy+(int)(r_tail*sinf(a)+py),
                  cx+(int)(r_tip *cosf(a)+px), cy+(int)(r_tip *sinf(a)+py), col);
        }
      };

      draw_hand(sec_a,  -28, 188, 1, Color(0x55, 0x00, 0x00)); // second: 3px dim red
      draw_hand(min_a,  -22, 168, 2, Color(0x77, 0x60, 0x00)); // minute: 5px amber
      draw_hand(hour_a, -18, 115, 3, Color(0xb8, 0x86, 0x0b)); // hour:   7px gold

      // Center pivot cap
      it.filled_circle(cx, cy, 8, Color(0xb8, 0x86, 0x0b));
      it.filled_circle(cx, cy, 4, Color(0xff, 0xb3, 0x00));
      it.filled_circle(cx, cy, 2, Color(0xff, 0xff, 0xff));

      // Date (dim, at bottom of face)
      it.printf(cx, cy + 85, font_date, Color(0x33, 0x28, 0x00),
                TextAlign::CENTER, "%s", now.strftime("%b %d").c_str());
    }
  }
}

}  // namespace display_face
