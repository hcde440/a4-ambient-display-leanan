/*  Leana Nakkour
 *  HCDE 440: Midterm Mini Project
 *  May 7, 2019
 * 
 *  Publisher code; Sunset API and Light Sensor (Photocell) communicate to MQTT server
 *  that the LED Display Subscribes to.
 */ 


#include <stdlib.h>
#include <stdio.h>
#include <ESP8266WiFi.h>        //Requisite Libraries . . .
#include <ESP8266HTTPClient.h>  //
#include "Wire.h"               //
#include <PubSubClient.h>       //
#include <ArduinoJson.h>        //

////// Wifi /////
#define wifi_ssid "APT301"  
#define wifi_password "dslrapt301" 

///// MQTT server /////
#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

WiFiClient espClient;                     //blah blah blah, espClient
PubSubClient mqtt(espClient);             //blah blah blah, tie PubSub (mqtt)

char mac[6];                              //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

typedef struct { 
  String sunset;  // Variable to store Activity name as a string    
  String daylngth;  // Variable to store Accessibility as a string
} DaylightData;

DaylightData dd;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);  //register the callback function

  getSunset();
  Serial.println("Sunset will be at " + dd.sunset);
  // Serial.println("The total number of daylight hours will be " + dd.daylngth);


}

/////SETUP_WIFI/////
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
//  Serial.print("Connecting to ");
//  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
//  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}                                     //84:F3:EB:1A:45:EE


/////CONNECT/RECONNECT/////Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("fromDaylight/+"); // fromDaylight (sensors) to Bookworm (LED display) we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void loop() {
  // put your main code here, to run repeatedly:
  
  if (!mqtt.connected()) {
    reconnect();
  }

  mqtt.loop(); //this keeps the mqtt connection 'active'

  // how to PUBLISH
  // Publish message to the  topic 
  
  // EDIT MESSAGE BELOW TO DISPLAY HOW MUCH DAYLIGHT LEFT
  // sprintf(message, "{\"Temperature\":\"%s\", \"Humidity\":\"%s\", \"Pressure\":\"%s\"}", str_temp, str_hum, pressure); //JSON format using {"XX":"XX"}
  // mqtt.publish("fromDaylight/hrsdl", message);
  
}

//// USING SUNSET API TO GET DAYLIGHT HOURS LEFT ////
void getSunset(){
  HTTPClient theClient;
  Serial.println("Making HTTP request");
  // Seattle, WA Lon =  -122.3320708, Lat = 47.6062095
  theClient.begin("http://api.sunrise-sunset.org/json?lat=47.6062095&lng=-122.3320708&date=2019-05-07"); 
  int httpCode = theClient.GET();

  String sunsetTime = "";
  String sunsetToFloat = "";
  String dayhrs = "";

/// CODE DOESNT SEEM TO BE GOING INTO THIS, WHY?
  if (httpCode > 0) {
    if (httpCode == 200) {
      Serial.println("Received HTTP payload.");
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload);
      Serial.println();

      // Test if parsing succeeds.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        Serial.println();
        return;
      } 

      //Some debugging lines below:
      //Serial.println(payload);
      //root.printTo(Serial);

      dd.sunset = root["results"]["sunset"].as<String>();   // Get's time of Sunset & stores it in variable
      Serial.println(dd.sunset);
      sunsetTime = dd.sunset;
      //dd.daylngth = root["results"]["day_length"].as<String>();
      //dayhrs = dd.daylngth;
      //Serial.println(dd.daylngth);
      
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }

  // CONVERTING TIME OF SUNSET TO AN INTEGER/FLOAT
      Serial.println("-------CONVERT SUNSET TIME TO A DECIMAL HERE -----------");
      Serial.println("FROM THE ROOT CALL, dd.sunset = " + dd.sunset);              // prints sunset time
      sunsetToFloat = sunsetTime;             // duplicates the string
      sunsetToFloat.replace(":", ".");        // replaces : with . to convert to a decimal
      // ex. 3.38.35
      sunsetToFloat.remove(4);  
      Serial.println("Replacing : with . " + sunsetToFloat);          // prints new time with . instead of :
      double sunsetTIME;  
      sunsetTIME = atof(sunsetToFloat); // Converts string to a Float to use in calculations
      Serial.println("sunsetTime = " + sunsetTime);
     
      Serial.println("--------------------------------------------------------");
      Serial.println("");
      
     // float daylightLEFT = ((sunsetToFloat - (sunsetToFloat-1)) * 100) / 60;
     // Serial.println("DAYLIGHT LEFT = " + String(daylightLEFT) + " hours");
  }

  /*  Write code to parse sunset string and convert
   *   1 = 13, 2 = 14, 3 = 15, 4 = 16, 5 = 17, 6 = 18, 7 = 19, 8 = 20, 9 = 21
   *   10 = 22, 11 = 23, 12 = 24
   *    
   *   Calculate daylight left by taking sunset time (ex. 16:30) - current time (15:45),
   *   multiply by 100 to get whole number and divide by 60 to get total hours left of 
   *   daylight.
   *   
   *   Solution: if I can't find specific function to get current time of day, I'll use
   *   a time that is 1 hour before sunset.
   *   
   */
  
}
    

/////CALLBACK/////
//The callback is where we attach a listener to the incoming messages from the server.
//By subscribing to a specific channel or topic, we can listen to those topics we wish to hear.
//We place the callback in a separate tab so we can edit it easier . . . (will not appear in separate
//tab on github!)
/////

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }

  /////
  //We can use strcmp() -- string compare -- to check the incoming topic in case we need to do something
  //special based upon the incoming topic, like move a servo or turn on a light . . .
  //strcmp(firstString, secondString) == 0 <-- '0' means NO differences, they are ==
  /////

  if (strcmp(topic, "fromDaylight/LBIL") == 0) {
    Serial.println("A message from Batman . . .");
  }

  else if (strcmp(topic, "fromDaylight/tempHum") == 0) {
    Serial.println("Some weather info has arrived . . .");
  }

  else if (strcmp(topic, "fromDaylight/switch") == 0) {
    Serial.println("The switch state is being reported . . .");
  }

  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}
