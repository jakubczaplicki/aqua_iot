// This #include statement was automatically added by the Particle IDE.
#include <OneWire.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_SSD1306.h>

#include <MQTT.h>
#include "DS18.h"

#define MQTT_KEEPALIVE 30 * 60

// Screen
// use hardware SPI
#define OLED_DC     D3
#define OLED_CS     D4
#define OLED_RESET  D5
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);

//LEDStatus blinkRed(RGB_COLOR_RED, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_NORMAL);
DS18 tempSensor(D0);
double temperature;
bool publish_temperature = false;
bool send_mqqt = false;
// screen
int  x, minX; // variables for scrolling code

// mqtt
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void timer_callback_temperature_check();
void timer_callback_send_mqqt_data();

Timer temperatureTimer(5 * 60 * 1000, timer_callback_temperature_check);
Timer mqttTimer(15 * 60 * 1000, timer_callback_send_mqqt_data);

byte server[] = { 192, 168, 0, 108 };
MQTT client(server, 1883, MQTT_KEEPALIVE, mqtt_callback);

// callbacks implementation
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    Particle.publish("mqtt", p, 3600, PRIVATE);
}

void timer_callback_temperature_check() {
    publish_temperature = true;
}

void timer_callback_send_mqqt_data() {
  send_mqqt = true;
}

void display_fill_rect(void) {
  uint8_t color = 1;
  for (int16_t i=0; i<display.height()/2; i+=3) {
    // alternate colors
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, color%2);
    display.display();
    color++;
  }
}


void setup() {
  pinMode(D7, OUTPUT);
  
  // setup display
  display.begin(SSD1306_SWITCHCAPVCC);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setTextWrap(false);
  display_fill_rect();

  Particle.variable("tempC", temperature);
  // connect to mqtt broker
  client.connect("photon", "mqtt", "mqtt");
  if (client.isConnected())
  {
    Particle.publish("mqtt", "Connected to Home Assistant", 3600, PRIVATE);
  }
  else
  {
    Particle.publish("mqtt", "Failed to connect to Home Assistant", 3600, PRIVATE);
  }
  LEDStatus(RGB_COLOR_WHITE, LED_PATTERN_BLINK, LED_SPEED_FAST, LED_PRIORITY_NORMAL);

  // start timers 
  temperatureTimer.start();
  mqttTimer.start();
}

void loop() {
    if (tempSensor.read())
        temperature = tempSensor.celsius();

    display.clearDisplay();
    display.setCursor(0, 5);
    display.print(temperature);
    display.display();

    if (publish_temperature)
    {
      Particle.publish("temp", String(temperature, 1), 3600, PRIVATE);
      publish_temperature = false;
    }

    
    if (send_mqqt) 
    {
      digitalWrite(D7, HIGH);
      if (client.isConnected()) 
      {
        Particle.publish("mqtt", "Pushing " + String(temperature, 1) + " to Home Assistant", 3600, PRIVATE);
        client.publish("photon/aqua/temp", String(temperature, 1));
      }
      else 
      {
        client.connect("photon", "mqtt", "mqtt");
        if (client.isConnected())
        {
          Particle.publish("mqtt", "Re-connected to Home Assistant", 3600, PRIVATE);
        } 
        else 
        {
          Particle.publish("mqtt", "Failed to re-connect to Home Assistant", 3600, PRIVATE);
        }
      }
      delay(1000);
      digitalWrite(D7, LOW);
      send_mqqt = false;
    }
}

