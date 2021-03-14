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

#include "sdcard.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

extern const char* TAG;

#define SDCARD_MOUNT "/sdcard"

static int file_number=0;

void find_max_file_number(void)
{
	DIR* dir = opendir(SDCARD_MOUNT "/");
	if( !dir )
	{
		ESP_LOGE(TAG, "Failed to open directory " SDCARD_MOUNT "/");
		return;
	}

	struct dirent* entry;
	int n;

	while((entry = readdir(dir)))
	{
		ESP_LOGD(TAG, "dir: %s", entry->d_name);
		if( 8+1+3 == strlen(entry->d_name)) /// 12345678.JPG
		{
			entry->d_name[8] = '\0';
			n = atoi(entry->d_name); //for non numeric = -1
			if( n > file_number )
			{
				file_number = n;
			}
		}
	}
	closedir(dir);

	file_number++;

	ESP_LOGD(TAG, "Next sd card file number: %d", file_number);
}

//MMC slot
bool sdcard_init(void)
{
    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    // ESP32-CAM pin 4 connected to LED and SDCARD D1
    // Use 1 line mode to prevent LED blinking during SD CARD operations
    slot_config.width = 1;

    esp_vfs_fat_sdmmc_mount_config_t mount_config =
    {
        .format_if_mount_failed = false,
        .max_files = 2,
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SDCARD_MOUNT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return false;
    }

    find_max_file_number();

    return true;
}


void sdcard_transfer_pic(uint8_t* p, size_t s)
{
	char filename[64];

	sprintf(filename, SDCARD_MOUNT "/%08d.jpg", file_number);
	ESP_LOGI(TAG, "Write %d bytes to %s", s, filename );
	FILE* f = fopen(filename,"w");
	if( !f )
	{
		ESP_LOGE(TAG, "Error opening file for write %s", filename);
		return;
	}

	if( s != fwrite(p, 1, s, f) )
	{
		ESP_LOGE(TAG, "Failed to write all bytes to sdcard" );
	}

	fclose(f);
	file_number++;
}
