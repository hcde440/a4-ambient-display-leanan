/*  Leana Nakkour
 *  HCDE 440: Midterm Mini Project
 *  May 7, 2019
 * 
 *  Publisher code; Sunset API and Light Sensor (Photocell) communicate to MQTT server
 *  that the LED Display Subscribes to.
 */ 

#include <ESP8266WiFi.h>        //Requisite Libraries . . .
#include <ESP8266HTTPClient.h>  //
#include "Wire.h"               //
#include <PubSubClient.h>       //
#include <ArduinoJson.h>        //

////// Wifi /////
#define wifi_ssid "University of Washington"  
#define wifi_password "" 

///// MQTT server /////
#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

WiFiClient espClient;                     //blah blah blah, espClient
PubSubClient mqtt(espClient);             //blah blah blah, tie PubSub (mqtt)

char mac[6];                              //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
char message[201];                        //201, as last character in the array is the NULL character, denoting the end of the array

// Set Photocell pins
int photocellPin = A0;     // the cell and 10K pulldown are connected to a0
int photocellReading;     // the analog reading from the analog resistor divider
String daylightStatus;
String sunset;            // Variable to store Sunset Time as a string    

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);  //register the callback function
  
  getSunset();
  //Serial.println("Calling getSunset..............");

  Serial.println();
  Serial.println("+++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Sunset will be at " + sunset);
  Serial.println("+++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.println();
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
    Serial.println("Attempting MQTT connection...");
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

void loop(void) {
  photocellReading = analogRead(photocellPin);  // Reads the light sensor data and stores it (int)

  // Debugging
  //Serial.print("Analog reading = ");
  // Serial.print(photocellReading); // the raw analog reading
  
  // Conditional statements to interpret numerical data received from light sensor
  // A string interpretting the light status of the room (ex: "Dim") will be stored
  // in a variable daylightStatus to be published over the MQTT server
  if (photocellReading < 10) {
    daylightStatus = "Dark";
  } else if (photocellReading < 200) {
    daylightStatus = "Dim";
  } else if (photocellReading < 500) {
    daylightStatus = "Light";
  } else if (photocellReading < 800) {
    daylightStatus = "Bright";
  } else {
    daylightStatus = "Very Bright";
  }

  //Serial.print(" - " + daylightStatus);
  //Serial.println();
  // 1 second delay in between 
  delay(1000);

   if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop(); //this keeps the mqtt connection 'active'


  // Publish message to the tem/hum topic 

 
  sprintf(message, "{\"Sunset\":\"%s\",\"Daylight Status\":\"%s\"}", sunset.c_str(), daylightStatus.c_str()); //JSON format using {"XX":"XX"}  
  mqtt.publish("fromDaylight/sunstatus", message);
//  Serial.println("+++++++++++++++++++++++++++++++++++++++++++++++++");
//  Serial.println("PRINTING SUNSET HERE: " + sunset);
//  Serial.println("PRINTING DAYLIGHTSTATUS HERE: " + daylightStatus);
  
}

void getSunset(){
  // Sunset API that takes in Lon and Lat of an area and 
  // returns a JSON of what time the sun will set in that area
  
  HTTPClient theClient;
  Serial.println("Making HTTP request");
  // Seattle, WA Lon =  -122.335167, Lat = 47.608013
  theClient.begin("http://api.sunrise-sunset.org/json?lat=47.608013&lng=-122.335167&formatted=0");
  int httpCode = theClient.GET();

  String hr;        // string to store the hour of the day when sun sets
  String minute;    // string to store the minute of the day when sun sets

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

      sunset = root["results"]["sunset"].as<String>();   // Get's time of Sunset & stores it in variable
      // ex. 2015-05-21T19:22:59+00:00"
      sunset = sunset.substring(11, 16); ; // remove characters up to index 10 -> 19:22:59+00:00 (hh:mm:ss)
      
      // uses substring to get the hour and minute of sunset from the string returned
      hr = sunset.substring(0, 2);
      minute = sunset.substring(3);

      // API returnes the date and time in UTC, not PST. UTC is 7 hours ahead
      // so the following lines are to convert that time to PST
      int convertPST;
      convertPST = hr.toInt();
      //Serial.println("THIS IS THE CONVERSION " + convertPST);
      
      if (convertPST - 7 < 0) {
        convertPST = 12 + (convertPST - 7);
        
        // DEBUG 
        //Serial.println("PST hour is: " + convertPST);
        
        // stores sunset time in variable to be published on MQTT server
        sunset = String(convertPST) + ":" + minute + "PM";  
      } else {

        // stores sunset time in variable to be published on MQTT server
        sunset = hr + ":" + minute + "PM";
      }
      
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }  
}


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

  else if (strcmp(topic, "fromDaylight/sunstatus") == 0) {
    Serial.println("Some weather info has arrived . . .");
  }

  else if (strcmp(topic, "fromDaylight/switch") == 0) {
    Serial.println("The switch state is being reported . . .");
  }

  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}
