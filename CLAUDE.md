# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESPHome firmware project for a round AMOLED touchscreen display (466×466, CO5300 panel) mounted in a living room TV cabinet. The device runs on an ESP32-S3 and serves as a smart home display that shows:
- **Volume face**: A radial bar volume ring for a Yamaha receiver (`media_player.tx_rz30`) when it's on
- **Clock face**: An analog clock when the receiver is off (or user force-toggled to it)

The display integrates with Home Assistant via the native API.

## Key Commands

### Validate / compile firmware (no device needed)
```bash
esphome compile livingroom-tv-cabinet-display.yaml
```

### Flash to device over USB
```bash
esphome upload livingroom-tv-cabinet-display.yaml
```

### Flash OTA (device must be online)
```bash
esphome upload --device <ip-address> livingroom-tv-cabinet-display.yaml
```

### Stream device logs
```bash
esphome logs livingroom-tv-cabinet-display.yaml
```

### Full validate (config check only, no compile)
```bash
esphome config livingroom-tv-cabinet-display.yaml
```

## Architecture

### Single-file config
All firmware logic lives in `livingroom-tv-cabinet-display.yaml`. There is no separate C++ application code — display rendering, touch handling, and HA integration are all inline `lambda:` blocks within the YAML.

### Display rendering (`display` → `lambda`)
Runs every 100ms. Decides between two faces based on `receiver_state` and `force_clock`:
- **Volume face**: 45 radial bars (every 8°) sweeping 270° with a yellow→red gradient. The `display_vol` global is eased toward the real volume (35% per frame) for smooth animation.
- **Clock face**: Analog clock with hour/minute/second hands, hour markers, minute ticks, and a date line.

### Touch zones (touchscreen `on_touch` / `on_update` / `on_release`)
The FT3168 touch IC reports raw coordinates. The display is rotated 90°, so coordinates are remapped: `display_x = touch.y`, `display_y = 466 - touch.x`.

Three interaction zones relative to center (233, 233):
- **Zone 1 (ring)**: radius 150–240px — circular swipe adjusts volume (270° = 100%), tap jumps to angle position
- **Zone 2 (center)**: radius < 150px — swipe adjusts volume (1% per 20px), tap toggles mute
- **Zone 3 (bottom edge)**: `ty > 380` — upward swipe ≥20px toggles between clock and volume faces (`force_clock`)

### Custom FT5x06 component (`components/ft5x06/`)
A local override of the ESPHome built-in ft5x06 touchscreen driver. Required because the board uses an FT3168 variant (vendor ID `0x06`) that the upstream driver doesn't recognize. The component:
- Adds vendor ID `FT5X06_ID_4 = 0x06` to the accepted list
- Waits 1200ms after boot before initialising (FT3168 needs >1000ms power-rail stabilisation)
- GPIO42 is driven HIGH at boot priority 800 to power the touch IC before component setup runs

### Home Assistant integration
Sensors polled from HA:
- `media_player.tx_rz30` state → `receiver_state` (text sensor)
- `media_player.tx_rz30` `volume_level` attribute → `receiver_volume` (sensor)
- `media_player.tx_rz30` `is_volume_muted` attribute → `receiver_muted` (text sensor)

Volume changes are sent back via `homeassistant.service` calls (`media_player.volume_set`, `media_player.volume_mute`).

### Hardware notes
- **Display bus**: Quad SPI (QSPI) on GPIO 9–14
- **Touch IC**: FT3168 at I2C address 0x38, I2C bus on GPIO47 (SDA) / GPIO48 (SCL)
- **Touch IC power**: GPIO42, driven HIGH at boot
- **PSRAM**: Octal mode, 80MHz (required for AMOLED framebuffer)
