// Minimal MAX7456 charset updater for MinimOSD boards (stock Arduino core).
// Protocol at 57600: host 'F' -> 'H'; per char: 'C',idx,54 bytes,xor-chk -> 'K'/'E';
// host 'D' -> "BY" and display re-enabled.
#include <SPI.h>

const uint8_t CS = 6;   // MAX7456 chip select on MinimOSD (PD6)

static void mwrite(uint8_t r, uint8_t v) {
  digitalWrite(CS, LOW); SPI.transfer(r); SPI.transfer(v); digitalWrite(CS, HIGH);
}
static uint8_t mread(uint8_t r) {
  digitalWrite(CS, LOW); SPI.transfer(r); uint8_t v = SPI.transfer(0xFF); digitalWrite(CS, HIGH); return v;
}

static uint8_t rd() { while (!Serial.available()); return Serial.read(); }

void setup() {
  Serial.begin(57600);
  pinMode(10, OUTPUT); digitalWrite(10, HIGH);   // SPI SS must be output-high
  pinMode(CS, OUTPUT); digitalWrite(CS, HIGH);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);          // 1 MHz, safe
  pinMode(13, OUTPUT);
}

void loop() {
  uint8_t c = rd();
  if (c == 'F') {                 // hello: disable display for NVM access
    mwrite(0x00, 0x00);           // VM0 = 0
    Serial.write('H');
  } else if (c == 'C') {          // one character
    uint8_t idx = rd();
    uint8_t buf[54]; uint8_t chk = idx;
    for (uint8_t i = 0; i < 54; i++) { buf[i] = rd(); chk ^= buf[i]; }
    uint8_t want = rd();
    if (chk != want) { Serial.write('E'); return; }
    digitalWrite(13, HIGH);
    mwrite(0x09, idx);                            // CMAH = char number
    for (uint8_t i = 0; i < 54; i++) {            // shadow RAM
      mwrite(0x0A, i);                            // CMAL
      mwrite(0x0B, buf[i]);                       // CMDI
    }
    mwrite(0x08, 0xA0);                           // CMM: shadow -> NVM
    while (mread(0xA0) & 0x20);                   // STAT bit5 = NVR busy (~12ms)
    digitalWrite(13, LOW);
    Serial.write('K');
  } else if (c == 'D') {          // done: re-enable display
    mwrite(0x00, 0x0C);           // VM0 = enable display on next vsync
    Serial.write('B'); Serial.write('Y');
  }
}
