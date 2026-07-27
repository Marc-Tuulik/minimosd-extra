#!/usr/bin/env python3
"""Build a MinimOSD-Extra EEPROM image (5 x 128-byte blocks: 4 panel screens +
settings) from a configurator .osd layout file, for writing over MAVLink with
osd_config.py.

Point encoding (see Func.h / eeprom.h):
  x: bits0-5 = column, bit7 = sign icon HIDDEN, bit6 = alt4
  y: bits0-3 = row,    bit7 = panel OFF, bit6/5/4 = alt/alt2/alt3
Settings live at EEPROM 512..639; offsets computed from struct Settings
(pack(1)).  horiz_offs/vert_offs are written RAW to the MAX7456 HOS/VOS
registers, so neutral is 0x20/0x10 - zero means 32px left / 16 lines up.
PAL/NTSC autodetect requires flags.mode_auto (flags byte1 bit1).

Usage: python3 build_eeprom.py [layout.osd] [out.bin]
       (defaults: ../../Released/default.osd -> osd_eeprom_640.bin)
"""
import os, struct, sys

# panel name in .osd -> Point index (offsetof(Panel, field) / sizeof(Point))
IDX = {
 'Pitch':1,'Roll':2,'Battery A':3,'Battery B':4,'Visible Sats':5,'Real heading':6,
 'GPS Coord':7,'Heading Rose':8,'Heading':9,'Home Direction':10,'Home Distance':11,
 'WP Direction':12,'WP Distance':13,'RSSI':14,'Current':15,'Altitude':17,'Velocity':18,
 'Throttle':19,'Flight Mode':20,'Horizon':21,'Home Altitude':22,'Air Speed':23,
 'Battery Percent':24,'Time':25,'Warnings':26,'Wind Speed':27,'Vertical Speed':28,
 'Tune':29,'Efficiency':30,'Call Sign':31,'Channel Raw':32,'Temperature':33,
 'Trip Distance':34,'Radar Scale':36,'Flight Data':37,'Message':38,
 'Sensor 1':39,'Sensor 2':40,'Sensor 3':41,'Sensor 4':42,'GPS HDOP':43,
 'Channel state':44,'Channel Scale':45,'Channel Value':47,
}

def encode(x, y, vis, sign, alt, alt2, alt3, alt4):
    bx = (x & 0x3F)
    if sign == 0: bx |= 0x80
    if alt4 == 1: bx |= 0x40
    by = (y & 0x0F)
    if not vis:   by |= 0x80
    if alt  == 1: by |= 0x40
    if alt2 == 1: by |= 0x20
    if alt3 == 1: by |= 0x10
    return bx, by

def build(osd_path):
    screens, cur = {}, None
    for line in open(osd_path, encoding='ascii', errors='replace'):
        line = line.rstrip('\r\n')
        if line.startswith('Configuration'): break
        if line.startswith('Panel '):
            cur = int(line.split()[1]); screens[cur] = bytearray(128); continue
        if cur is None or not line.strip(): continue
        f = line.split('\t')
        if f[0] not in IDX: continue
        bx, by = encode(int(f[1]), int(f[2]), f[3] == 'True',
                        *[int(v) for v in f[4:9]])
        off = IDX[f[0]] * 2
        screens[cur][off], screens[cur][off+1] = bx, by

    # panels the .osd doesn't know about (newer firmware) -> OFF, not (0,0)-ON
    known = {IDX[n] * 2 for n in IDX}
    for blk in screens.values():
        for pid in range(1, 55):
            off = pid * 2
            if off < 128 and off not in known: blk[off+1] |= 0x80

    s = bytearray(128)
    s[0:4] = bytes([0x00, 0x02, 0x00, 0x00])  # flags: mode_auto (PAL/NTSC autodetect)
    s[4]  = 1                                  # model_type: copter
    s[11] = 23                                 # timeOffset: bias 20 + UTC offset (+3)
    s[15] = 20                                 # batt_warn_level %
    s[26] = 79                                 # CHK1_VERSION (= VER in Config.h)
    s[27] = 79 ^ 0x55                          # CHK2_VERSION
    one = struct.pack('<f', 1.0)               # horizon coefficients PAL+NTSC
    s[50:54] = one; s[54:58] = one; s[58:62] = one; s[62:66] = one
    s[67] = 0x10                               # vert_offs  (neutral)
    s[68] = 0x20                               # horiz_offs (neutral)
    s[71] = 1                                  # n_screens

    img = bytearray()
    for n in range(4):
        img += screens.get(n, bytearray([0, 0x80]) * 64)
    img += s
    return bytes(img)

if __name__ == '__main__':
    here = os.path.dirname(os.path.abspath(__file__))
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, '..', '..', 'Released', 'default.osd')
    dst = sys.argv[2] if len(sys.argv) > 2 else 'osd_eeprom_640.bin'
    img = build(src)
    open(dst, 'wb').write(img)
    print('wrote %s (%d bytes) from %s' % (dst, len(img), src))
