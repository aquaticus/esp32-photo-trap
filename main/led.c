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

#include "led.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "sdkconfig.h"

#if CONFIG_STATUS_LED_INVERTED
#define LED_ON 0
#else
#define LED_ON 1
#endif

#if CONFIG_STATUS_LED

void led_init(void)
{
    ledc_timer_config_t ledc_timer =
    {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel =
    {
		.channel    = LEDC_CHANNEL_1,
		.duty       = 0,
		.gpio_num   = CONFIG_STATUS_LED_GPIO,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.hpoint     = 0,
		.timer_sel  = LEDC_TIMER_1
    };

    ledc_channel_config(&ledc_channel);
}

void status_led_mode(led_mode_t mode)
{
	switch(mode)
	{
	default:
	case LEDM_OFF:
		ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, !LED_ON);
		break;

	case LEDM_SLOW_BLINK:
		ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 1);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 4000);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        break;

	case LEDM_FAST_BLINK:
		ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 10);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 6000);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        break;

	case LEDM_CONTINOUS:
		ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, LED_ON);
		break;
	}
}

#endif

