import os
import json
import tempfile
import sys
import datetime
import time
import binascii
import requests
import ssl

from urllib.parse import urlparse
import urllib.request
from urllib.error import HTTPError

def help():
	print("Usage: %s [<key id>] <url>" % sys.argv[0])

def parse_url(url):
	if url.startswith("httpi://"):
		return "https://%s.ident" % url[8:]
	elif url.startswith("https://") and url.endswith(".ident"):
		return url
	else:
		return None

if len(sys.argv)<2:
	help()
	exit()

url = sys.argv[-1]
key= None
if len(sys.argv) >2:
	key = sys.argv[-2]

url = parse_url(url)

if url == None:
	print("Invalid URL")
	exit()

url = urlparse(url)

try:
	data = urllib.request.urlopen(url.geturl())

	print(data)
except HTTPError as e:
	print("Error requesting token: %s" % e)
	

