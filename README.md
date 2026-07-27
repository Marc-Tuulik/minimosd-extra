# minimosd-extra — MAVLink 2 fork

Fork of [night-ghost/minimosd-extra](https://github.com/night-ghost/minimosd-extra)
(MinimOSD-Extra firmware for MinimOSD / micro MinimOSD boards) with MAVLink 2
support, metric speeds in m/s, and fixes that make the project build on modern
toolchains.

## Changes in this fork

### MAVLink 2 support
* Telemetry input parses **MAVLink 2** frames (`0xFD`) and remains backward
  compatible with MAVLink 1 (`0xFE`) — the parser auto-detects per frame.
  Telemetry output (stream requests, config replies) is MAVLink 2.
* Verified on real hardware (ATmega328P): v2 heartbeats in, CRC-valid v2
  `REQUEST_DATA_STREAM` out.
* Fixed several latent bugs in the bundled v2.0 header set that had never been
  used on AVR before:
  * missing `uAvionix` dialect header (upstream shipped the v2.0 tree incomplete)
  * two direct PROGMEM reads in the parser that would corrupt CRC validation on AVR
  * NULL `r_message` dereference on every received byte (this firmware parses
    in place; the v1.0 headers were patched for that long ago, v2.0 was not)
  * config-read replies (`mavlink_return_packet`) carried broken CRCs — the v2
    message struct stores msgid as uint32 vs 3 bytes on the wire, so header and
    payload must be CRC'd separately

### PX4 support (MAVLINKPX4 build)
Verified end-to-end against a real PX4 fixed-wing (mode `MAV_x_MODE = OSD`
on the telemetry port; the firmware autobauds, 115200 recommended):
* AUTO sub-mode names fixed (off-by-one: HOLD showed as "miss", MISSION as
  "rtl") - mode labels now match QGroundControl.
* NaN telemetry (attitude / climb / airspeed before EKF alignment) sanitized
  instead of printing garbage; VFR_HUD heading clamped to 0..360.
* The deprecated `REQUEST_DATA_STREAM` re-request cycle is compiled out - on
  a GPS-less bench it blocked the main loop ~450ms every heartbeat (hard
  periodic screen freeze). PX4 stream rates come from the port profile.
* Redraws are frame-locked to every 2nd VSYNC (25Hz PAL / 30Hz NTSC), fixing
  serial-overflow stutter at PX4 stream rates.
* Build with `./build-nano-cli.sh MAVLINKPX4`. Handy to know: PX4 sends
  battery voltage `UINT16_MAX` mV ("unknown") when unpowered from USB - the
  OSD then shows 65.54V. The board LED doubles as a loop-alive heartbeat
  (steady 2Hz blink = healthy; pauses = main loop blocked).

### Metric speed in m/s
* All metric speed panels (ground speed, air speed, wind speed, max speeds,
  setup screen) now read **m/s** instead of km/h. The per-panel "alternate
  units" toggle shows km/h.
* Note: metric stall / overspeed warning thresholds are now interpreted in m/s —
  re-enter them accordingly. The bundled configurator still *labels* speeds as
  km/h (cosmetic only), and its per-panel "alternate units" checkbox on speed
  panels now means km/h (it used to mean m/s) — leave it unchecked for m/s.
  When placing panels, don't butt "Real heading" (COG) against "Heading": COG
  is 7 chars wide (arrows + value + degree sign) and adjacent panels merge
  into one unreadable number on screen.

### Fits on ATmega328 (30.7KB limit)
MAVLink 2 initially cost ~6.4KB over the 328's flash. Reclaimed without
dropping features:
* MAVLink 2 packet signing (SHA-256) compiled out via `MAVLINK_NO_SIGNING` —
  this OSD never configures signing keys (~3.7KB)
* message CRC table trimmed from 190 dialect entries to the 26 messages the
  firmware actually receives (~1.5KB)
* build with the size flags from the project Makefile (see
  `MinimOsd_Extra/build-nano-cli.sh`) (~1.2KB)

Current MAVLINK/328 build: **30676 / 30720 bytes**. There is almost no headroom
left — check the size report after any change.

### Builds on modern avr-gcc
* `params3` (sensor setup screen) used integer-to-pointer casts in a PROGMEM
  initializer that avr-gcc ≥ 7 rejects; rewritten as offset arithmetic.
  The project now compiles with the stock Arduino AVR core / arduino-cli —
  no vintage gcc 5.x required.

### New tools
* `MinimOsd_Extra/build-nano-cli.sh` — build the ATmega328 MAVLink firmware
  with arduino-cli (no Arduino.mk needed); prints avrdude flash commands.
* `Tools/font_updater/` — minimal MAX7456 charset updater (2.2KB sketch +
  host script). The stock `Character_Updater` builds in `Released/` did not
  respond on test hardware; this replacement burns all 256 characters in ~8s
  with a checksummed, acked protocol. A wrong/stale charset shows as letter
  salad in the artificial horizon and random letters instead of panel icons —
  this firmware **requires** its `MinimOSD_2.4.1.x.mcm` charset.
* `Tools/mavlink_config/` — configure a live OSD over serial MAVLink, no
  configurator and no reflash needed: `build_eeprom.py` converts a `.osd`
  layout file into the raw EEPROM image (panels + settings), `osd_config.py`
  writes and verifies it through the firmware's `ENCAPSULATED_DATA` config
  channel and soft-reboots the board. Also documents the EEPROM/point
  encoding (screen offsets are stored raw for the MAX7456 HOS/VOS registers —
  neutral is `0x20`/`0x10`; PAL/NTSC autodetect requires the `mode_auto`
  settings flag).

## Bringing up a board

Everything can be done over the board's serial header (FTDI with DTR reset):

1. **Charset** (once per MAX7456): flash `Tools/font_updater/`, run
   `font_send.py`.
2. **Firmware**: `MinimOsd_Extra/build-nano-cli.sh`, flash the hex with
   avrdude (`-c arduino -b 115200` for Optiboot, `-b 57600` for old
   bootloaders).
3. **Configuration**: either the classic `OSD_Config.exe` from
   `Released/FW_+_Char`, or headless via `Tools/mavlink_config/`.
4. A board with a blank EEPROM shows a version-check error / unconfigured
   layout — step 3 fixes that. The `No input data! <n>` screen with the
   detected baud rate is the normal idle state without telemetry.

## Upstream description

The original project (see upstream README for full history): refactored
MinimOSD firmware using less RAM/EEPROM, MAX7456 refresh in VSYNC interrupt
(no "snow"), 4 screens, per-screen icon control, external voltage/current/RSSI
inputs, TLOG player in the configurator, radar + ILS in the horizon, dynamic
PAL/NTSC detection, screen offsets from the configurator, RC-channel-to-pin
output, setup screen adjustable from the RC transmitter, and more. See
[CHANGELOG.md](CHANGELOG.md).

Fonts: `MinimOSD_2.4.1.x.mcm` (base), `MinimOSD_2.4.1.x-digital.mcm`
(7-segment style).

**Attention!** This version is incompatible with the tools from ArduCam and
the original MinimOSD-Extra.

## License

GNU GPL v2, as upstream.
