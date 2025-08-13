# Led-control-mqtt
Controle de led via mqtt por aplicação web

# Arquitetura Geral
[APLICAÇÃO WEB] --(MQTT/WebSocket)--> [BROKER MQTT] <--(MQTT/TCP)--> [ESP32]

# Firmware no ESP32
#include <WiFi.h>          // Conexão WiFi
#include <PubSubClient.h>  // Comunicação MQTT
#include <ArduinoJson.h>   // Processamento de JSON

// Definições de pinos e constantes
#define PIN_RED 14         // GPIO para LED Vermelho
#define PIN_GREEN 27       // GPIO para LED Verde
#define PIN_BLUE 26        // GPIO para LED Azul

// Modos de operação
enum {
  MODE_OFF,               // LEDs desligados
  MODE_FAST_COLOR_CHANGE, // Transição rápida entre cores
  MODE_SLOW_COLOR_CHANGE, // Transição lenta entre cores
  MODE_FAST_SINGLE_COLOR, // Piscar vermelho rápido
  MODE_CUSTOM_SEQUENCE    // Sequência personalizada
};
## Fluxo de Funcionamento:
### Inicialização:
void setup() {
  setupPins();       // Configura GPIOs
  setupWiFi();       // Conecta ao WiFi
  setupMQTT();       // Configura cliente MQTT
}
### Loop Principal:
void loop() {
  maintainConnection();  // Gerencia conexões
  processMQTT();         // Verifica mensagens
  handleCurrentMode();   // Executa lógica do modo atual
}

### Comunicação MQTT:
Tópicos:
- carlos/rgb_pwm (entrada de comandos)
- carlos/rgb_pwm/status (saída de status)

### Processamento de Mensagens:
void callback(char* topic, byte* payload, unsigned int length) {
  // Converte payload para String
  // Detecta se é JSON ou comando simples
  // Executa ação correspondente
}

# Aplicação Web
## Fluxo de Funcionamento:
### Inicialização:
window.addEventListener('load', () => {
  connectMQTT();  // Conecta ao broker
  setupUI();      // Configura listeners de botões
});
### Comunicação MQTT:
function connectMQTT() {
  client = new Paho.Client("broker.hivemq.com", 8000, "clientId");
  client.connect({
    onSuccess: () => {
      client.subscribe("carlos/rgb_pwm/status");
    }
  });
}
### Controle dos LEDs:
function sendCommand(command) {
  const message = new Paho.Message(command);
  message.destinationName = "carlos/rgb_pwm";
  client.send(message);
}
# Protocolo de Comunicação
## Comandos da Web para ESP32:
### Comandos Simples (string):
"fast_red"
"off"
"get_status"

### Comandos Complexos (JSON):
{
  "brightness": 75,
  "sequence": [
    {"r":255, "g":0, "b":0},
    {"r":0, "g":255, "b":0}
  ],
  "speed": 1000
}

### Respostas do ESP32 para Web (JSON):
{
  "status": "fast_red",
  "brightness": 100,
  "mode": 3
}

# Controle dos LEDs
## Lógica de Acionamento:
void setColor(int r, int g, int b) {
  // Aplica brilho (0-100%)
  r = r * currentBrightness / 100;
  g = g * currentBrightness / 100;
  b = b * currentBrightness / 100;
  
  // Escreve nos pinos PWM (invertido para ânodo comum)
  analogWrite(PIN_RED, 255 - r);
  analogWrite(PIN_GREEN, 255 - g);
  analogWrite(PIN_BLUE, 255 - b);
}

## Modos de Operação:
### Mudança de Cores:
void handleColorChange(int speed) {
  if (millis() - lastChange > speed) {
    currentColorIndex = (currentColorIndex + 1) % colorCount;
    setColor(colors[currentColorIndex]);
    lastChange = millis();
  }
}

### Sequência Personalizada:
void handleCustomSequence() {
  if (millis() - lastChange > sequenceSpeed) {
    sequenceIndex = (sequenceIndex + 1) % sequenceLength;
    setColor(customSequence[sequenceIndex]);
    lastChange = millis();
  }
}

# Sincronização de Estado
## Fluxo Completo:
1. Usuário clica em "Vermelho Rápido" na web
2. Aplicação publica "fast_red" em carlos/rgb_pwm
3. ESP32 recebe e muda para MODE_FAST_SINGLE_COLOR
4. ESP32 publica status em carlos/rgb_pwm/status:
{"status":"fast_red","brightness":100}

5. Aplicação web recebe e atualiza a interface

# Tratamento de Erros
## No ESP32:
void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.subscribe("carlos/rgb_pwm");
      publishStatus(); // Reenvia estado atual
    } else {
      delay(5000);
    }
  }
}

## Na Web:
client.onConnectionLost = (response) => {
  if (response.errorCode !== 0) {
    showError("Conexão perdida");
    setTimeout(connectMQTT, 5000);
  }
};
