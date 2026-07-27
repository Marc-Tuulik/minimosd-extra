# Minimal MAX7456 charset updater

The stock Character_Updater builds in this repo (Released/FW_+_Char hexes and
the checked-in build-atmega328 ELF) did not respond on test hardware, so this
is a from-scratch replacement using the stock Arduino core.

Symptom of a wrong/stale charset: artificial-horizon interior renders as
letter salad (KKK/MNT/HHH...), ILS bars as hexagon glyphs, panel icons as
random letters. This fork REQUIRES its own MinimOSD_2.4.1.x.mcm charset.

Usage (board reachable via serial with DTR reset, e.g. FTDI):
1. arduino-cli compile --fqbn arduino:avr:nano font_updater/  (2.2KB)
2. avrdude -p atmega328p -c arduino -P /dev/ttyUSBx -b 115200 -D -U flash:w:font_updater.ino.hex:i
3. python3 font_send.py            # streams ../../Released/MinimOSD_2.4.1.6.mcm, ~8s
4. reflash the normal MinimOsd_Extra firmware (EEPROM + font NVM both survive)

Protocol at 57600 baud: 'F'->'H' hello (display off); per char 'C',idx,54
bitmap bytes,xor-chk -> 'K' ok / 'E' bad (host retries); 'D'->"BY" done
(display re-enabled).
