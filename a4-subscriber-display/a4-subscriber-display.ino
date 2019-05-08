/*  Leana Nakkour
 *  HCDE 440: Midterm Mini Project
 *  May 7, 2019
 * 
 *  Subscriber code; Sunset API and Light Sensor (Photocell) communicate to MQTT server
 *  that the LED Display Subscribes to.
 */ 

// Recieving  messages are in the JSON format. 

//Including libraries for Photocell, JSON and MQTT
#include <Wire.h>            // Necessary Libraries for Light sensor and MQTT server
#include <SPI.h>             //
#include <PubSubClient.h>    //
#include <ArduinoJson.h>     //
#include <ESP8266WiFi.h>     //
#include <Adafruit_Sensor.h> //

#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

////// Wifi /////
#define wifi_ssid "University of Washington"  
#define wifi_password "" 

WiFiClient espClient;             // espClient
PubSubClient mqtt(espClient);     // tie PubSub (mqtt) client to WiFi client

char mac[6]; //A MAC address as the unique user ID!

char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array

char espUUID[8] = "ESP8602"; // Name of the microcontroller

int led = 2;           // the PWM pin the LED is attached to
int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by

String light_status;

void setup() {
  // Start the serial connection
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  // System status
  while(! Serial);
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));
  
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883); // Start the mqtt
  mqtt.setCallback(callback); //Register the callback function
  
}

/////SETUP_WIFI/////
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}       



void loop() {
  if (!mqtt.connected()) {  // Try connecting again
    reconnect();
  }
  mqtt.loop(); //this keeps the mqtt connection 'active'
  // Serial.println("Daylight Status: " + light_status);

  // Receives the message from the Light Sensor via the MQTT server
  // Reads the light status and if it is "Dim" (low lighting registered)
  // then the LEDs will begin to fade
  if (light_status == "Dim") {
       analogWrite(led, brightness);

       // change the brightness for next time through the loop:
       brightness = brightness + fadeAmount;

        // reverse the direction of the fading at the ends of the fade:
       if (brightness <= 0 || brightness >= 255) {
          fadeAmount = -fadeAmount;
       }
       
       // wait for 30 milliseconds to see the dimming effect
        delay(30);
  } else {
      analogWrite(led, LOW);    // Turns off LED/Keeps it off
  }
  
}


//function to reconnect if we become disconnected from the server
void reconnect() {

  // Loop until we're reconnected
  while (!espClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // Attempt to connect
    if (mqtt.connect(espUUID, mqtt_user, mqtt_password)) { //the connction
      Serial.println("connected");
      // Once connected, publish an announcement...
      char announce[40];
      strcat(announce, espUUID);
      strcat(announce, "is connecting. <<<<<<<<<<<");
      mqtt.publish(espUUID, announce);
      // ... and resubscribe
      //      client.subscribe("");
      mqtt.subscribe("fromDaylight/sunstatus");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
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

  // Receives the light sensor data and stores it in a variable light_status
  if (String(topic) == "fromDaylight/sunstatus") {
    Serial.println("printing message");
    Serial.print("Message arrived in topic: ");
    String daylight_status = root["Daylight Status"];
    // debug
    // Serial.println(daylight_status);
    light_status = String(daylight_status);

 }

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }
}
