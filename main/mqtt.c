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
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "mqtt.h"
#include "global_event.h"
#include "pic_cache.h"
#include "camera_config.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_ota_ops.h"

extern const char* TAG;
#if CONFIG_HOME_ASSISTANT_MQTT_DISCOVERY
static void mqtt_ha_register(void);
#endif
static char mqtt_uid[2*6+1]; //MAC

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
        xEventGroupSetBits(global_event_group, MQTT_CONNECTED_BIT);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupClearBits(global_event_group, MQTT_CONNECTED_BIT);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
#if 0
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        printf("ID=%d, total_len=%d, data_len=%d, current_data_offset=%d\n", event->msg_id, event->total_data_len, event->data_len, event->current_data_offset);
        if (event->topic) {
            actual_len = event->data_len;
            msg_id = event->msg_id;
        } else {
            actual_len += event->data_len;
            // check consisency with msg_id across multiple data events for single msg
            if (msg_id != event->msg_id) {
                ESP_LOGI(TAG, "Wrong msg_id in chunked message %d != %d", msg_id, event->msg_id);
                abort();
            }
        }
        memcpy(actual_data + event->current_data_offset, event->data, event->data_len);
        if (actual_len == event->total_data_len) {
            if (0 == memcmp(actual_data, expected_data, expected_size)) {
                printf("OK!");
                memset(actual_data, 0, expected_size);
                actual_published ++;
                if (actual_published == expected_published) {
                    printf("Correct pattern received exactly x times\n");
                    ESP_LOGI(TAG, "Test finished correctly!");
                }
            } else {
                printf("FAILED!");
                abort();
            }
        }
        break;
#endif
    case MQTT_EVENT_ERROR:
        ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
        break;

    case MQTT_EVENT_BEFORE_CONNECT:
    	ESP_LOGD(TAG, "MQTT_EVENT_BEFORE_CONNECT");
    	break;

    default:
        ESP_LOGD(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

esp_mqtt_client_handle_t mqtt_client = NULL;

void mqtt_init(void)
{
    const esp_mqtt_client_config_t mqtt_cfg =
    {
        .event_handle = mqtt_event_handler,
    };

    uint8_t mac[6];
	ESP_ERROR_CHECK(esp_read_mac(mac, 0/*WiFi*/));
	sprintf(mqtt_uid, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);


    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

	esp_mqtt_client_set_uri(mqtt_client, CONFIG_MQTT_BROKER_TCP_URI);

	xEventGroupClearBits(global_event_group, MQTT_CONNECTED_BIT);
}

void mqtt_connect(void)
{
	ESP_LOGD(TAG, "MQTT init connection process");
	esp_mqtt_client_start(mqtt_client);
}

void mqtt_transfer_pic(uint8_t* p, size_t s)
{
	char topic_image[64];
	sprintf(topic_image, MQTT_TOPIC_IMAGE, mqtt_uid);

	if( esp_mqtt_client_publish(mqtt_client, topic_image, (char*)p, s, 0, 0) < 0 )
	{
		ESP_LOGE(TAG, "MQTT Publish error");
	}
	else
	{
		ESP_LOGI(TAG, "MQTT published image to %s", topic_image);
	}
}

void mqtt_stop(void)
{
	EventBits_t bits = xEventGroupGetBits( global_event_group );

	if( (bits & MQTT_CONNECTED_BIT) ==  MQTT_CONNECTED_BIT)
	{
		ESP_LOGI(TAG, "Publishing offline");
		// send offline message
		if( esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_LWT, MQTT_OFFLINE, strlen(MQTT_OFFLINE), 0, 0) < 0 )
		{
			ESP_LOGE(TAG, "Cannot send MQTT " MQTT_OFFLINE);
		}

		ESP_LOGI(TAG, "MQTT Stop");
		if( ESP_OK != esp_mqtt_client_stop(mqtt_client) )
		{
			ESP_LOGE(TAG, "esp_mqtt_client_stop() failed");
		}

		xEventGroupClearBits(global_event_group, MQTT_CONNECTED_BIT);
	}
}

bool mqtt_wait(bool first_boot)
{
	//wait for MQTT
	ESP_LOGI(TAG, "MQTT Waiting to connect...");
	EventBits_t bits = xEventGroupWaitBits( global_event_group,
											MQTT_CONNECTED_BIT,
											pdTRUE,
											pdFALSE,
											MAX_MQTT_CONNECTION_TIMEOUT_MS / portTICK_PERIOD_MS );

	if( (bits & MQTT_CONNECTED_BIT) ==  MQTT_CONNECTED_BIT )
	{
		ESP_LOGI(TAG, "MQTT CONNECTED");
		if( esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_LWT, MQTT_ONLINE, strlen(MQTT_ONLINE), 0, 0) < 0 )
		{
			ESP_LOGE(TAG, "Cannot send MQTT " MQTT_ONLINE);
		}
#if CONFIG_HOME_ASSISTANT_MQTT_DISCOVERY
		if( first_boot )
		{
			mqtt_ha_register();
		}
#endif
		return true;
	}
	else
	{
		ESP_LOGI(TAG, "MQTT Connection Timeout");
		return false;
	}
}

#if CONFIG_HOME_ASSISTANT_MQTT_DISCOVERY
static void mqtt_ha_register(void)
{
	char discovery_topic[64];
	sprintf(discovery_topic,"homeassistant/camera/%s/config", mqtt_uid);
	const esp_app_desc_t *d = esp_ota_get_app_description();

	char payload[256];
	sprintf(payload,
			"{" \
				"\"name\": \"Photo Trap %s\", " \
				"\"topic\": \"" MQTT_TOPIC_IMAGE "\", " \
				"\"device\": "\
				"{ " \
					"\"identifiers\": [\"%s\"], " \
					"\"manufacturer\": \"Aquaticus\", " \
					"\"sw_version\": \"%s\", " \
					"\"name\": \"Photo Trap\", " \
					"\"model\": \"%s\" " \
				"}" \
			"}",
			mqtt_uid+2*3, //last 3 bytes of MAC
			mqtt_uid,
			mqtt_uid,
			d->version,
			CAMERA_MODEL
			);

	if( esp_mqtt_client_publish(mqtt_client, discovery_topic, payload, strlen(payload), 0, 0) < 0 )
	{
		ESP_LOGE(TAG, "Cannot send Home Assistant discovery");
	}
	else
	{
		ESP_LOGI(TAG, "Home Assistant discovery sent. Topic %s", discovery_topic);
	}
}
#endif
