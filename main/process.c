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
#include <mqtt.h>
#include "process.h"
#include "esp_camera.h"
#include "esp_log.h"

extern const char* TAG;

void process_image(size_t width, size_t height, pixformat_t pixformat, uint8_t* buf, size_t buf_size)
{
	int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_IMAGE, (const char *)buf, buf_size, 0, 0);
	ESP_LOGI(TAG, "[%d] MQTT Publishing image...", msg_id);
	ESP_LOGI(TAG, "CAM IMAGE: %dx%d; %d bytes", width, height, buf_size);
}
