import os
import json
import tempfile
import sys
import datetime
import time
import binascii
import requests
import ssl
import gnupg
import base64

from urllib.parse import urlparse
import urllib.request
from urllib.error import HTTPError

def serialise_headers(items):
	d = dict()
	for k,v in sorted(items, key=lambda tup: tup[0]):
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

	return dict(sorted(ser.items(), key=lambda tup: tup[0]))

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

sys.stdout.write("Requesting token from URL %s... " % url.geturl())

surl =None
token= None

try:
	data = urllib.request.urlopen(url.geturl())
	decd = json.loads(data.read())

	print("OK");

	recv = urlparse(url.scheme+"://"+url.netloc+decd["for"])

	url = urlparse(url.geturl()[0:-6])

	if recv.geturl() != url.geturl():
		print("WARNING: Authorised URL is not as expected (got %s, expected %s)" % (recv.geturl(), url.geturl()))

	surl = recv
	token = decd

except HTTPError as e:
	print("Error requesting token: %s" % e)
	exit()	

request = urllib.request.Request(recv.geturl())
request.add_header("Connection", "close")
request.add_header("X-ident-token", token["token"])
request.add_header("User-agent", "HTTPIReq 0.1")
request.add_header("Host", recv.netloc)
request.add_header("Accept-encoding", "identity")

ser = serialise_request(request)

print("You may now be asked for your password to sign the request.")

gpg = gnupg.GPG(gnupghome="~/.gnupg")
print(json.dumps(ser))
sig = gpg.sign(json.dumps(ser), binary=True, detach=True, keyid=key)

request.add_header("X-Ident-Signature", base64.b64encode(str(sig).encode()))

sys.stdout.write("Attempting to use token %s to connect to %s... \n" % (token["token"], token["for"]))

try:
	resp = urllib.request.urlopen(request)
	print(resp.read().decode())
except HTTPError as e:
	print(e)
	exit()
