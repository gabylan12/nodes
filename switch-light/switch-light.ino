/**
 * Example use
 * GET curl -X GET  http://192.168.1.142/sensors
 * POST curl -X POST --data "{\"light1\":\"OFF\",\"light2\":\"OFF\"}" http://192.168.1.142/sensors
 */


#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include "Bounce2.h" // https://github.com/thomasfredericks/Bounce2/wiki

#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include "Adafruit_MQTT.h" //https://github.com/adafruit/Adafruit_MQTT_Library
#include "Adafruit_MQTT_Client.h"
#include <ESP8266WebServer.h>
#include <AsyncDelay.h> // https://github.com/stevemarple/AsyncDelay
#include "constants.h"


/************************* WiFi Access Point *********************************/

char ssid[] = WLAN_SSID;        // your network SSID (name)
char pass[] = WLAN_PASS;
/************************* Adafruit.io Setup *********************************/

char serverMqtt[] = AIO_SERVER;
int portMqtt = AIO_SERVERPORT;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, serverMqtt, portMqtt);

/****************************** Feeds ***************************************/

Adafruit_MQTT_Publish publishMqttLight1 = Adafruit_MQTT_Publish(&mqtt,  "living/light1/state");
Adafruit_MQTT_Publish publishMqttLight2 = Adafruit_MQTT_Publish(&mqtt,  "living/light2/state");
Adafruit_MQTT_Publish publishMqttLightTurnOff = Adafruit_MQTT_Publish(&mqtt,  "living/turn_off/state");
Adafruit_MQTT_Subscribe subscribeMqttLight1 = Adafruit_MQTT_Subscribe(&mqtt,  "living/light1/state");
Adafruit_MQTT_Subscribe subscribeMqttLight2 = Adafruit_MQTT_Subscribe(&mqtt,  "living/light2/state");
Adafruit_MQTT_Subscribe subscribeMqttLightTurnOff = Adafruit_MQTT_Subscribe(&mqtt,  "living/turn_off/state");


//RELAY
#define LIGHT_1_PIN  13 // D7
#define LIGHT_2_PIN  12 //D6
#define BUTTON_PIN_RELAY_1  4 //D2
#define BUTTON_PIN_RELAY_2  0 //D3
#define BUTTON_TURN_OFF  14 //D5


int state_relay1 = HIGH;       // estado actual del pin de salida 
int state_relay2 = HIGH;       // estado actual del pin de sali

Bounce debouncer_relay1 = Bounce(); 
Bounce debouncer_relay2 = Bounce();
Bounce debouncer_turn_off = Bounce();



//STATUS
#define LED_RED_STATUS  15 //D8
#define LED_GREEN_STATUS  1//  D5

ESP8266WebServer server(80);

AsyncDelay mqttDelay;

AsyncDelay wifiDelay;


void MQTT_connect();

void setup() {
  Serial.begin(115200);
  pinMode(LED_GREEN_STATUS, OUTPUT);
  pinMode(LED_RED_STATUS, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);



  //RELAY
  pinMode(LIGHT_1_PIN,  OUTPUT) ;
  pinMode(LIGHT_2_PIN,  OUTPUT) ;

  pinMode(BUTTON_PIN_RELAY_1, INPUT_PULLUP);
  debouncer_relay1.interval(5);
  debouncer_relay1.attach(BUTTON_PIN_RELAY_1);

  pinMode(BUTTON_PIN_RELAY_2, INPUT_PULLUP);
  debouncer_relay2.interval(5);
  debouncer_relay2.attach(BUTTON_PIN_RELAY_2);

  
  //TURN OFF
  pinMode(BUTTON_TURN_OFF, INPUT_PULLUP);
  debouncer_turn_off.interval(5);
  debouncer_turn_off.attach(BUTTON_TURN_OFF);

 
  
  digitalWrite(LED_RED_STATUS , HIGH);  
  digitalWrite(LED_GREEN_STATUS , LOW);  


  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&subscribeMqttLight1);
  mqtt.subscribe(&subscribeMqttLight2);
  mqtt.subscribe(&subscribeMqttLightTurnOff);


  server.on("/sensors", sensors);
  server.begin();
  mqttDelay.start(3000, AsyncDelay::MILLIS);

  WiFi.begin(ssid, pass);
  for(int i = 1 ; i < 5 ; i++){ 
      if(WiFi.status() != WL_CONNECTED) {
        delay(500);
      }
      else{
        break;
      }
   }
   wifiDelay.start(15000, AsyncDelay::MILLIS);


   

 


}

void connectWifi(){
  Serial.print("Connecting to ");
  Serial.println(ssid);
  for(int i = 1 ; i < 5 ; i++){ 
     if(WiFi.status() != WL_CONNECTED) {
       delay(500);
     }
     else{
       break;
     }
   }
   wifiDelay.repeat();

 
}

/* Rest server GET
 *  sensors status
 */
void sensors(){
   digitalWrite(LED_BUILTIN, LOW);
   DynamicJsonDocument doc(1024);
   if(server.method() == HTTP_GET){
     deserializeJson(doc, "{}");
     doc["light1"] = digitalRead(LIGHT_1_PIN)==LOW?"ON":"OFF";
     doc["light2"] = digitalRead(LIGHT_2_PIN)==LOW?"ON":"OFF"; 
     String output;
     serializeJson(doc, output);
     server.send(200, "application/json", output);
   }
   else if (server.method() == HTTP_POST){
     String body =server.arg("plain");
     deserializeJson(doc, body);
     int light = strcmp(doc["light1"],"ON") == 0?HIGH:LOW;
     int dread = digitalRead(LIGHT_1_PIN);  
     if(light != dread){
       digitalWrite(LIGHT_1_PIN, light); 
     } 
 
     light = strcmp(doc["light2"],"ON") == 0?HIGH:LOW;
     dread = digitalRead(LIGHT_2_PIN);  
     if(light != dread){
       digitalWrite(LIGHT_2_PIN, light); 
     }     
     server.send(200, "application/json", body);

   }
   digitalWrite(LED_BUILTIN, HIGH);

}


/**
 * function for connect to mqtt server
 */
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("MQTT Connected!");
      digitalWrite(LED_BUILTIN, HIGH);
      return;
  }

  Serial.print("Connecting to MQTT... ");
  digitalWrite(LED_BUILTIN, LOW);

  if ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       mqtt.disconnect();
  } 
  digitalWrite(LED_BUILTIN, HIGH);

  mqttDelay.repeat(); // Count from when the delay expired, not now
}

/**
 * check the if the buttons were pressed to
 * change the relays status or send command to openhab
 * 
 */
void checkButtons(){
  int dread ;

  debouncer_relay1.update();
  
  // Get the update value
  int value = debouncer_relay1.read();
  if (value != state_relay1 && value==0) {
     digitalWrite(LED_BUILTIN, LOW);
     dread =digitalRead(LIGHT_1_PIN);
     digitalWrite(LIGHT_1_PIN , !dread);  
       if (! publishMqttLight1.publish(dread==LOW?"ON":"OFF")) {
         Serial.println(F("Failed"));
       } else {
         Serial.println(F("OK!"));
       }
     Serial.println("change state relay1");
    
     digitalWrite(LED_BUILTIN, HIGH);
  }
  state_relay1 = value;


  debouncer_relay2.update();
  // Get the update value
  value = debouncer_relay2.read();
  if (value != state_relay2 && value==0) {
     
     digitalWrite(LED_BUILTIN, LOW);
     dread =digitalRead(LIGHT_2_PIN);
     digitalWrite(LIGHT_2_PIN, !dread);   
     if (! publishMqttLight2.publish(dread==LOW?"ON":"OFF")) {
         Serial.println(F("Failed"));
     } else {
         Serial.println(F("OK!"));
     }
     Serial.println("change state relay2");

     digitalWrite(LED_BUILTIN, HIGH);
  }

  state_relay2 = value;


  debouncer_turn_off.update();
  // Get the update value
  value = debouncer_turn_off.read();
  if (value==0) {
     digitalWrite(LED_BUILTIN, LOW);
     if (! publishMqttLightTurnOff.publish("ON")) {
        Serial.println(F("Failed"));
     } else {
        Serial.println(F("OK!"));
     }
     Serial.println("change state relay2");
     Serial.println("turn off");
     digitalWrite(LED_BUILTIN, HIGH);

  }

  
}

/**
 * listen to mqtt topic subscriptions
 */
void listenSubscriptions(){
  char* payload;
  int dread;
  int payloadValue;
  if (!mqtt.connected()) {
    return;
  }
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription())) {
    if (subscription == &subscribeMqttLight1) {
      Serial.print(F("Got: "));
      payload= (char *)subscribeMqttLight1.lastread;
      payloadValue = strcmp(payload,"ON") == 0?HIGH:LOW;
      dread = digitalRead(LIGHT_1_PIN);  
      if(payloadValue != dread){
        digitalWrite(LIGHT_1_PIN, payloadValue); 
      } 
      Serial.println(payload);
    }
    else if (subscription == &subscribeMqttLight2) {
      Serial.print(F("Got: "));
      payload= (char *)subscribeMqttLight2.lastread;
      payloadValue = strcmp(payload,"ON") == 0?HIGH:LOW;
      dread = digitalRead(LIGHT_2_PIN);  
      if(payloadValue != dread){
        digitalWrite(LIGHT_2_PIN, payloadValue); 
      } 
      Serial.println(payload);
    }
  }

}

void loop() {

  if(  mqttDelay.isExpired()) {
    Serial.println("connecting");
    MQTT_connect();
  }

  checkButtons();

  listenSubscriptions();

  server.handleClient();

  if(  wifiDelay.isExpired()) {
     connectWifi();
  }
 
        
        

   
}
