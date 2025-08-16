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
_void setup() {
  setupPins();       // Configura GPIOs
  setupWiFi();       // Conecta ao WiFi
  setupMQTT();       // Configura cliente MQTT
}_
### Loop Principal:
_void loop() {
  maintainConnection();  // Gerencia conexões
  processMQTT();         // Verifica mensagens
  handleCurrentMode();   // Executa lógica do modo atual
}_

### Comunicação MQTT:
Tópicos:
- carlos/rgb_pwm (entrada de comandos)
- carlos/rgb_pwm/status (saída de status)

### Processamento de Mensagens:
_void callback(char* topic, byte* payload, unsigned int length) {
  // Converte payload para String
  // Detecta se é JSON ou comando simples
  // Executa ação correspondente
}
_
# Aplicação Web
## Fluxo de Funcionamento:
### Inicialização:
_window.addEventListener('load', () => {
  connectMQTT();  // Conecta ao broker
  setupUI();      // Configura listeners de botões
});_
### Comunicação MQTT:
_function connectMQTT() {
  client = new Paho.Client("broker.hivemq.com", 8000, "clientId");
  client.connect({
    onSuccess: () => {
      client.subscribe("carlos/rgb_pwm/status");
    }
  });
}_
### Controle dos LEDs:
_function sendCommand(command) {
  const message = new Paho.Message(command);
  message.destinationName = "carlos/rgb_pwm";
  client.send(message);
}_
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
_void setColor(int r, int g, int b) {
  // Aplica brilho (0-100%)
  r = r * currentBrightness / 100;
  g = g * currentBrightness / 100;
  b = b * currentBrightness / 100;
  
  // Escreve nos pinos PWM (invertido para ânodo comum)
  analogWrite(PIN_RED, 255 - r);
  analogWrite(PIN_GREEN, 255 - g);
  analogWrite(PIN_BLUE, 255 - b);
}_

## Modos de Operação:
### Mudança de Cores:
_void handleColorChange(int speed) {
  if (millis() - lastChange > speed) {
    currentColorIndex = (currentColorIndex + 1) % colorCount;
    setColor(colors[currentColorIndex]);
    lastChange = millis();
  }
}_

### Sequência Personalizada:
_void handleCustomSequence() {
  if (millis() - lastChange > sequenceSpeed) {
    sequenceIndex = (sequenceIndex + 1) % sequenceLength;
    setColor(customSequence[sequenceIndex]);
    lastChange = millis();
  }
}_

# Sincronização de Estado
## Fluxo Completo:
1. Usuário clica em "Vermelho Rápido" na web
2. Aplicação publica "fast_red" em carlos/rgb_pwm
3. ESP32 recebe e muda para MODE_FAST_SINGLE_COLOR
4. ESP32 publica status em carlos/rgb_pwm/status:
{"status":"fast_red","brightness":100}
5. Aplicação web recebe e atualiza a interface



# Como Executar o Projeto
Para replicar e executar este projeto, siga os passos abaixo. Você precisará de acesso ao hardware e às ferramentas de desenvolvimento.

## Ferramentas Necessárias 🛠️
1. ESP32: Uma placa de desenvolvimento ESP32.
2. Ambiente de Desenvolvimento (IDE): Visual Studio Code com a extensão PlatformIO ou a IDE do Arduino.
3. Fita de LED RGB (24V): Com um pino de controle para cada cor (R, G, B).
4. Hardware de Controle: Um circuito com transistores MOSFET (como o IRF520 ou similar) para controlar a fita de LED de 24V com os sinais de 3.3V do ESP32.
5. Internet: Para que o ESP32 e a aplicação web possam se comunicar com o broker MQTT.

# Passos para Configuração 💻
## 1. Configuração do Firmware (Código do ESP32)
Baixe o Código: Faça o download do arquivo de firmware (geralmente um arquivo .ino se estiver usando a IDE do Arduino, ou o projeto completo se estiver em PlatformIO).

Configure o Wi-Fi: Abra o código e atualize as variáveis WIFI_SSID e WIFI_PASSWORD com as credenciais da sua rede Wi-Fi.

C++

_#define WIFI_SSID "Seu_Nome_Da_Rede"
#define WIFI_PASSWORD "Sua_Senha"_

### Instale as Bibliotecas: Certifique-se de que as seguintes bibliotecas estejam instaladas no seu IDE:
 - PubSubClient (para comunicação MQTT)
 - ArduinoJson (para processar as mensagens JSON)
Faça o Upload: Conecte o ESP32 ao seu computador e faça o upload do código para a placa.

## 2. Configuração do Hardware
Conexões de Energia: Conecte a fonte de 24V aos pinos de energia da fita de LED.

Conexões dos Transistores:
- Conecte o pino de controle de cada cor (R, G, B) do ESP32 aos pinos Gate (G) dos transistores MOSFET.
- Conecte o pino Drain (D) de cada transistor ao pino correspondente na fita de LED (R, G, B).
- Conecte o pino Source (S) de cada transistor ao GND da fonte de 24V.
- Alimentação do ESP32: Alimente o ESP32 via USB ou por uma fonte de 5V.

## 3. Configuração da Interface Web (Frontend)
### Hospedagem:
Você pode hospedar o arquivo index.html em um servidor local (usando Live Server do VS Code, por exemplo).
Para acesso público, hospede o arquivo em uma plataforma como o Vercel ou o Netlify. Ambas oferecem planos gratuitos e são ideais para projetos estáticos.
### Broker MQTT: 
O projeto já está configurado para usar o broker público e gratuito _broker.hivemq.com_. Não é necessária nenhuma configuração adicional, pois o frontend e o ESP32 se conectarão a ele automaticamente.
