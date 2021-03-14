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

#include "cam.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "sdkconfig.h"
#include "camera_config.h"
#include "process.h"
#include <stdio.h>
#include <sys/time.h>
#include "cam.h"
#include "global_event.h"
#include <inttypes.h>
#include "pic_cache.h"
#include <freertos/semphr.h>

#define DEBUG

extern const char* TAG;
extern EventGroupHandle_t global_event_group;

EventGroupHandle_t camera_event_group=NULL;
time_t camera_working_period=CONFIG_CAMERA_WORKING_PERIOD_SECS;
TaskHandle_t camera_task_handle = NULL;

#if CONFIG_IR_ILLUMINATOR_ENABLED
#if CONFIG_IR_ILLUMINATOR_INVERTED
#define IR_LEVEL_ON 0
#else
#define IR_LEVEL_ON 1
#endif
#endif


bool camera_setup(void)
{
	framesize_t f;
#if CONFIG_FRAMESIZE_96x96
	f = FRAMESIZE_96x96;
#elif CONFIG_FRAMESIZE_QQVGA
	f = FRAMESIZE_QQVGA;
#elif CONFIG_FRAMESIZE_QQVGA2
	f = FRAMESIZE_QQVGA2;
#elif CONFIG_FRAMESIZE_QCIF
	f = FRAMESIZE_QCIF;
#elif CONFIG_FRAMESIZE_HQVGA
	f = FRAMESIZE_HQVGA;
#elif CONFIG_FRAMESIZE_240x240
	f = FRAMESIZE_240x240;
#elif CONFIG_FRAMESIZE_QVGA
	f = FRAMESIZE_QVGA;
#elif CONFIG_FRAMESIZE_CIF
	f = FRAMESIZE_CIF;
#elif CONFIG_FRAMESIZE_VGA
	f = FRAMESIZE_VGA;
#elif CONFIG_FRAMESIZE_SVGA
	f = FRAMESIZE_SVGA;
#elif CONFIG_FRAMESIZE_XGA
	f = FRAMESIZE_XGA;
#elif CONFIG_FRAMESIZE_SXGA
	f = FRAMESIZE_SXGA;
#elif CONFIG_FRAMESIZE_UXGA
	f = FRAMESIZE_UXGA;
#elif CONFIG_FRAMESIZE_QXGA
	f = FRAMESIZE_QXGA;
#else
#error Not supported picture resolution
#endif

    if(PWDN_GPIO_NUM != -1)
    {
    	// This is probably done in camera driver, but just in case
		gpio_pad_select_gpio(PWDN_GPIO_NUM);
		gpio_set_direction(PWDN_GPIO_NUM, GPIO_MODE_OUTPUT);
    }

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = f;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    camera_power_up();

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera init failed with error 0x%X", err);
        return false;
    }

#if CONFIG_IR_ILLUMINATOR_ENABLED
    gpio_pad_select_gpio(CONFIG_IR_ILLUMINATOR_GPIO);
    gpio_set_direction(CONFIG_IR_ILLUMINATOR_GPIO, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "Illuminator pin initialized");
#endif

    camera_event_group = xEventGroupCreate();
    if(camera_event_group == NULL)
    {
    	ESP_LOGE(TAG, "Failed to create camera event group");
    }

	xEventGroupSetBits(global_event_group, CAMERA_FINISHED_BIT);

    ESP_LOGI(TAG, "Camera driver initialized");

    return true;
}


// Power down camera
void camera_power_down(void)
{
    if(PWDN_GPIO_NUM != -1)
    {
        esp_err_t err = gpio_set_level(PWDN_GPIO_NUM, 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "camera_power_down error 0x%X", err);
        }

    }
}

// Power up camera
void camera_power_up(void)
{
    if(PWDN_GPIO_NUM != -1)
    {
        esp_err_t err = gpio_set_level(PWDN_GPIO_NUM, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "camera_power_up error 0x%X", err);
        }
    }
}

void ir_on(void)
{
#if CONFIG_IR_ILLUMINATOR_ENABLED
	ESP_LOGI(TAG, "IR on");
    esp_err_t err = gpio_set_level(CONFIG_IR_ILLUMINATOR_GPIO, IR_LEVEL_ON);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ir_on error 0x%X", err);
    }
#endif
}

void ir_off(void)
{
#if CONFIG_IR_ILLUMINATOR_ENABLED
	ESP_LOGI(TAG, "IR off");
    esp_err_t err = gpio_set_level(CONFIG_IR_ILLUMINATOR_GPIO, !IR_LEVEL_ON);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ir_off error 0x%X", err);
    }
#endif
}

static void camera_shoot_and_store()
{
    struct timeval tv_start;
    struct timeval tv_now;
    camera_fb_t * fb;
	int64_t start;
	int64_t end;
	int counter;

	while(1)
	{
        xTaskNotifyWait( 0x00,
                         ULONG_MAX,
                         NULL,
                         portMAX_DELAY );

		ESP_LOGI(TAG, "Camera start working");

		ir_on();
		camera_power_up();

		gettimeofday(&tv_start, NULL);

		counter=0;

		do
		{
			start = esp_timer_get_time();
			fb = esp_camera_fb_get();

			end = esp_timer_get_time();

			if (fb)
			{
				pic_cache_put(fb->buf, fb->len);
				esp_camera_fb_return(fb);
				if( pic_cache_is_full())
				{
					ESP_LOGW(TAG, "Picture cache full");
					break;
				}

				ESP_LOGI(TAG, "Picture #%d %d bytes. Processing (total/pic) %lld/%lld µs", counter, fb->len, (esp_timer_get_time()-start), end-start);

				counter++;
			}
			else
			{
				ESP_LOGE(TAG, "Camera Capture Failed");
			}

			gettimeofday(&tv_now, NULL);

			end = esp_timer_get_time();

#if CONFIG_TIME_BETWEEN_PHOTOS_MS > 0
			int64_t delay = (CONFIG_TIME_BETWEEN_PHOTOS_MS - (end-start))/portTICK_PERIOD_MS;
			if( delay > 0 )
			{
				vTaskDelay(delay);
			}
#endif
		} while(
#if CONFIG_CAMERA_NUMBER_OF_PICTURES > 0
				counter < CONFIG_CAMERA_NUMBER_OF_PICTURES &&
#endif
				tv_now.tv_sec - tv_start.tv_sec < camera_working_period &&
				!pic_cache_is_full());

		ir_off();
		camera_power_down();

		ESP_LOGI(TAG,"Camera task finished");

		xEventGroupSetBits(global_event_group, CAMERA_FINISHED_BIT);
	}
}

bool create_camera_task()
{
	const uint32_t STACK_SIZE = 1024*8;
	const UBaseType_t TASK_PRIORITY = 20; //greater number = higher priority (base is tskIDLE_PRIORITY)
	const BaseType_t CORE = tskNO_AFFINITY; // 0 or 1 or tskNO_AFFINITY

	xTaskCreatePinnedToCore(
			camera_shoot_and_store,
            "CAMERA",
			STACK_SIZE,
            NULL,
			TASK_PRIORITY,
            &camera_task_handle,
			CORE);

	if( camera_task_handle != NULL )
	{
		ESP_LOGI(TAG, "Camera task created");
	}
	else
	{
		ESP_LOGE(TAG, "Camera task creation failed");
	}

	return camera_task_handle != NULL;
}

void camera_start()
{
	xEventGroupClearBits(global_event_group, CAMERA_FINISHED_BIT);

	xTaskNotify( camera_task_handle, 0, eNoAction );
}

void camera_wait()
{
	EventBits_t bits;
	ESP_LOGD(TAG, "camera_wait start");
	do
	{
		bits = xEventGroupWaitBits( global_event_group,
								CAMERA_FINISHED_BIT,
								pdFALSE,
								pdFALSE,
								portMAX_DELAY );
	} while( (bits & CAMERA_FINISHED_BIT) == 0 );

	ESP_LOGD(TAG, "camera_wait exit");
}
