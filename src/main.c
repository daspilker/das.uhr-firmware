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
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include "dcf77.h"
#include "gamma.h"
#include "matrix.h"
#include "rtc.h"
#include "time.h"
#include "uart.h"

#define MINUTES_PER_HOUR   60

#define MINIMUM_BRIGHTNESS 0x00
#define MAXIMUM_BRIGHTNESS 0xFF

#define TRANSITION_DELAY   5

#define COMMAND_VERSION    'v'
#define COMMAND_BRIGHTNESS 'b'
#define COMMAND_TIME       't'

#define CR                 '\r'
#define LF                 '\n'
#define CRLF               "\r\n"

#define ARGUMENT_BUFFER_SIZE 12

uint16_t minuteData[12][4] = {
  {
    0b11011100000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
  },
  {
    0b11011101111,
    0b00000000000,
    0b00000000000,
    0b11110000000,
  },
  {
    0b11011100000,
    0b00000001111,
    0b00000000000,
    0b11110000000,
  },
  {
    0b11011100000,
    0b11111110000,
    0b00000000000,
    0b11110000000,
  },
  {
    0b11011100000,
    0b00000000000,
    0b11111110000,
    0b11110000000,
  },
  {
    0b11011101111,
    0b00000000000,
    0b00000000111,
    0b00000001111,
  },
  {
    0b11011100000,
    0b00000000000,
    0b00000000000,
    0b00000001111,
  },
  {
    0b11011101111,
    0b00000000000,
    0b00000000000,
    0b11110001111,
  },
  {
    0b11011100000,
    0b00000000000,
    0b11111110111,
    0b00000000000,
  },
  {
    0b11011100000,
    0b11111110000,
    0b00000000111,
    0b00000000000,
  },
  {
    0b11011100000,
    0b00000001111,
    0b00000000111,
    0b00000000000,
  },
  {
    0b11011101111,
    0b00000000000,
    0b00000000111,
    0b00000000000,
  },
};

uint16_t hourData[12][5] = {
  {
    0b00000000000,
    0b00000000000,
    0b00011111000,
    0b00000000000,
    0b00000000000,
  },
  {
    0b00111100000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
  },
  {
    0b11110000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
  },
  {
    0b00000011110,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
  },
  {
    0b00000000000,
    0b11110000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000001111,
    0b00000000000,
    0b00000000000,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b01111100000,
    0b00000000000,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000111111,
    0b00000000000,
  },
  {
    0b00000000000,
    0b00000001111,
    0b00000000000,
    0b00000000000,
    0b00000000000,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00011110000,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b11110000000,
  },
  {
    0b00000000000,
    0b00000000000,
    0b11100000000,
    0b00000000000,
    0b00000000000,
  },
};

uint16_t fullHourData[12][5] = {
  {
    0b00000000000,
    0b00000000000,
    0b00011111000,
    0b00000000000,
    0b00000000111,
  },
  {
    0b00111000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000111,
  },
  {
    0b11110000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000111,
  },
  {
    0b00000011110,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000111,
  },
  {
    0b00000000000,
    0b11110000000,
    0b00000000000,
    0b00000000000,
    0b00000000111,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000001111,
    0b00000000000,
    0b00000000111,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b01111100000,
    0b00000000111,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000111111,
    0b00000000111,
  },
  {
    0b00000000000,
    0b00000001111,
    0b00000000000,
    0b00000000000,
    0b00000000111,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00011110111,
  },
  {
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b00000000000,
    0b11110000111,
  },
  {
    0b00000000000,
    0b00000000000,
    0b11100000000,
    0b00000000000,
    0b00000000111,
  },
};

volatile uint8_t maximum_brightness = MAXIMUM_BRIGHTNESS;
time_t time;

static void rtcCallback(time_t* rtc_time) {
  time = *rtc_time;
}

ISR(TIMER1_COMPA_vect) {
  time_t dcf77Time;
  if (trackDcf77(&dcf77Time)) {
    time = dcf77Time;
    writeTime(&time);
    maximum_brightness = MAXIMUM_BRIGHTNESS;
    disableDcf77();
  } else {
    readTime(&rtcCallback);
  }
}

static void init(void) {
  TCCR1B = _BV(WGM12) | _BV(CS11);
  OCR1AH = 0x27;
  OCR1AL = 0x10;
  TIMSK1 |= _BV(OCIE1A);
}

static void getDisplayTime(time_t* displayTime) {
  uint8_t diff = time.seconds >= 30 ? 3 : 2;
  displayTime->minutes = time.minutes + diff;
  displayTime->hours = time.hours % 12;
  if (displayTime->minutes >= MINUTES_PER_HOUR) {
    displayTime->minutes -= MINUTES_PER_HOUR;
    displayTime->hours += 1;
    if (displayTime->hours == 12) {
      displayTime->hours = 0;
    }
  }
}

static void handleMatrix() {
  static uint8_t rawGsData[ROWS][COLUMNS];
  time_t displayTime;

  getDisplayTime(&displayTime);
  for (uint8_t i = 0; i < ROWS; i += 1) {
    for (uint8_t j = 0; j < COLUMNS; j += 1) {
      uint8_t current = rawGsData[i][j];
      uint16_t data;
      if (i < 4) {
	data = minuteData[displayTime.minutes / 5][i];
      } else {
	if (displayTime.minutes < 5) {
	  data = fullHourData[displayTime.hours][i - 4];
	} else if (displayTime.minutes < 25) {
	  data = hourData[displayTime.hours][i - 4];
	} else {
	  data = hourData[(displayTime.hours + 1) % 12][i - 4];
	}
      }
      uint8_t target = data & _BV(COLUMNS - 1 - j) ? maximum_brightness : MINIMUM_BRIGHTNESS;
      if (current > target) {
	current -= 1;
      } else if (current < target) {
	current += 1;
      } else {
	continue;
      }
      rawGsData[i][j] = current;
      setMatrixData(i, j, getGammaValue(current));
    }
  }
}

static void execute_command(const uint8_t command, const char argument[], const uint8_t argument_length) {
  uart_putc(command);
  if (command == COMMAND_VERSION) {
    uart_puts(VERSION);
  } else if (command == COMMAND_BRIGHTNESS) {
    if (argument_length == 2) {
      sscanf(argument, "%2hhX", &maximum_brightness);
    }
    char formatted_output[3];
    sprintf(formatted_output, "%02X", maximum_brightness);
    uart_puts(formatted_output);
  } else if (command == COMMAND_TIME) {
    if (argument_length == 12) {
      sscanf(argument, "%2hhd%2hhd%2hhd%2hhd%2hhd%2hhd", &time.year, &time.month, &time.day, &time.hours, &time.minutes, &time.seconds);
      writeTime(&time);
    }
    char formatted_time[13];
    sprintf(formatted_time, "%02d%02d%02d%02d%02d%02d", time.year, time.month, time.day, time.hours, time.minutes, time.seconds);
    uart_puts(formatted_time);
  }
  uart_puts(CRLF);
}

static void handleUart() {
  static uint8_t command;
  static char argument[ARGUMENT_BUFFER_SIZE + 1];
  static uint8_t byte_count;
  static uint8_t last_byte = 0;

  if (uart_has_data()) {
    uint8_t byte = uart_getc();
    if (byte_count < ARGUMENT_BUFFER_SIZE + 3) {
      byte_count += 1;
    }
    if (last_byte == CR && byte == LF) {
      if (byte_count > 2) {
	argument[byte_count - 3] = 0x00;
	execute_command(command, argument, byte_count - 3);
      }
      byte_count = 0;
    } else if (byte_count == 1) {
      command = byte;
    } else if (byte_count - 2 < ARGUMENT_BUFFER_SIZE) {
      argument[byte_count - 2] = byte;
    }
    last_byte = byte;
  }
}

int main(void) {
  init();
  initMatrix();
  initRtc();
  uart_init();

  sei();

  time.year = 13;
  time.month = 1;
  time.day = 1;
  time.hours = 10;
  time.minutes = 0;
  time.seconds = 0;
  writeTime(&time);

  for (;;) {
    _delay_ms(TRANSITION_DELAY);
    handleMatrix();
    handleUart();
  }
}
