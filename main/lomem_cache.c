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
#include <esp_heap_caps.h>
#include "esp_log.h"
#include <string.h>

extern const char* TAG;

static lomem_cache_t lomem_cache;

bool lomem_cache_init(void)
{
    // allocate mem for buffer
	size_t lomem_size = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
	lomem_cache.start = heap_caps_malloc(lomem_size, MALLOC_CAP_SPIRAM);
	lomem_cache.available = lomem_cache.start ? lomem_size : 0;
	if( !lomem_cache.start )
	{
		ESP_LOGE(TAG, "Failed to allocate lomem");
		return false;
	}
	ESP_LOGI(TAG, "Lomem allocated: %d", lomem_size);

	return true;
}

bool lomem_cache_put(uint8_t* pic, size_t size)
{
	if( lomem_cache.available - lomem_cache.used >= size && lomem_cache.counter < LOMEM_MAX_IMAGES)
	{
		uint32_t p = (uint32_t)lomem_cache.start + lomem_cache.used;
		ESP_LOGD(TAG, "Picture #%d in lomem cache @ 0x%X. Free %d. Used %d.", lomem_cache.counter, p, lomem_cache.available - lomem_cache.used, lomem_cache.used);

		memcpy((void*)p, pic, size);
		lomem_cache.pointers[lomem_cache.counter] = lomem_cache.start + lomem_cache.used;
		lomem_cache.sizes[lomem_cache.counter] = size;
		lomem_cache.counter++;
		lomem_cache.used += size;

		return true;
	}
	else
	{
		ESP_LOGD(TAG, "No free memory or image slots in lomem cache");
		return false;
	}
}

uint8_t* lomem_cache_get(int index, size_t* size)
{
	if( index >= lomem_cache.counter )
	{
		ESP_LOGD(TAG, "Image #%d not available in lomem cache", index);
		return false;
	}

	*size = lomem_cache.sizes[index];
	return lomem_cache.pointers[index];
}

void lomem_reset(void)
{
	lomem_cache.counter=0;
	lomem_cache.used=0;
}

int lomem_cache_get_count(void)
{
	return lomem_cache.counter;
}
