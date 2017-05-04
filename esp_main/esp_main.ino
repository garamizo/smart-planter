// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ThingSpeak.h"

#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <Ticker.h>

Ticker flipper;

#define DEBUG

// hardware settings
int pin_load = 4,
    pin_pump = 5,
    pin_temp = 0,
    pin_wlvl = 12;

// ThingSpeak Settings
const int channelID_write = 266364;
const int channelID = 264988;
const char* writeAPIKey = "O330OOGZLJGD0HPG"; // write API key for your ThingSpeak Channel
const char* server = "api.thingspeak.com";

WiFiClient client;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(pin_temp);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

float pump_ctrl = 10; // seconds to keep pump off

volatile bool pump_state = false;
volatile long timer_count = 0;

void cycle_pump(void) {

  pump_state = !pump_state;
  if (pump_ctrl != 1 && pump_ctrl != 100)
    digitalWrite(pin_pump, pump_state);
  else
    digitalWrite(pin_pump, pump_ctrl == 1 ? HIGH : LOW);

  flipper.attach(pump_state ? 5 : pump_ctrl, cycle_pump);
}

/*
 * The setup function. We only start the sensors here
 */
void setup(void)
{
  // start serial port
  Serial.begin(9600);
  pinMode(pin_pump, OUTPUT);  

  // Start up the library
  sensors.begin();

  WiFiManager wifiManager;
//  WiFi.begin(ssid, password);
  wifiManager.autoConnect("SmartPlanter0001");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Trying to connect...");
  }
  Serial.println("Connected");
  
  ThingSpeak.begin(client);

  flipper.attach(1, cycle_pump);
  pinMode(pin_wlvl, INPUT);
}

/*
 * Main function, get and show the temperature
 */
void loop(void)
{

  // post
  static long t3 = millis();
  if(millis() >= t3) {
    // read sensors
    int bright = analogRead(A0);
    sensors.requestTemperatures(); // Send the command to get temperatures
    float temp = sensors.getTempCByIndex(0);
    int level = pulseIn(pin_wlvl, HIGH, 100000);

    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, bright);
    ThingSpeak.setField(3, int(pump_ctrl));
    ThingSpeak.setField(4, level);
        
    if( ThingSpeak.writeFields(channelID, writeAPIKey) == 200 ) { // success
      Serial.println(String(temp) + '\t' + bright + '\t' + level);
      t3 = millis() + 15000;
    }
    else
      Serial.println("Could not write to the cloud");
  }


  // update states
  static long t4 = millis();
  if(millis() >= t4) {
    float reading = ThingSpeak.readFloatField(channelID_write, 1);
    if (reading != 0) {
      
      if(pump_ctrl != reading) {
        pump_state = false;
        pump_ctrl = reading;
        flipper.attach(1, cycle_pump);
      }        
      pump_ctrl = reading;

      t4 = millis() + 1000;
      Serial.println(String("Updated pump control to ") + pump_ctrl);
    }
    else
      Serial.println("Could not read from cloud");
  }
  
}

