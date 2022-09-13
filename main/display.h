/*
   Copyright 2012-2022 Daniel A. Spilker

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

#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <stdint.h>

#define ROWS    9
#define COLUMNS 11

void display_init();
void set_gs_data(uint8_t row, uint8_t channel, uint16_t value);
uint64_t get_display_update_count();

#endif
