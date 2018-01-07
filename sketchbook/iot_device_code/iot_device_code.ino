#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>  


/* Set these to your desired credentials for AP MODE, 
you can also run APmode without password for open  access*/
const char *APSSID{"IOT_by_OTUK"};
const char *APPASS{"password"};

const int SSIDSIZE{32};
const int PASSSIZE{128};
const int SERVICESIZE{8};
const int MAINURLSIZE{128};
const int SERVER_PORT{80};
const int RELAY1{1};
const int RELAY2{2};
const int PINRELAY_1{12};
const int PINRELAY_2{13};
const int EEADDRESS{0};
const unsigned char WIFI_START_MARKER{132};
const unsigned char WIFI_END_MARKER{231};
const int CONN_TRY{30};
const int CONN_WAIT{500};

ESP8266WebServer iotserver{SERVER_PORT};

bool APMODE{false};
int rel1state{0};
int rel2state{0};

typedef struct {
  unsigned char w_start{0};
  char ssid[SSIDSIZE]{0};
  char pass[PASSSIZE]{0};
  char servicename[SERVICESIZE]{0};
  char mainurl[MAINURLSIZE]{0};
  unsigned char w_end{0}; 
} wifiinfo_s;

wifiinfo_s wi;

#define H(h) html += #h
#define V(h) html += h


/*  In AP mode  go to http://192.168.4.1 in a web browser
 *  connected to this access point to see it.
 */

void handleUpdateWifi();

bool retrieveSsidPassword(){
  //TODO
  Serial.println("Retrieving ssid & password");
  EEPROM.get(EEADDRESS, wi);
  Serial.print("Retrieved ssid:"); Serial.println(wi.ssid);
  Serial.print("Retrieved pass:"); Serial.println(wi.pass);
  Serial.print("Retrieved servicename:"); Serial.println(wi.servicename);
  Serial.print("Retrieved mainurl:"); Serial.println(wi.mainurl);
  if(wi.w_start != WIFI_START_MARKER || wi.w_end != WIFI_END_MARKER){
    Serial.println("wifi info markers do not match");
    return false;
  }  
  return true;
}

bool saveSsidPassword(String ssid, String password,
		      String servicename, String mainurl){
  //TODO
  if(ssid.length()>0){
    Serial.println("Saving ssid & password");
    strncpy(wi.ssid, ssid.c_str(), SSIDSIZE-1);   
    strncpy(wi.pass, password.c_str(), PASSSIZE-1);
    strncpy(wi.servicename, servicename.c_str(), SERVICESIZE-1);
    strncpy(wi.mainurl, mainurl.c_str(), MAINURLSIZE-1);
    Serial.print("Saved ssid:"); Serial.println(wi.ssid);
    Serial.print("Saved pass:"); Serial.println(wi.pass);
    Serial.print("Save servicename:"); Serial.println(wi.servicename);
    Serial.print("Saved mainurl:"); Serial.println(wi.mainurl);
    wi.w_start = WIFI_START_MARKER;
    wi.w_end = WIFI_END_MARKER;
    EEPROM.put(EEADDRESS, wi);
    EEPROM.end();
    return true;
  }else 
    return false;
}


bool handleEraseWifi(){
    Serial.println("Erasing Wifi data");
    wifiinfo_s emptydata;
    EEPROM.put(EEADDRESS, emptydata);
    EEPROM.end();
    abort();
}


int toggle(int& binary){
  binary =  binary & 1 ?  0 : 1;
  return binary;
}

void handleRelay() {
  if(APMODE){
    handleUpdateWifi();
  }      
  Serial.println("Handling relay request");
  String pno;
  if(iotserver.hasArg("no")){
    Serial.println("Has no");
    pno = iotserver.arg("no");
    Serial.println(pno);
    int out = pno.toInt();
    Serial.println(out);
    if(out==RELAY1)  
      digitalWrite(PINRELAY_1, toggle(rel1state));
    else if (out==RELAY2)  
      digitalWrite(PINRELAY_2, toggle(rel2state));
  }  
  String html;
  H(<!DOCTYPE html>);
  H(<html>);
  H(<head>);
  H(<title> Relay status</title>);
  H(<style>body{background:lightgreen; font-family: sans serif }</style>);
  H(</head>);
  H(<body>);
  H(Relay 1 is :);
  rel1state ? V("ON") : V("OFF");
  H(<br>);
  H(Relay 2 is :);
  rel2state ? V("ON") : V("OFF");
  H(<br>);
  H(</body>);
  H(</html>);
  iotserver.send(200, "text/html", html);

}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += iotserver.uri();
  message += "\nMethod: ";
  message += (iotserver.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += iotserver.args();
  message += "\n";
  for (uint8_t i=0; i<iotserver.args(); i++){
    message += " " + iotserver.argName(i) + ": " + iotserver.arg(i) + "\n";
  }
  iotserver.send(404, "text/plain", message);
}

 
void handleUpdateWifi() {
  String pssid, ppass, pservicename, pmainurl, errormsg, warningmsg;
  if(!APMODE){
    Serial.println("Trying to change wifi while connected");
    warningmsg = "WARNING: ";
    if (WiFi.isConnected()){
      Serial.println("connected");
      warningmsg += "You seemed to be currently connected to a wifi network";
      warningmsg += "<br>This IoT device is accesible at IP address ";//+WiFi.localIP();
      warningmsg += "<br>on the network with ssid: ";
      warningmsg +=  WiFi.SSID();
    }
  }
  Serial.println(warningmsg);
  Serial.println("Handling update ssid screen");
  Serial.println(iotserver.method());
  if (iotserver.method()==HTTP_POST){
    if(iotserver.hasArg("ssid")){
      Serial.println("Has ssid");
      pssid = iotserver.arg("ssid");
      Serial.println(pssid); 
    }
    if(iotserver.hasArg("pass")){
      Serial.println("Has pass");
      ppass = iotserver.arg("pass");
      Serial.println(ppass); 
    }
    if(iotserver.hasArg("servicename")){
      Serial.println("Has servicename");
      pservicename = iotserver.arg("servicename");
      Serial.println(pservicename); 
    }
    if(iotserver.hasArg("mainurl")){
      Serial.println("Has mainurl");
      pmainurl = iotserver.arg("mainurl");
      Serial.println(pmainurl); 
    }
    if(!saveSsidPassword(pssid, ppass, pservicename, pmainurl)){
      Serial.println("cannot save the ssid/pass");
      errormsg = "Problem saving ssid/password, please re-try";
      APMODE = true;
    }else{
      Serial.println("ssid/pass saved, setting AP mode off");
      APMODE = false;
    }
  }
  String html;
  H(<!DOCTYPE html>);
  H(<html>);
  H(<head>);
  H(<title> WIFI ssid and Password Settings</title>);
  H(<style>body{background:lightgrey; font-family: sans serif }</style>);
  H(</head>);
  H(<body>);
  V(warningmsg);
  H(<br>);  
  H(<form action='' method='post'>);
  H(<br>);
  H(Enter your ssid and password for this IOT device to use for connections);
  H(<br>);
  H(<label for='ssid'>SSID:</label><input id='ssid' name='ssid' type='text'>);
  V(pssid);
  H(<br>);
  H(<label for='pass'>Password:</label><input id='pass' name='pass' type='password'><br>);
  H(<label for='servicename'>ServiceName:</label><input id='servicename' name='servicename' type='text'><br>);
  H(<label for='mainurl'>MainURL:</label><input id='mainurl' name='mainurl' type='text'><br>);
  H(<input id='sbutton' type='submit'><br>);
  H(</form>);
  if(errormsg.length() == 0)
    H(Will reset and try to connect to wifi at );
  else
    V(errormsg);
  V(pssid);
  H(</body>);
  H(</html>);
  iotserver.send(200, "text/html", html);
}



void setupAP() {
  Serial.println();
  Serial.print("Configuring AP - access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(APSSID, APPASS);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void updateMain(){
  // make a request to the mainurl to update the IP for this servicename
  HTTPClient hclient;
  String updateurl{wi.mainurl};
  updateurl += "/update/?serviceName=";
  updateurl += wi.servicename;
  updateurl += "&ip=";
  updateurl += WiFi.localIP().toString();  
  // Attempt to make a connection to the remote server
  hclient.begin(updateurl);
  // start connection and send HTTP header
  // Make an HTTP GET request
  int httpCode = hclient.GET();
  // httpCode will be negative on error
  if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      String payload = hclient.getString();
      Serial.println(payload);
    }
  }else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", hclient.errorToString(httpCode).c_str());
  }
  hclient.end();
}


bool connectToWiFi(){
  Serial.println("Will try to connect to wifi");
  Serial.println("before test retrieve");
  Serial.println(wi.ssid);
  Serial.println(wi.pass);
  retrieveSsidPassword();  // for test
  Serial.println("AFTER TEST RETRIEVE");
  Serial.println(wi.ssid);
  Serial.println(wi.pass);
  WiFi.begin(wi.ssid, wi.pass);
  Serial.println("");
  int counter =0;
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    if (counter++ > CONN_TRY){// wait ~30 secs for connection
        Serial.print("Cannot connect to wifi ");
	Serial.print(wi.ssid);
	Serial.print("/");
	Serial.print(wi.pass);
	Serial.println(" with pass ");
        return false;
    }    
    delay(CONN_WAIT);
    Serial.print("*");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(wi.ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  updateMain();
  return true;
}



void setupConnection(){
  if(retrieveSsidPassword()){
    Serial.println("Retrieved ssid password ");
    if(connectToWiFi()){
        Serial.println("Connected to wifi with retrieved password");
	APMODE = false;
	return;
    }else{
      Serial.println("COULD NOT connect to wifi");
      Serial.println(wi.ssid);
      Serial.println(wi.pass);
    }
  }else{
    Serial.println("Cannot retrieve  password ");
    Serial.println(wi.ssid);
    Serial.println(wi.pass);
  }
  APMODE = true;  
  setupAP();
}

void  startWebserver(ESP8266WebServer& wserver){
  wserver.onNotFound(handleNotFound);
  wserver.on("/wifi", handleUpdateWifi);   
  wserver.begin();
  Serial.println("HTTP server started for AP");
  while(APMODE || !connectToWiFi() ){
    wserver.handleClient();
  }
  Serial.println("Stopped for AP mode");
}

void handleStatus(){
  String html ="{";
  html += "\"relay1\" : ";
  html += String(rel1state);
  html += ",";
  html += "\"relay2\" : ";
  html += String(rel2state);
  html += "}";  
  iotserver.send(200, "application/json", html);
}

void handleSet(){
  String pno;
  if(iotserver.hasArg("no")){
    Serial.println("Has no");
    pno = iotserver.arg("no");
    Serial.println(pno);
    int out = pno.toInt();
    Serial.println(out);
    if(out==RELAY1)  
      digitalWrite(PINRELAY_1, toggle(rel1state));
    else if (out==RELAY2)  
      digitalWrite(PINRELAY_2, toggle(rel2state));
  }  
  handleStatus();
}

void setup(){
  delay(10000);
  Serial.begin(115200);
  EEPROM.begin(512);
  WiFi.disconnect(true);
  setupConnection();
  if(APMODE){
    // start webservice to collect wifi and mainurl information
    startWebserver(iotserver);
    iotserver.stop();
  }  
  // add program specific routes and seetings
  pinMode(PINRELAY_1, OUTPUT);
  pinMode(PINRELAY_2, OUTPUT);
  iotserver.on("/", handleRelay);  //test only
  iotserver.on("/relay", handleRelay);//test only
  iotserver.on("/eraseWifi",handleEraseWifi); 
  iotserver.on("/iotstatus",handleStatus); 
  iotserver.on("/iotset",handleSet);
  iotserver.begin();
  Serial.println("HTTP server started for IOT service");
}

void loop() {
  iotserver.handleClient();
}
