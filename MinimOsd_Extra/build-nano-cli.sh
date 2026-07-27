#!/bin/sh
# Build the ATmega328 (Arduino Nano / MinimOSD) MAVLink firmware with arduino-cli
# instead of the classic Arduino.mk Makefile. Verified with arduino-cli 1.x and
# the arduino:avr core (avr-gcc 7.3). Produces build-nano/MinimOsd_Extra.ino.hex.
#
# The size flags mirror the project Makefile (-mcall-prologues, --relax, fast
# single-precision math, -fwrapv, -fno-strict-aliasing - the last two are relied
# on by the code, do not drop them). With MAVLink2 the result is ~30.7KB, right
# at the 30720-byte limit, so keep an eye on the size report.
#
# Usage: ./build-nano-cli.sh   (from MinimOsd_Extra/, needs arduino-cli in PATH)

set -e

SRC="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$SRC")"
OUT="$SRC/build-nano"

SZ="-mcall-prologues -mrelax -ffast-math -fsingle-precision-constant \
-fassociative-math -freciprocal-math -fno-signed-zeros -fno-trapping-math \
-fmerge-all-constants -finline-functions-called-once -finline-small-functions \
-fno-caller-saves -funsigned-bitfields -fwrapv -fno-strict-aliasing"

arduino-cli compile \
  --fqbn arduino:avr:nano \
  --libraries "$ROOT/libraries" \
  --build-path "$OUT" \
  --build-property "compiler.cpp.extra_flags=-DUSE_MAVLINK=1 -I$ROOT/libraries/GCS_MAVLink $SZ" \
  --build-property "compiler.c.extra_flags=-DUSE_MAVLINK=1 -I$ROOT/libraries/GCS_MAVLink $SZ" \
  --build-property "compiler.c.elf.extra_flags=$SZ -Wl,--relax" \
  "$SRC"

echo
echo "hex: $OUT/MinimOsd_Extra.ino.hex"
echo "flash (Nano old bootloader): avrdude -p atmega328p -c arduino -P /dev/ttyUSB0 -b 57600 -D -U flash:w:$OUT/MinimOsd_Extra.ino.hex:i"
echo "flash (Nano new bootloader): avrdude -p atmega328p -c arduino -P /dev/ttyUSB0 -b 115200 -D -U flash:w:$OUT/MinimOsd_Extra.ino.hex:i"
