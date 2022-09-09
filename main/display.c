/*
  https://github.com/NidasioAlberto/7-segment-clock/blob/master/Programs/7_segment_clock/main/main.c
*/

#include "display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/pcnt.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

#define PIN_GSCLK         GPIO_NUM_22
#define PIN_GSCLK_COUNTER GPIO_NUM_35
#define PIN_SIN           GPIO_NUM_23
#define PIN_SCLK          GPIO_NUM_18
#define PIN_BLANK         GPIO_NUM_21
#define PIN_XLAT          GPIO_NUM_19

static const char *TAG = "DAS.UHR Display";

static const gpio_num_t GPIO_ROWS[] = {
    GPIO_NUM_12,
    GPIO_NUM_13,
    GPIO_NUM_14,
    GPIO_NUM_15,
    GPIO_NUM_25,
    GPIO_NUM_26,
    GPIO_NUM_27,
    GPIO_NUM_32,
    GPIO_NUM_33,
};

static TaskHandle_t gs_task;
static spi_device_handle_t spi;

volatile uint64_t display_update_counter = 0;
DMA_ATTR uint8_t gs_data[ROWS][24] = {0};

static void display_update_task(void *parameters)
{
    ESP_LOGI(TAG, "Started display update task");

    uint8_t current_row = ROWS - 1;
    uint8_t previous_row = current_row - 1;
    for (;;)
    {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        display_update_counter += 1;

        // PREVIOUS CYCLE

        gpio_set_level(PIN_BLANK, 1);

        gpio_set_level(GPIO_ROWS[previous_row], 0);

        gpio_set_level(PIN_XLAT, 1);
        vTaskDelay(1 / portTICK_PERIOD_MS);
        gpio_set_level(PIN_XLAT, 0);

        gpio_set_level(GPIO_ROWS[current_row], 1);

        // NEW CYCLE

        previous_row = current_row;
        current_row = (current_row + 1) % ROWS;

        gpio_set_level(PIN_BLANK, 0);

        spi_transaction_t transaction = {
            .length = 24 * 8,
            .tx_buffer = &gs_data[current_row],
        };

        spi_device_polling_transmit(spi, &transaction);
    }

    vTaskDelete(NULL);
}

static void IRAM_ATTR gs_clk_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(gs_task, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void init_gpio()
{
    uint64_t pin_bit_mask = (1ULL << PIN_BLANK) | (1ULL << PIN_XLAT);
    for (int i = 0; i < ROWS; i += 1)
    {
        pin_bit_mask |= 1ULL << GPIO_ROWS[i];
    }

    gpio_config_t config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pin_bit_mask = pin_bit_mask,
    };
    gpio_config(&config);
}

static void init_ledc()
{
    ledc_timer_config_t timer_config = {
        .timer_num = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_1_BIT,
        .freq_hz = 5000000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .gpio_num = PIN_GSCLK,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 1,
        .hpoint = 0,
    };
    ledc_channel_config(&channel_config);
}

static void init_pcnt()
{
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = PIN_GSCLK_COUNTER,
        .ctrl_gpio_num = PCNT_PIN_NOT_USED,
        .lctrl_mode = PCNT_MODE_KEEP,
        .hctrl_mode = PCNT_MODE_KEEP,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DIS,
        .counter_h_lim = 4095,
        .counter_l_lim = 0,
        .unit = PCNT_UNIT_0,
        .channel = PCNT_CHANNEL_0,
    };
    pcnt_unit_config(&pcnt_config);
    pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_H_LIM);

    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);

    pcnt_isr_service_install(0);
    pcnt_isr_handler_add(PCNT_UNIT_0, gs_clk_handler, NULL);

    pcnt_counter_resume(PCNT_UNIT_0);
}

static void init_spi()
{
    spi_bus_config_t bus_config = {
        .sclk_io_num = PIN_SCLK,
        .mosi_io_num = PIN_SIN,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 24 * 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &bus_config, 0));

    spi_device_interface_config_t device_config = {
        .clock_speed_hz = 5000000,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
        .command_bits = 0,
        .address_bits = 0,
        .input_delay_ns = 10,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &device_config, &spi));
}

void set_gs_data(uint8_t row, uint8_t channel, uint16_t value)
{
    uint8_t channelPos = 15 - channel;
    uint8_t i = (channelPos * 3) >> 1;
    if (channelPos % 2 == 0)
    {
        gs_data[row][i] = (uint8_t)((value >> 4));
        gs_data[row][i + 1] = (uint8_t)((gs_data[row][i + 1] & 0x0F) | (uint8_t)(value << 4));
    }
    else
    {
        gs_data[row][i] = (uint8_t)((gs_data[row][i] & 0xF0) | (value >> 8));
        gs_data[row][i + 1] = (uint8_t)value;
    }
}

uint64_t get_display_update_count()
{
    return display_update_counter;
}

void display_init()
{
    xTaskCreatePinnedToCore(display_update_task, "GS_CYCLE", 4096, NULL, 6, &gs_task, APP_CPU_NUM);

    init_gpio();
    init_spi();
    init_ledc();
    init_pcnt();
}