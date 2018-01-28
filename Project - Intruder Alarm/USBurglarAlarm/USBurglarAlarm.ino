#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Level1304";
const char* password = "crimson-king";
const char* SHA1Fingerprint = "C0:5D:08:5E:E1:3E:E0:66:F3:79:27:1A:CA:1F:FC:09:24:11:61:62";

// defines pins numbers
const int trigPin = 2;  //D4
const int echoPin = 0;  //D3
const int redLed = 5;  //D1
const int blueLed = 4;  //D2

// defines variables
boolean triggered = false;

long duration = 0;
int distance = 0;
int flagCount = 0;

int triggerCount = 0;

int distanceSetup = 0;
int distanceLoop = 0;


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

if (distanceSetup < 10)
{
  setup();
}


int counter = 0;

  if (triggered) {

    postToIFTTT();

    triggerCount = triggerCount + 1;
    while(counter < 21)
    {
          if(counter== 5){
              IntruderLightsON();
            }
            if(counter  == 20){
                IntruderLightsOFF();
            }


        digitalWrite(redLed, HIGH);
        digitalWrite(blueLed, LOW);
        delay(500);
        digitalWrite(redLed, LOW);
        digitalWrite(blueLed, HIGH);
        delay(500);
        counter = counter + 1;
      }
    setup();
  }

  else {

    distanceLoop = CalibrateSensor();
    if (distanceLoop < distanceSetup - 20) { //Relaxation Limit 20cms
      if(flagCount > 0){ //Trigger on 2 (n) successive flags - Set Value as n-2
      triggered = true;
      }
      else{
        flagCount = flagCount + 1;
      }
    }
    else{
      flagCount = 0;
    }
}

}

int CalibrateSensor()
{
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
