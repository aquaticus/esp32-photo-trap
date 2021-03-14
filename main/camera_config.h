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

#if CONFIG_CAMERA_MODEL_WROVER_KIT
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM       5
#define Y2_GPIO_NUM       4
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

#define CAMERA_MODEL "WROVER KIT"

#elif CONFIG_CAMERA_MODEL_ESP_EYE
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    4
#define SIOD_GPIO_NUM    18
#define SIOC_GPIO_NUM    23

#define Y9_GPIO_NUM      36
#define Y8_GPIO_NUM      37
#define Y7_GPIO_NUM      38
#define Y6_GPIO_NUM      39
#define Y5_GPIO_NUM      35
#define Y4_GPIO_NUM      14
#define Y3_GPIO_NUM      13
#define Y2_GPIO_NUM      34
#define VSYNC_GPIO_NUM   5
#define HREF_GPIO_NUM    27
#define PCLK_GPIO_NUM    25

#define CAMERA_MODEL "ESP EYE"

#elif CONFIG_CAMERA_MODEL_M5STACK_PSRAM
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    15
#define XCLK_GPIO_NUM     27
#define SIOD_GPIO_NUM     25
#define SIOC_GPIO_NUM     23

#define Y9_GPIO_NUM       19
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       39
#define Y5_GPIO_NUM        5
#define Y4_GPIO_NUM       34
#define Y3_GPIO_NUM       35
#define Y2_GPIO_NUM       32
#define VSYNC_GPIO_NUM    22
#define HREF_GPIO_NUM     26
#define PCLK_GPIO_NUM     21

#define CAMERA_MODEL "P5STACK PSRAM"

#elif CONFIG_CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define CAMERA_MODEL "AI THINKER"

#elif CONFIG_CAMERA_MODEL_TTGO_TCAMERA_V05
#define PWDN_GPIO_NUM     26
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     32
#define SIOD_GPIO_NUM     13
#define SIOC_GPIO_NUM     12

#define Y9_GPIO_NUM       39
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       23
#define Y6_GPIO_NUM       18
#define Y5_GPIO_NUM       15
#define Y4_GPIO_NUM        4
#define Y3_GPIO_NUM       14
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    27
#define HREF_GPIO_NUM     25
#define PCLK_GPIO_NUM     19

#define CAMERA_MODEL "TTGO TCAMERA V05"

#elif CONFIG_CAMERA_MODEL_TTGO_TCAMERA_V17
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     32
#define SIOD_GPIO_NUM     13
#define SIOC_GPIO_NUM     12

#define Y9_GPIO_NUM       39
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       23
#define Y6_GPIO_NUM       18
#define Y5_GPIO_NUM       15
#define Y4_GPIO_NUM        4
#define Y3_GPIO_NUM       14
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    27
#define HREF_GPIO_NUM     25
#define PCLK_GPIO_NUM     19

#define CAMERA_MODEL "TTGO TCAMERA V17"

#elif CONFIG_CAMERA_MODEL_TTGO_TJOURNAL
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     27
#define SIOD_GPIO_NUM     25
#define SIOC_GPIO_NUM     23

#define Y9_GPIO_NUM       19
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       39
#define Y5_GPIO_NUM        5
#define Y4_GPIO_NUM       34
#define Y3_GPIO_NUM       35
#define Y2_GPIO_NUM       17
#define VSYNC_GPIO_NUM    22
#define HREF_GPIO_NUM     26
#define PCLK_GPIO_NUM     21

#define CAMERA_MODEL "TTGO TJOURNAL"

#else
#error No camera board selected
#endif
