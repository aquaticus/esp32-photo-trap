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

#include "himem_cache.h"
#include "sdkconfig.h"
#include <esp_log.h>
#include <string.h>

extern const char* TAG;

#ifdef CONFIG_SPIRAM_BANKSWITCH_RESERVE
#define MAP_WINDOW_SIZE (ESP_HIMEM_BLKSZ*CONFIG_SPIRAM_BANKSWITCH_RESERVE)
#else
#define MAP_WINDOW_SIZE 0
#endif

#define COMPUTE_BLOCK_NUMBER(address) ((uint32_t)address / ESP_HIMEM_BLKSZ)

static himem_cache_t himem_cache;

void himem_cache_release_mem(void)
{
	if( himem_cache.ptr )
	{
		ESP_ERROR_CHECK(esp_himem_unmap(himem_cache.range_handle, himem_cache.ptr, MAP_WINDOW_SIZE));
		himem_cache.ptr = NULL;
	}
}

static size_t himem_cache_prepare_mem(uint8_t* addr)
{
	himem_cache_release_mem();

    uint32_t block_addr = COMPUTE_BLOCK_NUMBER(addr) * ESP_HIMEM_BLKSZ; // always aligned to 32KiB block boundary

    uint32_t free = himem_cache.available - block_addr;
    size_t map_window = free > MAP_WINDOW_SIZE ? MAP_WINDOW_SIZE : free; //everything is aligned to 32KiB

    ESP_LOGD(TAG, "  MAP Addr: 0x%X block addr: 0x%X map_window: 0x%X", (unsigned)addr, block_addr, map_window );

    ESP_ERROR_CHECK(esp_himem_map(himem_cache.handle, himem_cache.range_handle, block_addr, 0, map_window, 0, (void**)&himem_cache.ptr));

    return map_window;
}

bool himem_cache_put(uint8_t* pic, size_t size)
{
	if( himem_cache.available - himem_cache.used >= size && himem_cache.counter < HIMEM_MAX_IMAGES)
	{
		ESP_LOGD(TAG, "Picture in himem cache %d (%d). Free: %d", himem_cache.counter, size, himem_cache.available-himem_cache.used);

		size_t win_size = himem_cache_prepare_mem((uint8_t*)himem_cache.used);
		if( size > win_size )
		{
			// Picture size is too big
			ESP_LOGE(TAG, "Image doesn't fit in himem cache map window.");
			return false; //as side effect, no more images will be put in cache
		}
        memcpy(himem_cache.ptr+himem_cache.used % ESP_HIMEM_BLKSZ, pic, size);

        himem_cache.pointers[himem_cache.counter] = (uint8_t*)himem_cache.used;
        himem_cache.sizes[himem_cache.counter] = size;
        himem_cache.counter++;
        himem_cache.used += size;

        himem_cache_release_mem();
		return true;
	}
	else
	{
		ESP_LOGD(TAG, "No free memory or slot images in himem cache");
		return false;
	}
}

uint8_t* himem_cache_get(int index, size_t* size)
{
	if( index >= himem_cache.counter )
	{
		ESP_LOGD(TAG, "Image #%d not available in himem cache", index);
		return NULL;
	}

	himem_cache_prepare_mem(himem_cache.pointers[index]);
	*size = himem_cache.sizes[index];
	return (uint8_t*)(himem_cache.ptr + ((uint32_t)himem_cache.pointers[index] % ESP_HIMEM_BLKSZ));
}

static void himem_alloc_map(void)
{
	if( !himem_cache.range_handle )
	{
		ESP_ERROR_CHECK(esp_himem_alloc_map_range(MAP_WINDOW_SIZE, &himem_cache.range_handle));
	}
}

void himem_free_map(void)
{
	if( himem_cache.range_handle )
	{
		ESP_ERROR_CHECK(esp_himem_free_map_range(himem_cache.range_handle));
		himem_cache.range_handle = NULL;
	}
}

void himem_cache_init(void)
{
	memset(&himem_cache, 0, sizeof(himem_cache));

	himem_cache.available = esp_himem_get_free_size();
	if( !himem_cache.available )
	{
		ESP_LOGW(TAG, "Himem not available");
		return;
	}

	if( ESP_OK != esp_himem_alloc(himem_cache.available, &himem_cache.handle ) )
	{
		ESP_LOGE(TAG, "Failed to allocate himem %d bytes", himem_cache.available);
		himem_cache.available = 0;
	}

	ESP_LOGI(TAG, "Himem allocated: %d of %d", himem_cache.available, esp_himem_get_phys_size() );

	himem_alloc_map();
}

void himem_reset(void)
{
	himem_cache.counter=0;
	himem_cache.used=0;
}

int himem_cache_get_count(void)
{
	return himem_cache.counter;
}

#ifdef TEST
void himem_cache_test()
{
	static uint8_t buf[1024*33];

	ESP_LOGI(TAG, "himem_cache_test");

	himem_cache_init();

	for(int i=0;i<10;i++)
	{
		memset(buf,i, sizeof(buf));
		ESP_LOGI(TAG, "Put #%d", i);
		himem_cache_put(buf, sizeof(buf));
	}

	uint8_t* p;
	size_t s;

	for(int i=0;i<10;i++)
	{
		p = himem_cache_get(i, &s);
		ESP_LOGI(TAG, "Get %d size %d", i, s);
		for(int n=0;n<sizeof(buf);n++)
		{
			if( p[n] != i )
			{
				ESP_LOGE(TAG, "Invalid image #%d", i);
				return;
			}
		}
		ESP_LOGI(TAG, "Content verified #%d", i);
	}
}
#endif
