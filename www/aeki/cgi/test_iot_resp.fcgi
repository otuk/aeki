#!/usr/bin/env python

# 
#  This is  a for TEST ONLY cgi, it simulates a iot device/response
#  it assukmes iot device controls 2 relays
#  it similates status response that shd come from a iot device
#  it reacts to set commands and changes relay status
#  RT_TEST_UPDATE call results in in this
#  TEST device to register itself with iot console with an address/ip
#
#  the FCGI is complaint with  flup version for python 2.x
#  flup version: flup 1.0.3.dev-20110405


from cgi import escape
import sys, os
from flup.server.fcgi import WSGIServer
import json
import requests
import urlparse
import logging
from aeki_config import aeki_config as ac # configuration only


IOT_HOST = ac["IOT_HOST"]
IOT_SCRIPTS =  "/aeki/cgi/"
IOT_SCRIPT_MAIN = "iot.fcgi"
IOT_SCRIPT_TEST = "test_iot_resp.fcgi"
RT_TEST_UPDATE = "testupdate"
TEST_SERVICE = "LR01"
TEST_IP =  IOT_HOST + IOT_SCRIPTS + IOT_SCRIPT_TEST


IOT_PROTOCOL = "http://"
IOT_STATUS_SERVICE = "iotstatus"
IOT_CONTROL_SERVICE = "iotset" 
RT_UPDATE = "update"
LOG_FILENAME  = "log.iot"
LOG_LEVEL = logging.DEBUG


g_relay1 = 1
g_relay2 = 0 

logging.basicConfig(filename = LOG_FILENAME, level= LOG_LEVEL)

def toggle(no):
    global g_relay1
    global g_relay2
    no = int(no)
    if no == 1:
        g_relay1 = 0 if g_relay1==1 else  1
    elif no == 2:
        g_relay2 = 0 if g_relay2==1 else  1



def handleTestUpdate(environ, start_response):
    start_response('200 OK', [('Content-Type', 'application/json')])
    logging.debug(" TEST: "+IOT_PROTOCOL + IOT_HOST + IOT_SCRIPTS + IOT_SCRIPT_MAIN+"/"+RT_UPDATE+"?serviceName="+TEST_SERVICE+"&ip="+TEST_IP)
    r = requests.get(IOT_PROTOCOL + IOT_HOST + IOT_SCRIPTS + IOT_SCRIPT_MAIN+"/"+RT_UPDATE+"?serviceName="+TEST_SERVICE+"&ip="+TEST_IP)
    logging.debug(str(r))
    yield json.dumps(r.json())



def handleStatus(environ, start_response):
    start_response('200 OK', [('Content-Type', 'application/json')])
    d ={"relay1": g_relay1, "relay2": g_relay2}
    yield json.dumps(d)


def handleControl(environ, start_response):
    start_response('200 OK', [('Content-Type', 'application/json')])
    d = urlparse.parse_qs(environ["QUERY_STRING"])    
    no = d["no"][0]
    toggle(no)
    d ={"relay1": g_relay1, "relay2": g_relay2}
    yield json.dumps(d)



def handleNotFound(environ, start_response):
    start_response('404 Notfound', [('Content-Type', 'application/json')])
    r = {"error": 1, "errormsg": "TEST_IOT_RESP Route Not Found"}
    yield json.dumps(r)


def app(environ, start_response):
    route = escape(environ.get("PATH_INFO","")).strip("/")
    logging.debug("TEST serving: "+route)
    if route == IOT_STATUS_SERVICE :
        return handleStatus(environ, start_response)
    elif route == IOT_CONTROL_SERVICE:
        return handleControl(environ, start_response)
    elif route == RT_TEST_UPDATE:
        return handleTestUpdate(environ, start_response)
    else:
        return handleNotFound(environ, start_response)

    
logging.debug("* TEST simulation starting *")    
WSGIServer(app).run()

