#!/usr/bin/env python
# 
#
#  This is the main iot console services CGI
#  it allows iot devices to register via RT_REGISTER
#  it fetches iot device status via RT_STATUS
#  it sends set commands to iot devices via RT_CONTROL
#
#  the FCGI is complaint with  flup version for python 2.x
#  flup version: flup 1.0.3.dev-20110405

import sys
from cgi import escape
from flup.server.fcgi import WSGIServer
import json
import requests
import urlparse
import logging
import dbm   # simplest database option, you can change this to sql or others
from aeki_config import aeki_config as ac # configuration only

AEKI_HOST = ac["AEKI_HOST"] # host name where this cgi is being executed 

IOT_PROTOCOL = "http://"
IOT_STATUS_SERVICE = "iotstatus"
IOT_CONTROL_SERVICE = "iotset" 

RT_STATUS = "status"
RT_CONTROL = "control"
RT_UPDATE = "update"
RT_REGISTER = "register"

ERR_STR_JS = "{'error':1, 'errormsg':'%s'}"

HDR_CT_JSON = ('Content-Type', 'application/json')

DB_FILENAME = "data.registrations" #chmod these files ReadWrite for cgi   
LOG_FILENAME  = "log.iot" #chmod these files ReadWrite for cgi   
LOG_LEVEL = logging.DEBUG #logging.WARNING

REQ_TIMEOUT = 3 # timeout if not connected to iot device in X seconds

g_services = {}


def handleNotFound(environ, start_response):
    logging.warning("** Handling not found")
    start_response('404 Notfound', [HDR_CT_JSON])
    r =  ERR_STR_JS % "Route Not Found"
    yield r


def handleUpdate(environ, start_response):
    d = urlparse.parse_qs(environ["QUERY_STRING"])
    if "serviceName" in d.keys():
        serviceName = d["serviceName"][0]
        ip = d["ip"][0]
        logging.debug("parsed ip and serv >"+serviceName+"<>"+ip+"<")
    else:
        start_response('500 Error', [HDR_CT_JSON])
        r =  ERR_STR_JS % "Format err: serviceName not sent"
        yield r
        return 
    updated = False
    if serviceName in g_services.keys():
        if ip != g_services[serviceName]:
            updateServiceInfo(serviceName, ip)
            updated = True
    else:
        updateServiceInfo(serviceName, ip)
        logging.info("Updated existing service with>"+serviceName+"<>"+ip+"<")
        updated = True
    r = {"updated":updated,"serviceName":serviceName, "ip":g_services[serviceName]}
    start_response('200 OK', [HDR_CT_JSON])
    yield json.dumps(r)


def handleStatus(environ, start_response):
    serviceName = parseServiceName(environ)
    ip = g_services[serviceName]
    logging.debug(IOT_PROTOCOL + ip +"/"+ IOT_STATUS_SERVICE)
    try:
        requrl = IOT_PROTOCOL + ip +"/"+ IOT_STATUS_SERVICE
        r = requests.get(requrl, timeout=REQ_TIMEOUT)
        start_response('200 OK', [HDR_CT_JSON])
        yield json.dumps(r.json())
    except:   
        start_response('500 connection error', [HDR_CT_JSON])
        yield  ERR_STR_JS % 'Problem connecting to iot device'


def handleControl(environ, start_response):
    d = urlparse.parse_qs(environ["QUERY_STRING"])    
    serviceName = d["serviceName"][0]
    no = d["no"][0]
    ip = g_services[serviceName]
    try:
        r = requests.get(IOT_PROTOCOL + ip +"/"+ IOT_CONTROL_SERVICE +\
                         "?no="+no, timeout=REQ_TIMEOUT)
        start_response('200 OK', [('Content-Type', 'application/json')])
        yield json.dumps(r.json())
    except:
        start_response('500 connection error', [('Content-Type', 'application/json')])
        yield "{'error':1, 'errormsg':'Problem connecting to iot device'}"


def appRouter(environ, start_response):
    route = escape(environ.get("PATH_INFO","")).strip("/")
    logging.debug("Serving >"+str(route)+"<")
    if route == RT_STATUS :
        return handleStatus(environ, start_response)
    elif route == RT_UPDATE:
        return handleUpdate(environ, start_response)
    elif route == RT_CONTROL:
        return handleControl(environ, start_response)
    else:
        return handleNotFound(environ, start_response)


"""
 the following function is for test only 
"""
def registerTest():
    db = dbm.open(DB_FILENAME, 'c')
    db["LR01"]= AEKI_HOST+"/aeki/cgi/test_iot_resp.fcgi"
    db.close();
    loadServiceInfo()
"""
  remove this this is test only
"""

def openDatabase():
    try:
        db = dbm.open(DB_FILENAME, 'c')
    except:
        logging.error("Cannot open database file "+DB_FILENAME+"- will abort")
        sys.exit("Exiting script iot.fcgi - cannot open database file")
    return db    

def updateServiceInfo(service, ip):
    db = openDatabase()
    db[service] = ip
    db.close()
    loadServiceInfo()

def loadServiceInfo():
    db = openDatabase()
    for k in db.keys():
        g_services[k] = db[k]
    db.close()    
    
def parseServiceName(environ):
    d = urlparse.parse_qs(environ["QUERY_STRING"])
    return d["serviceName"][0]


logging.basicConfig(filename = LOG_FILENAME, level= LOG_LEVEL)
logging.error("* IoT Console Services: Logging started *")

###
registerTest()  # !!!! # remove this if not test
###

loadServiceInfo()
logging.error("Starting iot console services")


WSGIServer(appRouter).run()
