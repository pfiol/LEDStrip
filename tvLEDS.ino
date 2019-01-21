#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>

// WIFI SETUP
const char *ssid     = "";
const char *password = "";

int redPin = 13;
int whitePin = 14;
int bluePin = 12;
int greenPin = 15;

int transition = 2;
int custom_delay = 10; // DEfault delay

typedef struct {
  int red;
  int green;
  int blue;
  int white;
}  rgbLEDs;

rgbLEDs myLeds = {0,0,0,0};
rgbLEDs myLedsG = {0,0,0,0};

// MQTT vars
struct mqtt_config {
  String server;  // IP address of the broker
  String topic;   // Topic to read from
  String id;      // Our unique mqtt device ID
  String status;  // Current status
};

// Enter your MQTT server IP address as the first argument 
mqtt_config mqtt = {"", "Home/tvLEDS", "tvLEDS", "OFF"};


WiFiClient net;
MQTTClient client;

// Firmware update setup
// Path to access firmware update page (Not Neccessary to change)
const char* update_path = "/firmware";
// Username to access the web update page
const char* update_username = "admin";
// Password to access the web update page
const char* update_password = "admin";

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

void setup() {
  Serial.begin(115200);
  
  // Initialize RGB Led strip pins
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(whitePin, OUTPUT);

  Serial.println(myLeds.red);
  connect();
  
  server.on("/setLEDS", []() {
    myLedsG.red = map(server.arg("r").toInt(),0,255,0,1023);
    myLedsG.green = map(server.arg("g").toInt(),0,255,0,1023);
    myLedsG.blue = map(server.arg("b").toInt(),0,255,0,1023);
    myLedsG.white = map(server.arg("w").toInt(),0,255,0,1023);
    if(server.arg("t") != ""){
      transition = server.arg("t").toInt();
    }
    setLEDs();
    
    server.send(200, "text/plain", "Done!");
  });

  httpUpdater.setup(&server, update_path, update_username, update_password);
  server.begin();
}

void loop() {
   // MQTT Loop
  client.loop();
  delay(10);

  // Make sure device is connected
  if(!client.connected()) 
  {
    connect();
  }

  server.handleClient();
}

void connect() {
  Serial.println("Connecting:");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(1000);
    
  }
  Serial.println("");
  Serial.println("is connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Let's connect to MQTT
  client.setOptions(60, true, 1000);
  client.begin(mqtt.server.c_str(), net);
  client.onMessage(messageReceived);
  Serial.println("Connecting to MQTT");
  while (!client.connect(mqtt.id.c_str())){
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Done! Got subscribed!");
  client.subscribe(mqtt.topic);
  client.setOptions(60,true,2000);
  
}

void messageReceived(String &topic, String &payload) 
{
   Serial.println("Incoming message at: " + topic + " - " + payload);
   size_t size = payload.length();
   const size_t bufferSize = 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 100;
   DynamicJsonBuffer jsonBuffer(bufferSize);
   JsonObject& root = jsonBuffer.parseObject(payload);
   myLedsG.red = map(root["r"],0,255,0,1023);
   myLedsG.green = map(root["g"],0,255,0,1023);
   myLedsG.blue = map(root["b"],0,255,0,1023);
   myLedsG.white = map(root["w"],0,255,0,1023);
   transition = root["transition"];
   setLEDs();
}

void setLEDs(){
  Serial.println("Setting new color!");

  if(transition != 0){
    // Let's calculate the delay between steps so the transition always will take 
    // the "transition" amount of time.
    int max_diff = abs(myLeds.red - myLedsG.red);
    if(max_diff < abs(myLeds.green - myLedsG.green)){
      max_diff = abs(myLeds.green - myLedsG.green);
    }
    if(max_diff < abs(myLeds.blue - myLedsG.blue)){
      max_diff = abs(myLeds.blue - myLedsG.blue);
    }
    if(max_diff < abs(myLeds.white - myLedsG.white)){
      max_diff = abs(myLeds.white - myLedsG.white);
    }

    Serial.print("Maximum difference: ");
    Serial.println(max_diff);
    
    // OK, now the delay in ms should be transition * 1000 / max_diff
    custom_delay = (int) ((transition * 1000) / max_diff);
   
   Serial.println("Custom delay: "+ custom_delay);
   
   if(custom_delay < 2){
    custom_delay = 2;
   }

    Serial.print("Custom delay: ");
    Serial.println(custom_delay);
    
    while(( myLeds.red != myLedsG.red ) || ( myLeds.green != myLedsG.green ) || ( myLeds.blue != myLedsG.blue ) || ( myLeds.white != myLedsG.white )){
      if(myLeds.red != myLedsG.red){
         if(myLeds.red < myLedsG.red){
            myLeds.red++;
         }else{
            myLeds.red--;
         }
      }
      if(myLeds.green != myLedsG.green){
         if(myLeds.green < myLedsG.green){
            myLeds.green++;
         }else{
            myLeds.green--;
         }
      }
      if(myLeds.blue != myLedsG.blue){
         if(myLeds.blue < myLedsG.blue){
            myLeds.blue++;
         }else{
            myLeds.blue--;
         }
      }
      if(myLeds.white != myLedsG.white){
         if(myLeds.white < myLedsG.white){
            myLeds.white++;
         }else{
            myLeds.white--;
         }
      }
      analogWrite(redPin,myLeds.red);
      analogWrite(greenPin,myLeds.green);
      analogWrite(bluePin,myLeds.blue);
      analogWrite(whitePin,myLeds.white);
      delay(custom_delay);
    }
  }else{
    myLeds.red = myLedsG.red;
    myLeds.green = myLedsG.green;
    myLeds.blue = myLedsG.blue;
    myLeds.white = myLedsG.white;
    analogWrite(redPin,myLedsG.red);
    analogWrite(greenPin,myLedsG.green);
    analogWrite(bluePin,myLedsG.blue);
    analogWrite(whitePin,myLedsG.white);
  }
  Serial.println("Done");
}

