#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// GitHub Page = https://github.com/Tommrodrigues/homebridge-nodemcu-relay

// Script Type = Relay Momentary, Switch, Modulation

// D7 = Relay

/////////////////// CHANGE THESE VALUES //////////////////////
// Required:
const char* ssid = "LeON2.4"; // Name of your network
const char* password = "Password"; // Password for your network
const String relay = "HIGH"; // Relay type (`HIGH` or `LOW`)

// For Modulation:
const uint32_t modulationOn = 5000; // Time (in ms) for relay to be ON when modulating
const uint32_t modulationOff = 20000; // Time (in ms) for relay to be OFF when modulating
// For Momentary:
const int momentaryOn = 1000; // Delay time (in ms) for the ON state for MOMENTARY
const int momentaryOff = 1000; // Delay time (in ms) for the OFF state for MOMENTARY
//////////////////////////////////////////////////////////////

const int relayPin = 16;
int state = 0;
bool ignoreMe = false;

int relayOn, relayOff, brightness;
bool led_blinking, led_on;
uint32_t last_toggle;

WebServer server(80);

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

void setup() {
  if (relay.equals("LOW")) {
    relayOn = 0;
    relayOff = 1;
  } else {
    relayOn = 1;
    relayOff = 0;
  }

  //pinMode(relayPin, OUTPUT);
  ledcSetup(ledChannel, freq, resolution);
  //analogWrite(relayPin, 0);
  ledcAttachPin(relayPin, ledChannel);

  Serial.begin(115200);
  delay(10);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.println("Connecting to \"" + String(ssid) + "\"");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(String(++i) + " ");
  }
  Serial.println();
  Serial.println("Connected successfully");

  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/setState", []() {
    String type = server.arg("type");
    state = server.arg("value").toInt();
    if (type.equals("modulation")) {
      if (state) {
        ignoreMe = false;
        start_blinking();
      } else {
        ignoreMe = false;
        stop_blinking();
      }
    }

    if (type.equals("momentary")) {
      if (state) {
        stop_blinking();
        digitalWrite(relayPin, relayOn);
        delay(momentaryOn);
        digitalWrite(relayPin, relayOff);
        ignoreMe = false;
      } else {
        stop_blinking();
        digitalWrite(relayPin, relayOn);
        delay(momentaryOff);
        digitalWrite(relayPin, relayOff);
        ignoreMe = false;
      }
    }

    if (type.equals("switch")) {
      stop_blinking();
      //if (state) {
      
      Serial.println(state);
        //analogWrite(relayPin, brightness);
      if(state == 0) {
          led_on = false;
          ledcWrite(ledChannel, 0);
      } else {
          led_on = true;
          ledcWrite(ledChannel, brightness);
      }
      
    
        ignoreMe = true;
     // } else {
      //  digitalWrite(relayPin, relayOff);
     //   ignoreMe = false;
     // }
    }

    if(type.equals("brightness")) {
       brightness = state / 100.0 * 255;
       ledcWrite(ledChannel, brightness);
    }
    
    server.send(200);
  });

  server.on("/status", []() {
    server.send(200, "text/html", led_on ? "ON" : "OFF");
  });

  server.on("/status/brightness", []() {
    server.send(200, "text/html", String(brightness));
  });

  // Start the server
  server.begin();
}

//Start of modulation functions
void update_led() {
  uint32_t now = millis();
  if (!led_blinking && !ignoreMe) {
    digitalWrite(relayPin, relayOff);
    led_on = false;
    last_toggle = now - modulationOff;
    return;
  }
  if (led_on && now - last_toggle >= modulationOn && !ignoreMe) {
    digitalWrite(relayPin, relayOff);
    led_on = false;
    last_toggle = now;
  }
  if (!led_on && now - last_toggle >= modulationOff && !ignoreMe) {
    digitalWrite(relayPin, relayOn);
    led_on = true;
    last_toggle = now;
  }
}

void start_blinking() {
  digitalWrite(relayPin, relayOn);
  led_blinking = true;
  led_on = true;
  last_toggle = millis();
}

void stop_blinking() {
  digitalWrite(relayPin, relayOff);
  led_blinking = false;
  led_on = false;
}
//End of modulation functions

//Main loop
void loop() {
  update_led();
  server.handleClient();
}
