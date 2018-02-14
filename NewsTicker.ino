#include  "font.h"

/* 
 *  Bash line to extract rss feed:
 *  curl https://hackaday.com/blog/feed/ 2> /dev/null | xpath -q -e "rss/channel/item/title/text()" 2> /dev/null | paste -sd~ | sed -e 's/~/      /g'
 *  (needs sudo apt-get install libxml-xpath-perl )
 *    
 *    Next step: Loop that conencts to the com port, and send appropriate amount of data according to the reports form the arduino
 *  
 */

#define PANELS 3
#define ROW_LEN (PANELS * 8 + 1)
#define ROWS 16
// Connections to board
const byte latchPin = 8;
const byte clockPin = 12;
const byte data_R1 = 10;
const byte data_R2 = 11;
const byte en_74138 = 2;
const byte la_74138 = 3;
const byte lb_74138 = 4;
const byte lc_74138 = 5;
const byte ld_74138 = 6;
byte ScanRow = 0;
unsigned long counter;

// One bit per pixels, stored row after row.
uint8_t frame[64 * 16 / 8 * PANELS + ROWS];
uint16_t offset = 0;

char future_text_buffer[128] = "News Ticker V0.9a                            ";
uint8_t next_char_index = 0;
uint8_t last_char_index = strlen(future_text_buffer);


void setup() {
  Serial.begin(9600);
  pinMode(latchPin, OUTPUT);  pinMode(clockPin, OUTPUT);
  pinMode(data_R1, OUTPUT);   pinMode(data_R2, OUTPUT);

  pinMode(en_74138, OUTPUT);
  pinMode(la_74138, OUTPUT);  pinMode(lb_74138, OUTPUT);
  pinMode(lc_74138, OUTPUT);  pinMode(ld_74138, OUTPUT);

  digitalWrite(en_74138, LOW);
  digitalWrite(data_R1, HIGH); digitalWrite(data_R2, HIGH);

  for (int i = 0 ; i < sizeof(frame) ; i++) {
    frame [i] = 0;
    for (int j = 0 ; j < 8 ; j++) {
      if ((i * 8 + j) % (ROW_LEN * 8) == 0) {
        frame[i] |= 1 << j;
      }
    }
  }
}

void shiftDisplay(int8_t distance) {
  if (distance < 0 && -distance > offset) {
    offset = ROW_LEN * 8 + distance + offset;
  }
  else
  {
    offset = (offset + distance) % (ROW_LEN * 8);
  }
  if (offset % 8 == 0) {
    AddChar((offset + ROW_LEN * 8 - 8) % (ROW_LEN * 8), future_text_buffer[next_char_index]);
    ++next_char_index %= sizeof(future_text_buffer);
  }
}

byte flipByte(byte c)
{
  c = ((c >> 1) & 0x55) | ((c << 1) & 0xAA);
  c = ((c >> 2) & 0x33) | ((c << 2) & 0xCC);
  c = (c >> 4) | (c << 4) ;

  return c;
}
void AddChar(uint16_t x, int16_t c) {
  if (x % 8) {
    return;
  }
  // Replace invalid chracters with spaces.
  if (c < 0x20 || c > 0x7F) c = ' ';
  x = x / 8;
  for (uint16_t y = 0 ; y < 16 ; y++) {
    char val = flipByte(pgm_read_byte(&(font8x16_basic[16 * (c - 0x20) + y])));
    //    if (x == 16) { Serial.print("y ") ; Serial.print(y); Serial.print(" offset "); Serial.print(y * ROW_LEN + x) ; Serial.print(" val ") ; Serial.println(val); }

    frame[y * ROW_LEN + x] = val;
  }
}
unsigned long last_sent = 0;
unsigned long last_shift = 0;
void loop() {
  //cli();
  digitalWrite(en_74138, HIGH);     // Turn off display
  {
    byte bit_offset = offset % 8;
    for (byte i = 0 ; i < 8 * PANELS ; i++) {
      uint16_t index1 = ScanRow * ROW_LEN + (offset / 8 + i) % ROW_LEN;
      uint16_t index2 = ScanRow * ROW_LEN + (offset / 8 + i + 1) % ROW_LEN;
      uint8_t b1 = frame[index1];
      uint8_t b2 = frame[index2];
      for (byte j = bit_offset ; j < 8 ; j++) {
        PORTB &= ~(3 << (data_R1 - 8));                            // data_R2 is LOW; data_R1 is LOW;
        PORTB &= ~(1 << (clockPin - 8));                           // digitalWrite(clockPin,LOW);
        PORTB = (~((b1 >> j) & 1)) << (data_R2 - 8); // bottom set of rows
        PORTB |= 1 << (clockPin - 8);                              // digitalWrite(clockPin,HIGH);
      }
      for (byte j = 0 ; j < bit_offset ; j++) {
        PORTB &= ~(3 << (data_R1 - 8));                            // data_R2 is LOW; data_R1 is LOW;
        PORTB &= ~(1 << (clockPin - 8));                           // digitalWrite(clockPin,LOW);
        PORTB = (~((b2 >> j) & 1)) << (data_R2 - 8); // bottom set of rows
        PORTB |= 1 << (clockPin - 8);                              // digitalWrite(clockPin,HIGH);
      }
    }
  }

  digitalWrite(latchPin, LOW);
  digitalWrite(latchPin, HIGH);
  PORTD = (ScanRow << 3) | (PORTD & 0X87); // Highlight row: pins 3 4 5 6 (la_74138 lb_74138 lc_74138 ld_74138)
  digitalWrite(en_74138, LOW);     // Turn on display
  ++ScanRow %= 16;
  //sei();
  delayMicroseconds(100);

  if (ScanRow == 0) {
    if (Serial.available()) {
      if (next_char_index != (last_char_index + 1) % sizeof(future_text_buffer)) {
        char c = Serial.read();
        future_text_buffer[last_char_index] = c;
        ++last_char_index %= sizeof(future_text_buffer);
      }
    }
    auto now = millis();
    if (now  - last_sent > 2000) {

      uint8_t free_space = (sizeof(future_text_buffer) - 1);
      if (last_char_index > next_char_index) {
        free_space -= (last_char_index - next_char_index);
      }
      else if (last_char_index < next_char_index) {
        free_space = (next_char_index - last_char_index - 1);
      }

      Serial.println(free_space);
      last_sent = now;
    }

    if (last_char_index != next_char_index) {
      shiftDisplay(1);
    }
  }
}
