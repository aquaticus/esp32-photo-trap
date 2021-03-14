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

#include "freertos/event_groups.h"
#include "mqtt_client.h"

#define MAX_MQTT_CONNECTION_TIMEOUT_MS 45000

#define MQTT_ROOT "phototrap"
#define MQTT_TOPIC_LWT MQTT_ROOT "/LWT"
#define MQTT_ONLINE "online"
#define MQTT_OFFLINE "offline"
#define MQTT_TOPIC_IMAGE MQTT_ROOT "/%s/image"

extern esp_mqtt_client_handle_t mqtt_client;

void mqtt_init(void);
void mqtt_connect(void);
void mqtt_transfer_pic(uint8_t* p, size_t s);
void mqtt_stop(void);
bool mqtt_wait(bool first_boot);
