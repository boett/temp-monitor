#!/usr/bin/python

# [Unit]
# Description=Python Temperature Monitor
# After=network.target

# [Service]
# Type=simple
# User=pi
# WorkingDirectory=/home/pi
# ExecStart=/home/pi/temps.py
# Restart=always
# StandardOutput=syslog

# [Install]
# WantedBy=multi-user.target


import time
import json
import urllib2

temp_sensor = '/sys/bus/w1/devices/28-0000087686c0/w1_slave'

def temp_raw():
    f = open(temp_sensor, 'r')
    lines = f.readlines()
    f.close()
    return lines

def read_temp():

    lines = temp_raw()
    while lines[0].strip()[-3:] != 'YES':
        time.sleep(0.2)
        lines = temp_raw()
    temp_output = lines[1].find('t=')

    if temp_output != -1:
        temp_string = lines[1].strip()[temp_output+2:]
        temp_c = float(temp_string) / 1000.0
        temp_f = temp_c * 9.0 / 5.0 + 32.0
        return temp_c, temp_f

while True:
    try:
        (c,f) = read_temp()
        data = {'temperatures': [f]}
        req = urllib2.Request('http://pwb.pythonanywhere.com/post_temps')
        req.add_header('Content-Type', 'application/json')
        r = urllib2.urlopen(req, json.dumps(data))
    except:
        e = sys.exc_info()[0]
        print "temp-monitor: ", e

    time.sleep(60)
