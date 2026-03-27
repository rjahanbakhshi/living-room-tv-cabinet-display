# Livingroom TV Cabinet Display

An ESPHome firmware for a round 466×466 AMOLED touchscreen mounted in the living room TV cabinet. It shows a volume ring for the Yamaha receiver and an analog clock, with touch controls for volume and mute.

## Hardware

| Component | Detail |
|-----------|--------|
| MCU | ESP32-S3 (16 MB flash, octal PSRAM) |
| Display | 1.43" round AMOLED, CO5300 panel, 466×466 |
| Display bus | Quad SPI (QSPI) — GPIO 9–14 |
| Touch IC | FT3168 at I2C address 0x38 — GPIO 47 (SDA) / 48 (SCL) |
| Touch power | GPIO 42, driven HIGH at boot |

## Features

### Volume face
Shown when the Yamaha receiver (`media_player.tx_rz30`) is on.

- **Radial bar ring**: 45 bars sweeping 270°, yellow→red gradient
- **Ring swipe**: rotate finger on the ring to adjust volume
- **Center swipe**: drag up/down in the inner circle to nudge volume ±1% per 20 px
- **Center tap**: toggle mute
- **Ring tap**: jump volume to the tapped position
- **Bottom-edge upward swipe**: force-switch to clock face

### Clock face
Shown when the receiver is off, or after a bottom-edge swipe toggle.

- Smooth analog hands (hour, minute, second)
- Hour markers and minute tick dots
- Date line at the bottom

## Prerequisites

```bash
pip install esphome
```

Create a `secrets.yaml` in the project root (not committed) with your Wi-Fi credentials if you move them there, or keep them directly in the YAML.

## Build & Flash

### Compile only
```bash
esphome compile livingroom-tv-cabinet-display.yaml
```

### Compile and flash over USB
```bash
esphome run livingroom-tv-cabinet-display.yaml
```

### Flash OTA (device already on network)
```bash
esphome run --device <ip-address> livingroom-tv-cabinet-display.yaml
```

### Stream logs
```bash
esphome logs livingroom-tv-cabinet-display.yaml
# or with explicit device
esphome logs --device <ip-address> livingroom-tv-cabinet-display.yaml
```

### Validate config without compiling
```bash
esphome config livingroom-tv-cabinet-display.yaml
```

## Custom Component

`components/ft5x06/` is a local override of the ESPHome built-in FT5x06 touchscreen driver. The stock driver doesn't recognise the FT3168 variant (vendor ID `0x06`) used on this board. The override adds that vendor ID and extends the post-boot initialisation delay to 1200 ms, which the FT3168 requires for its power rail to stabilise.

## Home Assistant Integration

The device connects to Home Assistant via the native API. It reads:

- `media_player.tx_rz30` — receiver on/off state
- `media_player.tx_rz30` → `volume_level` — current volume (0.0–1.0)
- `media_player.tx_rz30` → `is_volume_muted` — mute state

Volume changes and mute toggles are sent back via `media_player.volume_set` and `media_player.volume_mute` HA services.
