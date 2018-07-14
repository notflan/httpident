import os
import json
import tempfile
import sys
import datetime
import time
import binascii
import gnupg
import hashlib
import base64

try:

	js = json.dumps(json.loads(base64.b64decode(sys.argv[1])), sort_keys=True) #reformat json
	sigfile = sys.argv[2]

	gpg = gnupg.GPG(gnupghome=sys.argv[3])

	verified = gpg.verify_data(sigfile, js.encode())

	if not verified:
		print(json.dumps(None))
	else:
		ret = dict()
		ret["username"] = verified.username
		ret["key_id"] = verified.key_id
		ret["signature_id"] = verified.signature_id
		ret["fingerprint"] = verified.fingerprint
		ret["trust_level"] = verified.trust_level
		ret["trust_text"] = verified.trust_text
		print(json.dumps(ret))
except:
	print(json.dumps(None))
