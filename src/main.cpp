

#include <WiFi.h>
#include "credentials.h"

// Pin definitions
#define RELAY_PIN 27

// Target server to check internet connectivity
const char *host = "google.com";
const uint16_t port = 80;

// Timing intervals (in milliseconds)
const long checkInterval = 86400000;  // check every 24 hours
const long requestTimeout = 10000; // expect a response within 10 seconds
const long resetTime = 10000;      // turn off the relay after 10 seconds
const long modemBootTime = 600000;   // wait 10 minutes for the modem to boot up

// Variables to track timing
unsigned long previousMillis = 0;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Event: WIFI_STA_CONNECTED");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Event: WIFI_STA_GOT_IP");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Event: WIFI_STA_DISCONNECTED");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to reconnect...");
  WiFi.begin(ssid, password);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ; // Needed for some boards

  delay(1000);

  // initialize LED digital pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize relay digital pin as an output
  pinMode(RELAY_PIN, OUTPUT);

  // delete old config
  WiFi.disconnect(true);

  delay(1000);

  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.begin(ssid, password);

  Serial.println();
  Serial.println();
}

void loop()
{

  unsigned long currentMillis = millis();

  // Check if 30 seconds have passed
  if (currentMillis - previousMillis >= checkInterval)
  {
    previousMillis = currentMillis;

    Serial.print("Checking WiFi connection to (");
    Serial.print(ssid);
    Serial.println(")");
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Unsuccessful. Gotta wait.");
      return;
    }
    else
    {
      Serial.println("Success!");
    }

    Serial.print("Checking internet connectivity to (");
    Serial.print(host);
    Serial.print(":");
    Serial.print(port);
    Serial.println(")");

    // Attempt to connect to an external host
    WiFiClient client;
    int connectionError = client.connect(host, port, requestTimeout);
    if (connectionError == 0 && WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Connection failed! Internet may be down or unstable.");

      Serial.println("Turning on relay to reset modem...");
      // turn on the relay and LED to indicate no internet

      // turn on the relay to reset the modem
      digitalWrite(RELAY_PIN, HIGH);

      // wait for modem to reset
      delay(resetTime);

      // turn off the relay to restart the modem
      digitalWrite(RELAY_PIN, LOW);

      Serial.println("Relay turned off. Waiting for internet to stabilize...");

      // wait for modem to start up
      delay(modemBootTime);
    }
    else if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi not connected!");
      client.stop(); // Close the connection immediately
    }
    else
    {
      Serial.println("Successfully connected to the internet!");
      client.stop(); // Close the connection immediately
    }
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}