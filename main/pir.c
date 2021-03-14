/*
esp32-photo-trap
Copyright (C) 2020 aquaticus

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pir.h"
#include "sdkconfig.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/rtc_io.h"

#if CONFIG_PIR_ENABLED

extern const char* TAG;

bool pir_setup_wakeup(void)
{
//	ESP_ERROR_CHECK(gpio_set_direction(CONFIG_PIR_GPIO, GPIO_MODE_INPUT));
#if CONFIG_PIR_INPUT_INVERTED
	int level = 0;
#else
	int level = 1;
#endif

#if CONFIG_PIR_INPUT_PULLUP
	ESP_ERROR_CHECK(rtc_gpio_pullup_en(CONFIG_PIR_GPIO));
	ESP_LOGI(TAG, "Pullup enabled for GPIO %d", CONFIG_PIR_GPIO);
#endif

	esp_err_t err = esp_sleep_enable_ext0_wakeup(CONFIG_PIR_GPIO, level);
	if( ESP_OK != err )
	{
		ESP_LOGE(TAG, "Cannot set %d pin as wakeup source.", CONFIG_PIR_GPIO);
		return false;
	}

	ESP_LOGI(TAG, "PIR/Switch on GPIO %d set as wakeup source", CONFIG_PIR_GPIO);

	return true;
}
#endif

