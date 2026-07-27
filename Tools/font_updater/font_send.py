#!/usr/bin/env python3
"""Host side for the minimal font updater: stream .mcm charset with acks."""
import os, termios, time, sys

fd = os.open('/dev/ttyUSB0', os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
a = termios.tcgetattr(fd)
a[0]=0; a[1]=0; a[2]=termios.CS8|termios.CREAD|termios.CLOCAL; a[3]=0
a[4]=termios.B57600; a[5]=termios.B57600
cc=list(a[6]); cc[termios.VMIN]=0; cc[termios.VTIME]=0; a[6]=cc
termios.tcsetattr(fd, termios.TCSANOW, a)

def read_for(dur):
    buf = bytearray(); t = time.time() + dur
    while time.time() < t:
        try:
            c = os.read(fd, 64)
            if c: buf.extend(c)
        except BlockingIOError: pass
        time.sleep(0.003)
    return bytes(buf)

lines = open('/tmp/MinimOSD_2.4.1.6.mcm').read().splitlines()
chars = []
for i in range(256):
    blk = lines[1+i*64 : 1+i*64+54]           # first 54 of 64 lines are stored
    chars.append(bytes(int(l, 2) for l in blk))

print('waiting for boot...'); time.sleep(2.5)
termios.tcflush(fd, termios.TCIOFLUSH)

ok = False
for _ in range(10):
    os.write(fd, b'F')
    if b'H' in read_for(0.3): ok = True; break
if not ok: print('no hello'); sys.exit(1)
print('hello - uploading...')

t0 = time.time()
for idx in range(256):
    data = chars[idx]
    chk = idx
    for b in data: chk ^= b
    msg = b'C' + bytes([idx]) + data + bytes([chk])
    done = False
    for attempt in range(4):
        os.write(fd, msg)
        r = b''
        t = time.time() + 1.5
        while time.time() < t:
            r += read_for(0.01)
            if b'K' in r or b'E' in r: break
        if b'K' in r: done = True; break
        termios.tcflush(fd, termios.TCIFLUSH)
        print(f'char {idx}: retry {attempt+1} ({r!r})')
    if not done: print(f'char {idx} FAILED'); sys.exit(2)
    if idx % 32 == 0: print(f'  {idx}/256 ({time.time()-t0:.0f}s)')

os.write(fd, b'D')
print('done in %.0fs, reply: %s' % (time.time()-t0, read_for(0.5)))
os.close(fd)
