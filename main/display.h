#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <stdint.h>

#define ROWS    9
#define COLUMNS 11

void display_init();
void set_gs_data(uint8_t row, uint8_t channel, uint16_t value);
uint64_t get_display_update_count();

#endif
