
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// defines pins numbers
const int trigPin = 2;  //D4
const int echoPin = 0;  //D3
const int redLed = 5;  //D1
const int blueLed = 4;  //D2
const int irPin= 14;  //D5

// defines variables
boolean triggered = false;
long duration = 0;
int distance = 0;
int flagCount = 0;
int triggerCount = 0;
int distanceSetup = 0;
int distanceLoop = 0;

//************************* WiFi Access Point *********************************
const char* ssid = "Level1304";
const char* password = "crimson-king";
const char* SHA1Fingerprint = "C0:5D:08:5E:E1:3E:E0:66:F3:79:27:1A:CA:1F:FC:09:24:11:61:62";

//************************* Adafruit.io Setup *********************************
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                                // use 8883 for SSL
#define AIO_USERNAME    "dhiwakr"
#define AIO_KEY         "18fda380cab04c638b768b6916f948d4"

//***************************Global State********************************
WiFiClient client;                                          // Create an ESP8266 WiFiClient class to connect to the MQTT server.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;           // Store the MQTT server, username, and password in flash memory.
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);

//******************************Publishing Feeds Setup***************************************
const char loopDistance_FEED[] PROGMEM = AIO_USERNAME "/feeds/distance-loop";
Adafruit_MQTT_Publish loopDistanceFeed = Adafruit_MQTT_Publish(&mqtt, loopDistance_FEED);

//*********************************** Setup *********************************
void setup() {
    Serial.begin(115200); // Starts the serial communication
    if(WiFi.status() != WL_CONNECTED){
        Serial.println("Wifi Connection Initialised");
        Serial.println("Connecting");
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
             delay(1000);
             Serial.print(".");
       }
    }
    Serial.println("Wifi Connected");
    Serial.println("IP address: ");
    Serial.print(WiFi.localIP());

    pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
    pinMode(echoPin, INPUT); // Sets the echoPin as an Input
    pinMode(irPin, INPUT); //IR Sensor
    pinMode(redLed, OUTPUT);
    pinMode(blueLed, OUTPUT);
    triggered = false;
    flagCount = 0;
    Serial.println("\nCalibration Begin");
    Serial.print(".");
    digitalWrite(redLed, HIGH);
    Serial.print(".");
    delay(5000);
    digitalWrite(blueLed, HIGH);
    Serial.print(".");
    delay(5000);
    Serial.print(".");

    distanceSetup = CalibrateSensor();

    Serial.println("Calibration Complete ");
    digitalWrite(redLed, LOW);
    digitalWrite(blueLed, LOW);
    Serial.println("Setup Distance (in Cms) : ");
    Serial.print(distanceSetup);
}

void loop() {
  if (distanceSetup < 10){
    setup();//Re-Run Callibration - Incorrect previous setup
  }
  MQTT_connect();

  int counter = 0;
  if (triggered) {
      postToIFTTT();
      triggerCount = triggerCount + 1;
      while(counter < 21){
          TriggerSequence(counter);
          counter = counter + 1;
        }
      setup();
    }
  else {
    distanceLoop = CalibrateSensor();
    if (distanceLoop < distanceSetup - 20) { //Relaxation Limit 20cms
        if(digitalRead(irPin) == HIGH){
          triggered = true;
          Serial.println("Triggered");
          Serial.println("distanceSetup");
          Serial.println(distanceSetup);
          Serial.println("distanceLoop");
          Serial.println(distanceLoop);
          //Adafruit Lines Start

        if (!loopDistanceFeed.publish(distanceLoop)){
              Serial.println(F("Sending Loop Distance Failed"));
            }
          else{
               Serial.println(F("Loop Distance Sent!"));
             }
          //Adafruit Line Ends
          }
      }
    }

    if(! mqtt.ping()) {
      Serial.println("MQTT Disconnected");
      mqtt.disconnect();}
}

int CalibrateSensor(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  //Velocity of Sound = 0.034cm/microsec
  distance= duration*0.034/2;
  delay(200);
  return distance;
}

void postToIFTTT(){
  if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin("https://maker.ifttt.com/trigger/IntruderDetected/with/key/vTunY9sALMkg8y7YpQO5V", SHA1Fingerprint);
      http.addHeader("Content-Type", "text/plain");
      int httpCode = http.GET();
      http.end();
  }
}

void TriggerSequence(int count){
  if(count== 5){
        IntruderLightsON();
      }
      if(count  == 20){
          IntruderLightsOFF();
      }
  digitalWrite(redLed, HIGH);
  digitalWrite(blueLed, LOW);
  delay(500);
  digitalWrite(redLed, LOW);
  digitalWrite(blueLed, HIGH);
  delay(500);
}

void IntruderLightsON(){
  if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin("https://maker.ifttt.com/trigger/IntruderLightsON/with/key/vTunY9sALMkg8y7YpQO5V", SHA1Fingerprint);
      http.addHeader("Content-Type", "text/plain");
      int httpCode = http.GET();
      http.end();
  }
}

void IntruderLightsOFF(){
  if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin("https://maker.ifttt.com/trigger/IntruderLightsOFF/with/key/vTunY9sALMkg8y7YpQO5V", SHA1Fingerprint);
      http.addHeader("Content-Type", "text/plain");
      int httpCode = http.GET();
      http.end();
  }
}

// ******************* MQTT Connect - Adafruit IO **************************
void MQTT_connect(){
  int8_t ret;
  if (mqtt.connected()) { return; }                       // Stop if already connected to Adafruit
  Serial.println("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {                   // Connect to Adafruit, will return 0 if connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT...");
       mqtt.disconnect();
       delay(5000);                                       // wait 5 seconds
       retries--;
       if (retries == 0) {                                // basically die and wait for WatchDogTimer to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
