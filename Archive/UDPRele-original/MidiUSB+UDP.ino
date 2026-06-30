#include <WiFi.h>
#include <WiFiUdp.h>
#include "USB.h"
#include "USBH_MIDI.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Пины для 8 реле (можешь заменить под себя)
const int relayPins[8] = {2, 4, 5, 12, 13, 14, 15, 16}; // пример
const int numRelays = sizeof(relayPins) / sizeof(relayPins[0]);

WiFiUDP udp;
unsigned int localUdpPort = 4210;  // Порт для UDP

USBHost usb;
USBH_MIDI  midi(usb);

char incomingPacket[255];

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  for (int i = 0; i < numRelays; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting...");
    delay(1000);
  }

  Serial.println("Connected to WiFi.");
  udp.begin(localUdpPort);
  usb.begin();
}

void loop() {
  // MIDI
  usb.Task();
  if (midi.available()) {
    midi_event_t event;
    midi.read(&event);

    if (event.type == 0x9) { // NoteOn
      byte note = event.midi1;
      handleMidiNote(note);
    }
  }

  // UDP
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = '\0';
      processRelayState(incomingPacket);
    }
  }
}

void handleMidiNote(byte note) {
  // Активировать реле по номеру ноты % 8
  String state = "00000000";
  int idx = note % 8;
  state[idx] = '1';
  processRelayState(state.c_str());
}

void processRelayState(const char* data) {
  if (strlen(data) != numRelays) return;

  for (int i = 0; i < numRelays; i++) {
    if (data[i] == '1') {
      digitalWrite(relayPins[i], HIGH);
    } else {
      digitalWrite(relayPins[i], LOW);
    }
  }
}