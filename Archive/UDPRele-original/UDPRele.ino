
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
//const char* ssid = "philip.gerasim ";
//const char* password = "Yogerrasim1";

const char* ssid = "";
const char* password = "04072000";



// Пины для реле
const int relayPins[5] = {5, 4, 14, 12, 13};  // Только 4 реле

void setup() {
  Serial.begin(115200);
  
  // Настройка пинов
  for (int i = 0; i < 5; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }


}

void loop() {
  delay(5000); 
  for (int i = 0; i < 5; i++) {
    bool requestedState = (true);
    digitalWrite(relayPins[i], LOW);  // Назначаем состояние независимо от текущего
    Serial.print("Relay ");
    Serial.print(i);
    Serial.print(" set to ");
    Serial.println(requestedState ? "HIGH" : "LOW");
  }
  delay(5000); 
  for (int i = 0; i < 5; i++) {
    bool requestedState = (false);
    digitalWrite(relayPins[i], HIGH);  // Назначаем состояние независимо от текущего
    Serial.print("Relay ");
    Serial.print(i);
    Serial.print(" set to ");
    Serial.println(requestedState ? "HIGH" : "LOW");
  }

}


  

