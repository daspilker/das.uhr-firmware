/*
   Copyright 2012 Daniel A. Spilker

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "matrix.h"

#define GSCLK_DDR       DDRB
#define GSCLK_PORT      PORTB
#define GSCLK_PIN       PB0

#define SIN_DDR         DDRB
#define SIN_PORT        PORTB
#define SIN_PIN         PB3

#define SCLK_DDR        DDRB
#define SCLK_PORT       PORTB
#define SCLK_PIN        PB5

#define BLANK_DDR       DDRD
#define BLANK_PORT      PORTD
#define BLANK_PIN       PD3

#define DCPRG_DDR       DDRD
#define DCPRG_PORT      PORTD
#define DCPRG_PIN       PD4

#define VPRG_DDR        DDRC
#define VPRG_PORT       PORTC
#define VPRG_PIN        PC3

#define XLAT_DDR        DDRC
#define XLAT_PORT       PORTC
#define XLAT_PIN        PC2

#define ANODES_CLK_DDR  DDRB
#define ANODES_CLK_PORT PORTB
#define ANODES_CLK_PIN  PB1

#define ANODES_RST_DDR  DDRB
#define ANODES_RST_PORT PORTB
#define ANODES_RST_PIN  PB2

#define setOutput(ddr, pin) ((ddr) |= _BV(pin))
#define setLow(port, pin)   ((port) &= ~_BV(pin))
#define setHigh(port, pin)  ((port) |= _BV(pin))
#define toggle(port, pin)  ((port) ^= _BV(pin))
#define pulse(port, pin)    { setHigh((port), (pin)); setLow((port), (pin)); }

#define GS_DATA_SIZE 24

volatile uint8_t gsData[ROWS][GS_DATA_SIZE];

void initMatrix(void) {
  setOutput(GSCLK_DDR, GSCLK_PIN);
  setOutput(SCLK_DDR, SCLK_PIN);
  setOutput(DCPRG_DDR, DCPRG_PIN);
  setOutput(VPRG_DDR, VPRG_PIN);
  setOutput(XLAT_DDR, XLAT_PIN);
  setOutput(BLANK_DDR, BLANK_PIN);
  setOutput(SIN_DDR, SIN_PIN);
  setOutput(ANODES_CLK_DDR, ANODES_CLK_PIN);
  setOutput(ANODES_RST_DDR, ANODES_RST_PIN);
  setHigh(BLANK_PORT, BLANK_PIN);

  SPCR = _BV(SPE) | _BV(MSTR);
  SPSR = _BV(SPI2X);

  TCCR0A = _BV(WGM01);
  TCCR0B = _BV(CS02) | _BV(CS00);
  OCR0A = 0x03;
  TIMSK0 |= _BV(OCIE0A);
}

void setMatrixData(uint8_t row, uint8_t channel, uint16_t value) {
  uint8_t channelPos = 15 - channel;
  uint8_t i = (channelPos * 3) >> 1;
  if (channelPos % 2 == 0) {
    gsData[row][i] = (uint8_t)((value >> 4));
    gsData[row][i + 1] = (uint8_t) ((gsData[row][i + 1] & 0x0F) | (uint8_t)(value << 4));
  } else {
    gsData[row][i] = (uint8_t) ((gsData[row][i] & 0xF0) | (value >> 8));
    gsData[row][i + 1] = (uint8_t)value;
  }
}

ISR(TIMER0_COMPA_vect) {
  static uint8_t row = 0;

  setHigh(BLANK_PORT, BLANK_PIN);
  if (row == 0) {
    pulse(ANODES_RST_PORT, ANODES_RST_PIN);
  } else {
    pulse(ANODES_CLK_PORT, ANODES_CLK_PIN);
  }
  pulse(XLAT_PORT, XLAT_PIN);
  setLow(BLANK_PORT, BLANK_PIN);
  row += 1;
  if (row == ROWS) {
    row = 0;
  }

  for (uint8_t i = 0; i < GS_DATA_SIZE; i++) {
    SPDR = gsData[row][i];
    loop_until_bit_is_set(SPSR, SPIF);
  }
}
