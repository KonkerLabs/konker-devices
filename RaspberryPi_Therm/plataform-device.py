#! /usr/bin/python3

import paho.mqtt.client as mqtt
import json

import os

import threading
from threading import Timer
import time

import RPi.GPIO as GPIO 
GPIO.setmode(GPIO.BOARD) 
GPIO.setup(11, GPIO.IN) 

os.system('modprobe w1-gpio')
os.system('modprobe w1-therm')

### Temperature Sensor DS18B20
## Nota: mudar o diretório de 28-0415a444b2ff para o nome visto em /sys/bus/w1/devices/

base_dir = '/sys/bus/w1/devices/28-0415a444b2ff/'
device_file = base_dir + '/w1_slave'

def read_temp_raw():
	f = open(device_file, 'r')
	lines = f.readlines()
	f.close()
	return lines

def read_temp():
	lines = read_temp_raw()
	while lines[0].strip()[-3:] != 'YES':
		time.sleep(0.2)
		lines = read_temp_raw()
	equals_pos = lines[1].find('t=')
	if equals_pos != -1:
		temp_string = lines[1][equals_pos+2:]
		temp_c = float(temp_string) / 1000.0
		return temp_c

### End of Temperature Sensor DS18B20

### MQTT code

def on_connect(client, userdata, rc):
       print("Connected with result code "+str(rc))
       mqttc.subscribe("sub/" + dev_name + "/sub")
       
def on_message(mqttc, userdata, msg):
    json_data = msg.payload.decode('utf-8')
    print("Message received: "+json_data)
    global firmware_ver, measure_type, measure_value, measure_unit
    data = json.loads(json_data)
    firmware_ver = data.get("fw")
    measure_type = data.get("metric")
    measure_value = data.get("value")
    measure_unit = data.get("unit")

dev_name = "" 
passkey = ""
ip = ""
mqttc = mqtt.Client("Konker" + dev_name)
mqttc.username_pw_set(dev_name, passkey)
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.connect(ip, 1883)
mqttc.loop_start()


### End of MQTT code

current_milli_time = lambda: int(round(time.time() * 1000))

def TempMessage():
    temp=read_temp()
    (rc, mid) = mqttc.publish("pub/"+ dev_name +"/temperature", json.dumps({"ts": current_milli_time(), "metric": "temperature", "value": temp, "unit": "Celsius"}))    
    print(rc)
    t = threading.Timer(10.0, TempMessage)
    t.start()

