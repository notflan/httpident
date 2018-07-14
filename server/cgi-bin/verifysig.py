import os
import json
import tempfile
import sys
import datetime
import time
import binascii
import gnupg
import base64

def reserialise_headers(items):
	d = dict()
	for k,v in sorted(items, key=lambda tup: tup[0]):
		d[k] = v
	return d

def reserialise_request(req):
	ser = dict()
	ser["method"] = req["method"]
	ser["type"] = req["type"]
	ser["host"] = req["host"]
	ser["uri"] = req["uri"]
	ser["headers"] = reserialise_headers(req["headers"].items())
	ser["data"] = req["data"]	

	return dict(sorted(ser.items(), key=lambda tup: tup[0]))

sd = json.loads(base64.b64decode(sys.argv[1]))

ld = reserialise_request(sd)

js = json.dumps(ld, sort_keys=True)

sys.stdout.write(js)
sys.stdout.flush()
