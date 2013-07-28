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
#include <avr/interrupt.h>
#include <avr/io.h>
#include "uart.h"

#define BAUD 9600
#include <util/setbaud.h>

#define BUFFER_SIZE 16

volatile static uint8_t rx_buffer[BUFFER_SIZE];
volatile static uint8_t rx_head = 0;
volatile static uint8_t rx_tail = 0;
volatile static uint8_t tx_buffer[BUFFER_SIZE];
volatile static uint8_t tx_head = 0;
volatile static uint8_t tx_tail = 0;

ISR(USART_RX_vect) {
  uint8_t tmp_head = (rx_head + 1) % BUFFER_SIZE;
  if (tmp_head != rx_tail) {
    rx_buffer[rx_head] = UDR0;
    rx_head = tmp_head;
  }
}

ISR(USART_UDRE_vect) {
  if (tx_head != tx_tail) {
    uint8_t tmp_tail = (tx_tail + 1) % BUFFER_SIZE;
    UDR0 = tx_buffer[tx_tail];
    tx_tail = tmp_tail;
  } else {
    UCSR0B &= ~_BV(UDRIE0);
  }
}

void uart_init(){
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
  UCSR0B = _BV(RXCIE0) | _BV(RXEN0) | _BV(TXEN0);
}

bool uart_has_data(void) {
  return rx_head != rx_tail;
}

uint8_t uart_getc(void) {
  uint8_t tmp_tail = (rx_tail + 1) % BUFFER_SIZE;
  uint8_t c = rx_buffer[rx_tail];
  rx_tail = tmp_tail;
  return c;
}

void uart_putc(const uint8_t c) {
  uint8_t tmp_head = (tx_head + 1) % BUFFER_SIZE;
  while (tmp_head == tx_tail);
  tx_buffer[tx_head] = c;
  tx_head = tmp_head;
  UCSR0B |= _BV(UDRIE0);
}

void uart_puts(const char* s) {
  while (*s) {
    uart_putc(*s);
    s++;
  }
}
