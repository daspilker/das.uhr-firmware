#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"

#include "clock.h"
#include "display.h"
#include "wifi.h"


static void init_nvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void init_sntp()
{
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void app_main(void)
{
    init_nvs();
    wifi_init();
    init_sntp();
    display_init();
    clock_init();
}
