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

#pragma once

#include <esp32/himem.h>
#include <stdint.h>
#include <stdbool.h>

#define HIMEM_MAX_IMAGES 1024

typedef struct
{
	size_t available;
	uint8_t* pointers[HIMEM_MAX_IMAGES];
	size_t sizes[HIMEM_MAX_IMAGES];
	esp_himem_handle_t handle;
	size_t used;
	int counter;
    esp_himem_rangehandle_t range_handle;
    uint8_t *ptr;
} himem_cache_t;

uint8_t* himem_cache_get(int index, size_t* size);
bool himem_cache_put(uint8_t* pic, size_t size);
void himem_cache_init(void);
void himem_cache_release_mem(void);
void himem_free_map(void);
void himem_reset(void);
int himem_cache_get_count(void);
#ifdef TEST
void himem_cache_test();
#endif
