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
#include <util/twi.h>
#include "rtc.h"
#include "time.h"

#define TWI_BITRATE    50000
#define TWBR_VALUE     F_CPU / 2 / TWI_BITRATE - 8

#define DS1307_ADDRESS 0b1101000

#define TWI_START      _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE)
#define TWI_WRITE      _BV(TWINT) | _BV(TWEN) | _BV(TWIE)
#define TWI_READ_ACK   _BV(TWINT) | _BV(TWEN) | _BV(TWEA) | _BV(TWIE)
#define TWI_READ_NACK  _BV(TWINT) | _BV(TWEN) | _BV(TWIE)
#define TWI_STOP       _BV(TWINT) | _BV(TWSTO) | _BV(TWEN)

#define STATE_IDLE     0
#define STATE_WRITE    1
#define STATE_READ     2

#define BUFFER_SIZE    8

#define DEBUG_DDR       DDRD
#define DEBUG_PORT      PORTD
#define DEBUG_PIN       PD5

volatile uint8_t buffer[BUFFER_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t state = STATE_IDLE;
void (*getTimeCallback)(time_t* time);

static uint8_t toBcd(uint8_t value) {
  return ((value / 10) << 4) | (value % 10);
}

static uint8_t fromBcd(uint8_t value) {
  return (value >> 4) * 10 + (value & 0x0F);
}

static void decodeTime() {
  time_t time;

  time.seconds = fromBcd(buffer[0]);
  time.minutes = fromBcd(buffer[1]);
  time.hours = fromBcd(buffer[2]);
  time.dayOfWeek = buffer[3];
  time.day = fromBcd(buffer[4]);
  time.month = fromBcd(buffer[5]);
  time.year = fromBcd(buffer[6]);

  getTimeCallback(&time);
}

ISR(TWI_vect) {
  static uint8_t bufferPos;
  uint8_t status = TW_STATUS;

  if (status == TW_START) {
    TWDR = (DS1307_ADDRESS << 1) | TW_WRITE;
    TWCR = TWI_WRITE;
    bufferPos = 0;
  } else if (status == TW_MT_SLA_ACK) {
    TWDR = 0x00;
    TWCR = TWI_WRITE;
  } else if (status == TW_MT_DATA_ACK) {
    if (state == STATE_WRITE) {
      if (bufferPos < BUFFER_SIZE) {
	TWDR = buffer[bufferPos];
	TWCR = TWI_WRITE;
	bufferPos += 1;
      } else {
	TWCR = TWI_STOP;
	state = STATE_IDLE;
      }
    } else if (state == STATE_READ) {
      TWCR = TWI_START;
    }
  } else if (status == TW_REP_START) {
    TWDR = (DS1307_ADDRESS << 1) | TW_READ;
    TWCR = TWI_WRITE;
  } else if (status == TW_MR_SLA_ACK) {
    TWCR = TWI_READ_ACK;
  } else if (status == TW_MR_DATA_ACK) {
    buffer[bufferPos] = TWDR;
    bufferPos += 1;
    if (bufferPos < BUFFER_SIZE - 1) {
      TWCR = TWI_READ_ACK;
    } else {
      TWCR = TWI_READ_NACK;
    }
  } else if (status == TW_MR_DATA_NACK) {
    buffer[bufferPos] = TWDR;
    TWCR = TWI_STOP;
    state = STATE_IDLE;
    decodeTime();
  } else {
    DEBUG_PORT |= _BV(DEBUG_PIN);
    TWCR = TWI_STOP;
    state = STATE_IDLE;
  }
}

void initRtc() {
  TWBR = TWBR_VALUE;
}

void writeTime(time_t* time) {
  if (state != STATE_IDLE) {
    return;
  }

  buffer[0] = toBcd(time->seconds);
  buffer[1] = toBcd(time->minutes);
  buffer[2] = toBcd(time->hours);
  buffer[3] = time->dayOfWeek;
  buffer[4] = toBcd(time->day);
  buffer[5] = toBcd(time->month);
  buffer[6] = toBcd(time->year);
  buffer[7] = 0x00;
  state = STATE_WRITE;
  TWCR = TWI_START;
}

void readTime(void (*callback)(time_t* time)) {
  if (state != STATE_IDLE) {
    return;
  }

  state = STATE_READ;
  TWCR = TWI_START;
  getTimeCallback = callback;
}
