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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include <esp_err.h>
#include "nvs_flash.h"
#include <esp_sleep.h>
#include <esp_wifi.h>
#include "global_event.h"
#include "cam.h"
#include "wifi.h"
#include "mqtt.h"
#include "pic_cache.h"
#include <driver/uart.h>
#include "ntp.h"
#include "sdkconfig.h"
#include "pir.h"
#include "esp_system.h"
#include "driver/rtc_io.h"
#include "led.h"
#include "sdcard.h"
#include "esp_camera.h"

const char* TAG="photo-trap";
static bool first_boot = true; //after power up
void transfer_pics(void);

// this should be longer than wifi connection timeout
#define MAX_WIFI_CONNECTION_TIMEOUT_MS 45000

// TODO: Light sleep got problems with camera initialization producing invalid images
#define CONFIG_DEEP_SLEEP 1

void fatal_error()
{
	ESP_LOGE(TAG, "FATAL ERROR.");
	ir_off();
#if CONFIG_STATUS_LED
		status_led_mode(LEDM_OFF);
#endif
	camera_power_down();
	esp_wifi_stop();

	//esp_deep_sleep_start();
	abort();
}

EventGroupHandle_t global_event_group;

void show_wakeup_reason()
{
	esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();

	switch (reason)
	{
	case ESP_SLEEP_WAKEUP_EXT0:
		ESP_LOGI(TAG, "Wakeup cause: RTC_IO (EXT0)");
		break;

	case ESP_SLEEP_WAKEUP_EXT1:
		ESP_LOGI(TAG, "Wakeup cause: RTC_CNTL (EXT1)");
		break;

	case ESP_SLEEP_WAKEUP_TIMER:
		ESP_LOGI(TAG, "Wakeup cause: timer");
		break;

	case ESP_SLEEP_WAKEUP_TOUCHPAD:
		ESP_LOGI(TAG, "Wakeup cause: touchpad");
		break;

	case ESP_SLEEP_WAKEUP_ULP:
		ESP_LOGI(TAG, "Wakeup cause: ULP");
		break;

	default:
		ESP_LOGI(TAG, "Wakeup cause: Other %d\n", reason);
		break;
	}
}

void app_main(void)
{
#if CONFIG_CAMERA_TIMER_SECS>0
	int64_t start;
	start = esp_timer_get_time();
#endif

	esp_log_level_set("*", ESP_LOG_ERROR);
	esp_log_level_set(TAG, ESP_LOG_DEBUG);

	ESP_LOGI(TAG, "Application start...");
	esp_reset_reason_t reset_reason = esp_reset_reason();
	if( ESP_RST_DEEPSLEEP == reset_reason )
	{
		ESP_LOGI(TAG, "Wakeup from DEEP SLEEP");
		first_boot = false;
		show_wakeup_reason();
	}

#if CONFIG_CAMERA_TIMER_SECS>0
	ESP_LOGI(TAG, "Timer configured as wakup source. %d secs", CONFIG_CAMERA_TIMER_SECS);
#else
	ESP_LOGI(TAG, "Timer not configured as wakup source.");
#endif

	// Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    global_event_group = xEventGroupCreate();

    if( !camera_setup() )
    {
    	ESP_LOGE(TAG, "CAMERA NOT DETECTED.");
    	fatal_error();
    }

#if CONFIG_STATUS_LED
    led_init();

    status_led_mode(LEDM_SLOW_BLINK);
#endif

    ir_off();

#if CONFIG_PIR_ENABLED
    if( !pir_setup_wakeup() )
    {
    	fatal_error();
    }
#else
	ESP_LOGI(TAG, "PIR/Switch not set as wakeup source");
#endif

#if CONFIG_SDCARD_ENABLED
    sdcard_init();
#endif

    mqtt_init();
	wifi_init_sta();

    if( !pic_cache_init() )
    {
    	fatal_error();
    }

    if( !create_camera_task() )
    {
    	fatal_error();
    }

    while(1) //main loop/sleep
    {
#if CONFIG_IR_ILLUMINATOR_HOLD_PIN_STATE_IN_SLEEP
    	// to make it possible changing illuminator pin state
	    if( ESP_OK != rtc_gpio_hold_dis(CONFIG_IR_ILLUMINATOR_GPIO) )
	    {
	    	ESP_LOGE(TAG, "esp_err_trtc_gpio_hold_dis pin is not RTC IO pad.");
	    }
#endif
    	pic_cache_reset();

		//start taking pictures immediately for defined period of time
		camera_start();

#if !CONFIG_PARALLEL_PROCESSING
    	// When WiFi and MQTT operations are in the same time as picture processing,
    	// resulting images may be corrupted. For better results first take pictures
    	// than establish WiFi and MQTT connection and transmit pictures
    	camera_wait();
#endif

    	ESP_LOGI(TAG, "Initiate WiFi connection");
#if CONFIG_STATUS_LED
    	status_led_mode(LEDM_FAST_BLINK);
#endif
    	ESP_ERROR_CHECK(esp_wifi_start() );

		// wait for wifi
		ESP_LOGI(TAG, "Waiting for WiFi to connect...");
		EventBits_t bits = xEventGroupWaitBits( global_event_group,
												WIFI_CONNECTED_BIT,
												pdTRUE,
												pdFALSE,
												MAX_WIFI_CONNECTION_TIMEOUT_MS / portTICK_PERIOD_MS );

		if( (bits & WIFI_CONNECTED_BIT) ==  WIFI_CONNECTED_BIT )
		{
			mqtt_connect();
		}
		else
		{
			ESP_LOGI(TAG, "WIFI Connection Timeout");
			camera_wait();
		}

#if CONFIG_TIME_SYNRONIZATION
	    time_t now;
	    struct tm timeinfo;
	    time(&now);
	    localtime_r(&now, &timeinfo);

	    if (timeinfo.tm_year < (2016 - 1900))
	    {
	        ESP_LOGI(TAG, "Time is not set yet. Getting time over NTP.");
	        ntp_obtain_time();

	    }
	    else
	    {
	    	ESP_LOGI(TAG, "Time is already synchronized");
		    print_time();
	    }
#endif

	    //wait for MQTT
		if( mqtt_wait(first_boot) )
		{
#if CONFIG_STATUS_LED
			status_led_mode(LEDM_CONTINOUS);
#endif
			transfer_pics();
		}
		else
		{
			ESP_LOGI(TAG, "MQTT Connection Timeout");
			//not connected
			camera_wait();
		}

		ESP_LOGI(TAG, "Preparing for sleep mode");
		mqtt_stop();
		esp_wifi_stop(); //typically MQTT client reports an error here

		first_boot = false;

#if CONFIG_CAMERA_TIMER_SECS>0
		int64_t end = esp_timer_get_time();
		int64_t work_time_ms = (end-start)/1000;
		int64_t wakeup_delay = CONFIG_CAMERA_TIMER_SECS*1000/*ms*/-work_time_ms;
		if( wakeup_delay <= 0 )
		{
			ESP_LOGW(TAG, "Camera timer value is too low resulting in continuous work. Increase CONFIG_CAMERA_TIMER_SECS");
		}
		ESP_LOGI(TAG, "Total work time: %lld ms. Timer: %d s. Wakeup delay: %lld ms. ", work_time_ms, CONFIG_CAMERA_TIMER_SECS, wakeup_delay );
		esp_sleep_enable_timer_wakeup(wakeup_delay*1000/*convert to us*/);
#endif

#if CONFIG_DEEP_SLEEP
		ESP_LOGI(TAG, "DEEP SLEEP MODE"); //~0.01 mA
#else
		ESP_LOGI(TAG, "LIGHT SLEEP MODE"); //~0.8 mA
#endif
		//		uart_wait_tx_done(UART_NUM_0, 100/portTICK_PERIOD_MS);
		vTaskDelay(100 / portTICK_PERIOD_MS); //for UART to flush

#if CONFIG_IR_ILLUMINATOR_HOLD_PIN_STATE_IN_SLEEP
	    // keep pin state in sleep mode. Works only for RTC pins
	    if( ESP_OK != rtc_gpio_hold_en(CONFIG_IR_ILLUMINATOR_GPIO) )
	    {
	    	// this error can be ignored if there is an external circuit that keeps off
	    	// state of the pin in sleep mode
	    	ESP_LOGE(TAG, "IR illuminator pin is not RTC IO pad.");
	    }
#endif

#if CONFIG_STATUS_LED
		status_led_mode(LEDM_OFF);
#endif

#if CONFIG_DEEP_SLEEP
		esp_deep_sleep_start();
#else
		esp_light_sleep_start();
		ESP_LOGI(TAG, "------------ LIGHT SLEEP WAKEUP ------------");
#if CONFIG_CAMERA_TIMER_SECS>0
    	start = esp_timer_get_time();
#endif
#endif
    }
}

void transfer_pics(void)
{
	int counter=0;
	int pic_count = -1;
	uint8_t* p;
	size_t s;
	EventBits_t bits;

	ESP_LOGD(TAG, "Start picture transfer");

	do
	{
		p = pic_cache_get(counter, &s);
		if( !p )
		{
			ESP_LOGD(TAG, "Waiting for picture to be available");
			//check if camera task is finished
			bits = xEventGroupWaitBits( global_event_group,
									CAMERA_FINISHED_BIT,
									pdFALSE,
									pdFALSE,
									100 / portTICK_PERIOD_MS );

			if( (bits & CAMERA_FINISHED_BIT) ==  CAMERA_FINISHED_BIT )
			{
				pic_count = pic_cache_get_count();
				ESP_LOGI(TAG, "### Final number of pictures: %d ###", pic_count);
			}
		}
		else
		{
			mqtt_transfer_pic(p, s);
#if CONFIG_SDCARD_ENABLED
			sdcard_transfer_pic(p, s);
#endif

			counter++;
		}
	} while(counter != pic_count );
}

