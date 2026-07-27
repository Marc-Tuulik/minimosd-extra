#!/usr/bin/env python3
"""Write MinimOSD-Extra EEPROM config over MAVLink2 (ENCAPSULATED_DATA 0xEE-OSD protocol)."""
import os, termios, time, sys

PORT, BAUD = '/dev/ttyUSB0', termios.B57600
MYSYS = 255

def x25(data):
    crc = 0xFFFF
    for b in data:
        tmp = b ^ (crc & 0xFF)
        tmp = (tmp ^ (tmp << 4)) & 0xFF
        crc = ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF
    return crc

SEQ = 0
def frame(msgid, payload, crc_extra, compid=190):
    global SEQ
    hdr = bytes([len(payload), 0, 0, SEQ & 0xFF, MYSYS, compid,
                 msgid & 0xFF, (msgid >> 8) & 0xFF, (msgid >> 16) & 0xFF])
    SEQ += 1
    crc = x25(hdr + payload + bytes([crc_extra]))
    return bytes([0xFD]) + hdr + payload + bytes([crc & 0xFF, crc >> 8])

def heartbeat(mtype):  # crc_extra 50
    return frame(0, bytes([0,0,0,0, mtype, 8, 0, 4, 3]), 50)

def encap(seqnr, inner):  # ENCAPSULATED_DATA id 131, crc_extra 223, compid MUST be 100 (camera)
    payload = bytes([seqnr & 0xFF, seqnr >> 8]) + inner
    return frame(131, payload, 223, compid=100)

def osd_cmd(seqnr, cmd, blk, ln, data=b''):
    inner = bytes([0xEE]) + b'OSD' + bytes([ord(cmd), blk, ln]) + data.ljust(128, b'\0') + b'\0'
    return encap(seqnr, inner)

fd = os.open(PORT, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
a = termios.tcgetattr(fd)
a[0]=0; a[1]=0; a[2]=termios.CS8|termios.CREAD|termios.CLOCAL; a[3]=0; a[4]=BAUD; a[5]=BAUD
cc=list(a[6]); cc[termios.VMIN]=0; cc[termios.VTIME]=0; a[6]=cc
termios.tcsetattr(fd, termios.TCSANOW, a)

def drain(dur=0.05):
    buf = bytearray(); t = time.time() + dur
    while time.time() < t:
        try:
            c = os.read(fd, 512)
            if c: buf.extend(c)
        except BlockingIOError: pass
        time.sleep(0.005)
    return buf

def find_frames(buf, want_id):
    out, i = [], 0
    while i < len(buf) - 12:
        if buf[i] == 0xFD:
            ln = buf[i+1]; mid = buf[i+7] | (buf[i+8]<<8) | (buf[i+9]<<16)
            end = i + 10 + ln + 2
            if end <= len(buf):
                if mid == want_id: out.append(bytes(buf[i+10:i+10+ln]))
                i = end; continue
        i += 1
    return out

img = open('/tmp/osd_eeprom_640.bin','rb').read()
assert len(img) == 640

# Phase 1: sync (saturate with FC heartbeats until firmware answers with REQUEST_DATA_STREAM)
termios.tcflush(fd, termios.TCIOFLUSH)
print('phase1: sync...'); t0 = time.time(); synced = False
while time.time() - t0 < 25 and not synced:
    os.write(fd, heartbeat(2) + heartbeat(2))
    if find_frames(drain(0.01), 66): synced = True
print('  synced!' if synced else '  NO SYNC'); 
if not synced: sys.exit(1)

# Phase 2: announce as GCS (sets mav_gcs_id=255, resets last_seq)
for _ in range(3): os.write(fd, heartbeat(6)); time.sleep(0.05)
drain(0.2)

# Phase 3: write + verify 5 blocks
seqnr = 1; ok_all = True
for blk in range(5):
    data = img[blk*128:(blk+1)*128]
    done = False
    for attempt in range(4):
        os.write(fd, osd_cmd(seqnr, 'w', blk, 128, data)); seqnr += 1
        time.sleep(0.15); drain(0.1)
        os.write(fd, osd_cmd(seqnr, 'r', blk, 128)); seqnr += 1
        rep = drain(0.6)
        for p in find_frames(rep, 131):
            # payload: seqnr(2) EE 'OSD' cmd id len data...
            if len(p) >= 137 and p[2] == 0xEE and p[3:6] == b'OSD' and p[6] == ord('r') and p[7] == blk:
                if p[9:137] == data: done = True
        if done: break
        print(f'  block {blk}: retry {attempt+1}')
    print(f'block {blk}: {"VERIFIED" if done else "FAILED"}')
    ok_all = ok_all and done
if not ok_all: sys.exit(2)

# Phase 4: reboot command
os.write(fd, osd_cmd(seqnr, 'b', 0, 0)); seqnr += 1
print('reboot sent; waiting...'); time.sleep(4); termios.tcflush(fd, termios.TCIOFLUSH)

# Phase 5: confirm alive after reboot
t0 = time.time(); alive = False
while time.time() - t0 < 25 and not alive:
    os.write(fd, heartbeat(2) + heartbeat(2))
    if find_frames(drain(0.01), 66): alive = True
print('ALIVE after reboot with new config' if alive else 'no response after reboot')
os.close(fd)
