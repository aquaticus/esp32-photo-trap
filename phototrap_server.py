#!/usr/bin/env python3

# esp32-photo-trap
# Copyright (C) 2020 aquaticus
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import paho.mqtt.client as mqtt
import os
import io
import sys
from datetime import datetime
import glob
import subprocess
import argparse
from PIL import Image, ImageFont, ImageDraw

last_timestamp = None
image_counter = 0


def on_connect(client, userdata, flags, rc):
    print("MQTT Connected")
    client.subscribe(args.topic)


def on_message(client, userdata, msg):
    global image_counter
    global last_timestamp
    t = msg.topic.split('/')
    now = datetime.now()
    file_timestamp = now.strftime("%Y-%m-%d_%H.%M.%S")
    image_timestamp = now.strftime("%Y-%m-%d %H:%M:%S")

    if file_timestamp == last_timestamp:
        image_counter = image_counter + 1
    else:
        image_counter = 0
        last_timestamp = file_timestamp

    filename = "{0}/{1}_{2}.{3}.jpeg".format(args.output, t[-2], file_timestamp, image_counter)

    img_file = io.BytesIO(msg.payload)
    img = Image.open(img_file)
    try:
        img.verify()
    except Exception:
        sys.stderr.write("Invalid image {0} bytes @ {1}".format(len(msg.payload), image_timestamp))
        return

    status = "OK {0}x{1} {2}".format(img.width, img.height, img.format)
    img = Image.open(io.BytesIO(msg.payload))
    if args.rotate:
        mode = eval('Image.' + args.rotate)
        img = img.transpose(mode)

    if args.timestamp:
        font = ImageFont.load_default()
        draw = ImageDraw.Draw(img)
        s = draw.textsize(image_timestamp, font)
        delta = 2
        x1 = img.width-s[0]-delta
        y1 = img.height-s[1]-delta
        x2 = img.width - delta+1
        y2 = img.height - delta+1
        draw.rectangle([x1, y1, x2, y2], fill=0)
        draw.text([x1+delta/2, y1+delta/2], fill=0xFFFFFF, text=image_timestamp)

    try:
        img.save(filename)
    except Exception as e:
        sys.stderr.write("Write error {0}. {1}".format(filename, e))
        return

    print('{0} {1}'.format(filename, status))


parser = argparse.ArgumentParser(description='Stores images received from MQTT on filesystem')
parser.add_argument('-s', '--host', default="127.0.0.1", help='MQTT host')
parser.add_argument('-p', '--port', default=1883, type=int, help='MQTT port')
parser.add_argument('-o', '--output', help='output directory where images are stored', default='.')
parser.add_argument('-t', '--topic', default="phototrap/+/image", help='Optional image topic')
parser.add_argument('-d', '--delete', help='Delete existing images in output directory', action='store_true')
parser.add_argument('-r', '--rotate', choices=['FLIP_LEFT_RIGHT', 'FLIP_TOP_BOTTOM', 'ROTATE_90', 'ROTATE_180', 'ROTATE_270'], help='Rotate image')
parser.add_argument('--timestamp', help='Add small timestamp', action='store_true')
args = parser.parse_args()

if args.output is not None and not os.path.isdir(args.output):
    raise FileExistsError("Output directory '%s' does not exist" % args.output)

if args.delete:
    for f in glob.glob(args.output + "/*.jpeg"):
        os.remove(f)

client = mqtt.Client()
client.connect(args.host, args.port, 60)
client.on_connect = on_connect
client.on_message = on_message

client.loop_forever()
