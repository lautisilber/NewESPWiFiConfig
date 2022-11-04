#include <Arduino.h>

#include "WiFiConfig.h"

WiFiConfig server;

void setup() {
  Serial.begin(115200);

  server.createRoutes();

  server.addNetwork(0, "Wifi-Casa", "canotaje");

  server.begin();
}

void loop() {
  server.loop();
}