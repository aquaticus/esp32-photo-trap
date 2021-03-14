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

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#define LOMEM_MAX_IMAGES 1024

typedef struct
{
	size_t available;
	uint8_t* pointers[LOMEM_MAX_IMAGES];
	size_t sizes[LOMEM_MAX_IMAGES];
	uint8_t* start;
	uint32_t used;
	int counter;
} lomem_cache_t;

bool lomem_cache_init(void);
uint8_t* lomem_cache_get(int index, size_t* size);
bool lomem_cache_put(uint8_t* pic, size_t size);
void lomem_reset(void);
int lomem_cache_get_count(void);
