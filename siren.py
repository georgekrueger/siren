import httplib
from datetime import datetime

def load_instrument(track_num, instrument):
	pass

def load_pattern(track_num, pattern):
	pass

# play pattern on given track
def play(track_num):
	pass

# stop track if playing
def stop(track_num):
	pass

# def _send_py_to_client(py_code):
# 	print "%s" % (datetime.now())
# 	conn = httplib.HTTPConnection("127.0.0.1", 80)
# 	conn.request("GET", "/", py_code)
# 	r1 = conn.getresponse()
# 	data1 = r1.read()
# 	print "%s %s %s '%s'" % (datetime.now(), r1.status, r1.reason, data1)

