#!/usr/bin/python

import os
import time
import json
import urllib2
import sys
from pyHS100 import SmartPlug

log_fans = True
plug_ip = ["192.168.15.102"]
temp_dir = '/sys/bus/w1/devices/'

def chomp(x):
    if x.endswith("\r\n"): return x[:-2]
    if x.endswith("\n"): return x[:-1]
    return x

def get_slaves():
    sensor_array = []
    f = open('/sys/bus/w1/devices/w1_bus_master1/w1_master_slaves', 'r')
    for l in f.readlines():
	sensor_array.append(chomp(l))
    f.close()
    return sensor_array
	
def temp_raw(s):
    nm = temp_dir + s + '/w1_slave'
    f = open(nm, 'r')
    lines = f.readlines()
    f.close()
    return lines

def read_temp(s):

    lines = temp_raw(s)
    while lines[0].strip()[-3:] != 'YES':
        time.sleep(0.2)
        lines = temp_raw()
    temp_output = lines[1].find('t=')

    if temp_output != -1:
        temp_string = lines[1].strip()[temp_output+2:]
        temp_c = float(temp_string) / 1000.0
        temp_f = temp_c * 9.0 / 5.0 + 32.0
        return temp_c, temp_f

sensor_array = get_slaves()
while True:
    try:
	d = []
	for s in sensor_array:
       	    (c,f) = read_temp(s)
            d.append({'sn': s, 'temp': f})

	plug = [None,None]
	p_state = ['not found', 'not found']
	if log_fans:	
	    for f in [0,1]:
	        try:
    	            plug[f] = SmartPlug(plug_ip[f])
		    p_state[f] = plug[f].state
		except:
		    pass
    	    data = {'temperatures': d, 'fans': p_state}
	else:
    	    data = {'temperatures': d}

    	req = urllib2.Request('http://pwb.pythonanywhere.com/post_temps')
    	req.add_header('Content-Type', 'application/json')
    	r = urllib2.urlopen(req, json.dumps(data))
	data = json.load(r)

	if log_fans:
	    for f in [0,1]:
		try:
	       	    if data['fanstate'][f] == 1:
	            	plug[f].turn_on()
		    elif data['fanstate'][f] == 0:
	    	        plug[f].turn_off()
		except:
		    pass

    except Exception as e:
	print "temp-monitor: ", e

    time.sleep(60)
