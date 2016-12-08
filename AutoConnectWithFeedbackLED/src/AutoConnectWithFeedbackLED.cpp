#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

//for LED id
#include "BlinkerID.h"

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  //ticker.attach(0.2, tick);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //set led pin as output
  pinMode(_STATUS_LED, OUTPUT);



  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);


  Serial.println("scan start");
  String ssidName="ESP";
  int ESPMaxnum=0;

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  delay(2000);

  if (n == 0)
    Serial.println("no networks found");
  else
  {
    String ssidTempName=ssidName;
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i){
      String stringOne = WiFi.SSID(i).c_str();
      int foundIndex = stringOne.indexOf(ssidName);
      if (foundIndex==0){
        int ESPnum=WiFi.SSID(i).substring(ssidTempName.length()+1, WiFi.SSID(i).length()).toInt();
        if(ESPMaxnum<ESPnum){
          ESPMaxnum=ESPnum;
        }
      }
      delay(10);
    }

    ssidName=ssidName +"_"+(ESPMaxnum+1);
  }

  // startBlickID
  startBlinkID(ESPMaxnum+1);


  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(ssidName.c_str())) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  startBlinkID(0);// pass zero to stop
  //keep LED on
  digitalWrite(_STATUS_LED, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:

}
