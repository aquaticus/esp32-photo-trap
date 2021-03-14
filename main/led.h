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

typedef enum
{
	LEDM_OFF,
	LEDM_CONTINOUS,
	LEDM_SLOW_BLINK,
	LEDM_FAST_BLINK
} led_mode_t;

void led_init(void);
void status_led_mode(led_mode_t mode);
