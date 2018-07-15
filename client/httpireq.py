import os
import json
import tempfile
import sys
import datetime
import time
import binascii
#import requests
import hashlib
import ssl
import gnupg
import base64

from urllib.parse import urlparse
import urllib.request
from urllib.error import HTTPError

def serialise_headers(items):
	d = dict()
	for k,v in items:
		d[k] = v
	return d

def serialise_request(req):
	ser = dict()
	ser["method"] = req.get_method()
	ser["type"] = req.type
	ser["host"] = req.host
	ser["uri"] = req.selector
	ser["headers"] = serialise_headers(req.header_items())
	ser["data"] = req.data	

	return ser

def help():
	print("Usage: %s [<key id>] <url>" % sys.argv[0])

def real_url(url):
	if(url.find("?")>-1):
		return real_url(url[0:url.index("?")])+url[url.index("?"):]
	else:
		return url[0:-6]

def parse_url(url):
	if url.startswith("httpi://"):
		if(url.find("?")>-1):
			#handle query string
			inde = url.index("?")
			qs = url[inde:]
			ur = url[0:inde]
			return "https://%s.ident%s" % (ur[8:], qs)
		return "https://%s.ident" % url[8:]
	elif url.startswith("https://") and url.endswith(".ident"):
		return url
	elif url.startswith("https://") and url.find("?")>-1:
		if(url[0:url.index("?")].endswith(".ident")):
			return url
		else:
			return None
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

sys.stdout.write("Requesting token from URL %s... " % url.geturl())

surl =None
token= None

try:
	data = urllib.request.urlopen(url.geturl())
	decd = json.loads(data.read())

	print("OK");

	recv = urlparse(url.scheme+"://"+url.netloc+decd["for"])

	url = urlparse(real_url(url.geturl()))

	if recv.geturl() != url.geturl():
		print("WARNING: Authorised URL is not as expected (got %s, expected %s)" % (recv.geturl(), url.geturl()))

	surl = recv
	token = decd

except HTTPError as e:
	print("Error requesting token: %s" % e)
	exit()	

print("Token received for %s" % token["who"])
if (token["until"]!=None):
	print("Token is valid until %s (UTC)" % token["until"])

request = urllib.request.Request(recv.geturl())
request.add_header("Connection", "close")
request.add_header("X-ident-token", token["token"])
request.add_header("User-agent", "HTTPIReq 0.1")
request.add_header("Host", recv.netloc)
request.add_header("Accept-encoding", "identity")

ser = serialise_request(request)

print("You may now be asked for your password to sign the request.")

gpg = gnupg.GPG()
sdump = json.dumps(ser, sort_keys=True)
sig = gpg.sign(sdump, binary=True, detach=True, keyid=key)

if(not sig):
	exit()

request.add_header("X-ident-signature", base64.b64encode(sig.data))

sys.stdout.write("Attempting to use token %s to connect to %s... \n" % (token["token"], token["for"]))

try:
	resp = urllib.request.urlopen(request)
	print(resp.read().decode())
except HTTPError as e:
	print(e)
	exit()
