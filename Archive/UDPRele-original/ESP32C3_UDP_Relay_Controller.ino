/*
  ESP32-C3 UDP Relay Controller
  Принимает UDP-команды и управляет 8 реле.

  Основной формат:
    10101010  -> выставить все 8 реле сразу
    00000000  -> выключить все
    11111111  -> включить все

  Дополнительные команды:
    ON 1      -> включить реле 1
    OFF 1     -> выключить реле 1
    TOGGLE 1  -> переключить реле 1
    ALL ON    -> включить все
    ALL OFF   -> выключить все
    STATUS    -> вернуть состояние

  ВАЖНО:
  - Пины relayPins[] надо заменить под твою реальную распайку ESP32-C3.
  - Если реле включается наоборот, поменяй RELAY_ACTIVE_LOW на true/false.
*/

#include <WiFi.h>
#include <WiFiUdp.h>

// ===================== Wi-Fi =====================

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Можно оставить 4210, как в старом скетче
const uint16_t LOCAL_UDP_PORT = 4210;

// ===================== Relay settings =====================

// true  = реле включается уровнем LOW
// false = реле включается уровнем HIGH
// Для большинства дешёвых релейных модулей часто true.
// Для SSR/модулей с нормальной логикой может быть false.
const bool RELAY_ACTIVE_LOW = false;

// Замени на свои реальные GPIO.
// Не используй GPIO8/GPIO9, если не уверен: они могут влиять на загрузку платы.
// GPIO18/GPIO19 на ESP32-C3 часто заняты USB, их лучше не трогать,
// если прошиваешь/отлаживаешь через USB-C.
const uint8_t relayPins[] = {
  0, 1, 2, 3, 4, 5, 6, 7
};

const uint8_t NUM_RELAYS = sizeof(relayPins) / sizeof(relayPins[0]);

// Состояние реле в логическом виде:
// true = включено, false = выключено
bool relayState[NUM_RELAYS];

WiFiUDP udp;
char incomingPacket[256];

// ===================== Helpers =====================

void writeRelay(uint8_t index, bool on) {
  if (index >= NUM_RELAYS) return;

  relayState[index] = on;

  if (RELAY_ACTIVE_LOW) {
    digitalWrite(relayPins[index], on ? LOW : HIGH);
  } else {
    digitalWrite(relayPins[index], on ? HIGH : LOW);
  }
}

void setAllRelays(bool on) {
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    writeRelay(i, on);
  }
}

String getStateString() {
  String state = "";
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    state += relayState[i] ? "1" : "0";
  }
  return state;
}

void sendUdpReply(const String& message) {
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.print(message);
  udp.endPacket();
}

String normalizeCommand(String command) {
  command.trim();
  command.replace("\r", "");
  command.replace("\n", "");
  command.toUpperCase();
  return command;
}

bool isBinaryRelayMask(const String& command) {
  if (command.length() != NUM_RELAYS) return false;

  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    if (command[i] != '0' && command[i] != '1') {
      return false;
    }
  }

  return true;
}

void applyBinaryRelayMask(const String& mask) {
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    writeRelay(i, mask[i] == '1');
  }
}

void processCommand(String command) {
  command = normalizeCommand(command);

  if (command.length() == 0) {
    sendUdpReply("ERR empty command");
    return;
  }

  Serial.print("UDP command: ");
  Serial.println(command);

  // Формат: 10101010
  if (isBinaryRelayMask(command)) {
    applyBinaryRelayMask(command);
    sendUdpReply("OK STATE " + getStateString());
    return;
  }

  // ALL ON / ALL OFF
  if (command == "ALL ON" || command == "ON ALL") {
    setAllRelays(true);
    sendUdpReply("OK STATE " + getStateString());
    return;
  }

  if (command == "ALL OFF" || command == "OFF ALL") {
    setAllRelays(false);
    sendUdpReply("OK STATE " + getStateString());
    return;
  }

  // STATUS
  if (command == "STATUS") {
    sendUdpReply("STATE " + getStateString());
    return;
  }

  // ON 1 / OFF 1 / TOGGLE 1
  int spaceIndex = command.indexOf(' ');
  if (spaceIndex > 0) {
    String action = command.substring(0, spaceIndex);
    String numberPart = command.substring(spaceIndex + 1);
    numberPart.trim();

    int relayNumber = numberPart.toInt(); // пользовательский номер 1..8
    int relayIndex = relayNumber - 1;     // индекс массива 0..7

    if (relayNumber < 1 || relayNumber > NUM_RELAYS) {
      sendUdpReply("ERR relay number must be 1.." + String(NUM_RELAYS));
      return;
    }

    if (action == "ON") {
      writeRelay(relayIndex, true);
      sendUdpReply("OK STATE " + getStateString());
      return;
    }

    if (action == "OFF") {
      writeRelay(relayIndex, false);
      sendUdpReply("OK STATE " + getStateString());
      return;
    }

    if (action == "TOGGLE") {
      writeRelay(relayIndex, !relayState[relayIndex]);
      sendUdpReply("OK STATE " + getStateString());
      return;
    }
  }

  sendUdpReply("ERR unknown command");
}

// ===================== Setup / Loop =====================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-C3 UDP Relay Controller");

  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    pinMode(relayPins[i], OUTPUT);
    writeRelay(i, false); // безопасный старт: все выключены
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Wi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  udp.begin(LOCAL_UDP_PORT);
  Serial.print("UDP listening on port: ");
  Serial.println(LOCAL_UDP_PORT);
}

void loop() {
  int packetSize = udp.parsePacket();

  if (packetSize > 0) {
    int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);

    if (len > 0) {
      incomingPacket[len] = '\0';
      processCommand(String(incomingPacket));
    }
  }

  // Небольшая пауза, чтобы не грузить CPU
  delay(2);
}
