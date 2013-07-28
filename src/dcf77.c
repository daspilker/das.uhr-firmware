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
#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include "dcf77.h"

#define DCF77_DATA_DDR  DDRD
#define DCF77_DATA_PORT PIND
#define DCF77_DATA_PIN  PD7

#define DCF77_PON_DDR   DDRD
#define DCF77_PON_PORT  PIND
#define DCF77_PON_PIN   PD6

#define DEBUG_DDR       DDRD
#define DEBUG_PORT      PORTD
#define DEBUG_PIN       PD5

#define DCF77_DATA_SIZE 8

volatile uint8_t dcf77Data[DCF77_DATA_SIZE];

static void clearDcf77Bits() {
  for (uint8_t i = 0; i < DCF77_DATA_SIZE; i += 1) {
    dcf77Data[i] = 0;
  }
}

static uint8_t getDcf77Bit(uint8_t bit) {
  return dcf77Data[bit / DCF77_DATA_SIZE] & _BV(bit % 8);
}

static void setDcf77Bit(uint8_t bit) {
  dcf77Data[bit / DCF77_DATA_SIZE] |= _BV(bit % 8);
}

static bool validateDcf77Parity(uint8_t from, uint8_t to) {
  uint8_t parity = 0;
  for (uint8_t i = from; i < to; i += 1) {
    if (getDcf77Bit(i)) {
      parity += 1;
    }
  }
  if (parity % 2) {
    return getDcf77Bit(to) != 0;
  } else {
    return getDcf77Bit(to) == 0;
  } 
}

static bool validateDcf77() {
  return getDcf77Bit(0) == 0 &&
    getDcf77Bit(20) != 0 &&
    validateDcf77Parity(21, 28) &&
    validateDcf77Parity(29, 35) &&
    validateDcf77Parity(36, 58);
}

static uint8_t decodeDcf77Bcd(uint8_t from, uint8_t to) {
  uint8_t result = 0;
  for (uint8_t i = 0; i <= to - from; i += 1) {
    if (getDcf77Bit(i + from)) {
      result += _BV(i);
    }
  }
  return result;
}

static void decodeDcf77(time_t* time) {
  time->seconds = 0;
  time->minutes = decodeDcf77Bcd(25, 27) * 10 + decodeDcf77Bcd(21, 24);
  time->hours = decodeDcf77Bcd(33, 34) * 10 + decodeDcf77Bcd(29, 32);
  time->day = decodeDcf77Bcd(40, 41) * 10 + decodeDcf77Bcd(36, 39);
  time->month = decodeDcf77Bcd(49, 49) * 10 + decodeDcf77Bcd(45, 48);
  time->year = decodeDcf77Bcd(54, 57) * 10 + decodeDcf77Bcd(50, 53);
  time->dayOfWeek = decodeDcf77Bcd(42, 44);
}

bool trackDcf77(time_t* time) {
  static uint8_t dcf77Ticks;
  static uint8_t dcf77State;  
  static uint8_t dcf77Bit;
  bool result = false;

  if (bit_is_clear(DCF77_PON_DDR, DCF77_PON_PIN)) {
    return result;
  }

  if (bit_is_set(DCF77_DATA_PORT, DCF77_DATA_PIN)) {
    DEBUG_PORT |= _BV(DEBUG_PIN);
    if (dcf77State) {
      dcf77Ticks += 1;
    } else {
      if (dcf77Ticks > 160) {
	clearDcf77Bits();
	dcf77Bit = 0;
      }
      dcf77Ticks = 0;
      dcf77State = 1;
    }
  } else {
    DEBUG_PORT &= ~_BV(DEBUG_PIN);
    if (dcf77State) {
      if (dcf77Ticks > 12 && dcf77Ticks < 36) {
	setDcf77Bit(dcf77Bit);
      }
      if (dcf77Bit == 58) {
	if (validateDcf77()) {
	  decodeDcf77(time);
	  clearDcf77Bits();
	  dcf77Bit = 0;
	  result = true;
	}
      } else {
	dcf77Bit += 1;
      }
      dcf77Ticks = 0;
      dcf77State = 0;
    } else {
      dcf77Ticks += 1;
    }
  }
  return result;
}

void initDcf77() {
  DEBUG_DDR |= _BV(DEBUG_PIN);
  DCF77_PON_DDR |= _BV(DCF77_PON_PIN);
}

void disableDcf77() {
  DCF77_PON_DDR &= ~_BV(DCF77_PON_PIN);
}
