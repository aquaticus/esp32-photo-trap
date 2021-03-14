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

#include "lomem_cache.h"
#include "himem_cache.h"
#include "freertos/FreeRTOS.h"
#include <freertos/semphr.h>
#include "esp_log.h"
#include "sdkconfig.h"

extern const char* TAG;

#ifdef CONFIG_PARALLEL_PROCESSING
#define MAX_TIME (200/portTICK_PERIOD_MS)
static SemaphoreHandle_t pic_semaphore = NULL;
#endif

static bool cache_full;

bool pic_cache_is_full()
{
	return cache_full;
}

bool pic_cache_init()
{
	cache_full = false;

#ifdef CONFIG_PARALLEL_PROCESSING
	pic_semaphore = xSemaphoreCreateMutex();
	if( !pic_semaphore )
	{
		ESP_LOGE(TAG, "Cannot create pic_semaphore");
		return false;
	}
#endif

	if(!lomem_cache_init())
	{
		return false;
	}

	himem_cache_init();

	return true;
}

bool pic_cache_put(uint8_t* pic, size_t size)
{
	bool r=false;

#ifdef CONFIG_PARALLEL_PROCESSING
	if( pdTRUE == xSemaphoreTake( pic_semaphore, MAX_TIME ) )
	{
#endif

		r = lomem_cache_put(pic, size);
		if( !r )
		{
			r = himem_cache_put(pic, size);
		}

		cache_full = !r;

#ifdef CONFIG_PARALLEL_PROCESSING
		xSemaphoreGive( pic_semaphore );
	}
	else
	{
		ESP_LOGE(TAG, "Error pic_cache_put xSemaphoreTake");
	}
#endif

	return r;
}

uint8_t* pic_cache_get(int index, size_t* size)
{
	uint8_t* p = NULL;

#ifdef CONFIG_PARALLEL_PROCESSING
	if( pdTRUE == xSemaphoreTake( pic_semaphore, MAX_TIME ) )
	{
#endif

		p = lomem_cache_get(index, size);
		if( !p )
		{
			p = himem_cache_get(index-lomem_cache_get_count(), size);
		}

#ifdef CONFIG_PARALLEL_PROCESSING
		xSemaphoreGive( pic_semaphore );
	}
	else
	{
		ESP_LOGE(TAG, "Error pic_cache_get xSemaphoreTake");
	}
#endif

	return p;
}

void pic_cache_reset(void)
{
	cache_full = false;
	lomem_reset();
	himem_reset();
}

/**
 * Returns number of pictures.
 *
 * Use only when camera task is not working!
 */
int pic_cache_get_count(void)
{
	return lomem_cache_get_count() + himem_cache_get_count();
}
