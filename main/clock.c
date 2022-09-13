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

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "clock_data.h"
#include "display.h"
#include "gamma.h"


#define MINUTES_PER_HOUR 60

#define MINIMUM_BRIGHTNESS 0x00
#define MAXIMUM_BRIGHTNESS 0xFF


static TaskHandle_t clock_task_handle;

static void get_display_time(struct tm *display_time)
{
    time_t now;

    time(&now);
    localtime_r(&now, display_time);

    uint8_t diff = display_time->tm_sec >= 30 ? 3 : 2;
    display_time->tm_min += diff;
    if (display_time->tm_min >= MINUTES_PER_HOUR)
    {
        display_time->tm_min -= MINUTES_PER_HOUR;
        display_time->tm_hour += 1;
    }
    display_time->tm_hour = display_time->tm_hour % 12;
}

static void update_display()
{
    static uint8_t raw_gs_data[ROWS][COLUMNS];
    struct tm display_time;

    get_display_time(&display_time);
    for (uint8_t i = 0; i < ROWS; i += 1)
    {
        for (uint8_t j = 0; j < COLUMNS; j += 1)
        {
            uint8_t current = raw_gs_data[i][j];
            uint16_t data;
            if (i < 4)
            {
                data = MINUTE_DATA[display_time.tm_min / 5][i];
            }
            else
            {
                if (display_time.tm_min < 5)
                {
                    data = FULL_HOUR_DATA[display_time.tm_hour][i - 4];
                }
                else if (display_time.tm_min < 25)
                {
                    data = HOUR_DATA[display_time.tm_hour][i - 4];
                }
                else
                {
                    data = HOUR_DATA[(display_time.tm_hour + 1) % 12][i - 4];
                }
            }
            uint8_t target = data & (1 << (COLUMNS - 1 - j)) ? MAXIMUM_BRIGHTNESS : MINIMUM_BRIGHTNESS;
            if (current > target)
            {
                current -= 1;
            }
            else if (current < target)
            {
                current += 1;
            }
            else
            {
                continue;
            }
            raw_gs_data[i][j] = current;
            set_gs_data(i, j, get_gamma_value(current));
        }
    }
}

static void clock_task(void *parameters)
{
    for (;;)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        update_display();
    }

    vTaskDelete(NULL);
}

void clock_init()
{
    xTaskCreatePinnedToCore(clock_task, "CLOCK", 4096, NULL, 2, &clock_task_handle, PRO_CPU_NUM);
}
