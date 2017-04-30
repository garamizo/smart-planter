// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ThingSpeak.h"

#include <ESP8266WiFi.h>

#define DEBUG

// hardware settings
int pin_load = 4,
    pin_pump = 5,
    pin_temp = 0;

// Wi-Fi Settings
const char* ssid = "Samsung GALAXY Note4 3640"; // your wireless network name (SSID)
const char* password = "hvkv8269"; // your Wi-Fi network password

//const char* ssid = "Galaxy S5 5664"; // your wireless network name (SSID)
//const char* password = "idbt3270"; // your Wi-Fi network password

//const char* ssid = "Hao's iPhone";
//const char* password = "hli89865426";

// ThingSpeak Settings
const int channelID = 264988;
const char* writeAPIKey = "O330OOGZLJGD0HPG"; // write API key for your ThingSpeak Channel
const char* server = "api.thingspeak.com";

WiFiClient client;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(pin_temp);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

float pump_ctrl = 10; // seconds to keep pump off

/*
 * The setup function. We only start the sensors here
 */
void setup(void)
{
  // start serial port
  Serial.begin(9600);
  pinMode(pin_load, OUTPUT); 
  pinMode(pin_pump, OUTPUT);  

  // Start up the library
  sensors.begin();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Trying to connect...");
  }
  Serial.println("Connected");
  
  ThingSpeak.begin(client);
}

/*
 * Main function, get and show the temperature
 */
void loop(void)
{
  // watering
  static long t0 = millis();
  if(millis() >= t0) {
    static bool state = LOW;

    digitalWrite(pin_pump, state = !state);
    
    #ifdef DEBUG
    t0 += ( state == HIGH ? 1000 : (pump_ctrl * 1000) );
    #else
    t0 += ( state == HIGH ? 60000 : (60 * 60000) );
    #endif
  }

  // wakup
  static long t1 = millis();
  if(millis() >= t1) {
    static bool state = LOW;

    digitalWrite(pin_load, state = !state);
    t1 = millis() + (state == HIGH ? 1000 : 35000);
  }

  // sensors, to take action
  static long t2 = millis();
  if(millis() >= t2) {
    sensors.requestTemperatures(); // Send the command to get temperatures
    Serial.println(sensors.getTempCByIndex(0));

    #ifdef DEBUG
    t2 += 10 * 60000;
    #else
    t2 += 4000;
    #endif
  }

  // post
  static long t3 = millis();
  if(millis() >= t3) {
    // read sensors
    int bright = analogRead(A0);
    sensors.requestTemperatures(); // Send the command to get temperatures
    float temp = sensors.getTempCByIndex(0);

    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, bright);
    ThingSpeak.setField(3, pump_ctrl);
    ThingSpeak.writeFields(channelID, writeAPIKey); 

    #ifdef DEBUG
    t3 += 20000;
    #else
    t3 += 10 * 60000;
    #endif
  }

  // update states
  static long t4 = millis();
  if(millis() >= t4) {
    pump_ctrl = ThingSpeak.readFloatField(channelID, 3);
    t4 = millis() + 20000;
    Serial.println(String("Updated pump control to ") + pump_ctrl);
  }
}

