#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Configurações de WiFi
const char* ssid = "CARLINHOS_VCNET";
const char* password = "21012004Cg@#1";

// Configurações MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "carlos/rgb_pwm";
const char* mqtt_status_topic = "carlos/rgb_pwm/status";

#define JSON_BUFFER_SIZE 256

// Pinos dos LEDs RGB (usando PWM)
#define PIN_RED   14
#define PIN_GREEN 27
#define PIN_BLUE  26

// Modos de operação
#define MODE_OFF 0
#define MODE_FAST_COLOR_CHANGE 1
#define MODE_SLOW_COLOR_CHANGE 2
#define MODE_FAST_SINGLE_COLOR 3
#define MODE_CUSTOM_SEQUENCE 4
#define MODE_BRIGHTNESS_CONTROL 5 

// Timeouts
#define WIFI_TIMEOUT_MS 20000
#define MQTT_TIMEOUT_MS 5000

// Variáveis de estado
int currentMode = MODE_OFF;
unsigned long lastChangeTime = 0;
int currentColorIndex = 0;
bool shouldPublishStatus = false;

// Variáveis adicionais
int currentBrightness = 100; // 0-100%
int sequenceSpeed = 1000; // ms entre transições
int customSequence[10][3]; // Armazena até 10 cores personalizadas
int sequenceLength = 0;

// Cores pré-definidas (R, G, B)
const int colors[][3] = {
  {255, 0, 0},    // Vermelho
  {0, 255, 0},    // Verde
  {0, 0, 255},    // Azul
  {255, 255, 0},  // Amarelo
  {0, 255, 255},  // Ciano
  {255, 0, 255}   // Magenta
};
const int colorCount = 6;

WiFiClient espClient;
PubSubClient client(espClient);

void setupPins() {
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  setColor(0, 0, 0); // Inicia com LEDs apagados
}

unsigned int safe_min(unsigned int a, int b) {
  return (a < (unsigned int)b) ? a : (unsigned int)b;
}

void setColor(int r, int g, int b) {
  r = r * currentBrightness / 100;
  g = g * currentBrightness / 100;
  b = b * currentBrightness / 100;
  
  analogWrite(PIN_RED,   255 - r);
  analogWrite(PIN_GREEN, 255 - g);
  analogWrite(PIN_BLUE,  255 - b);
}

void publishStatus() {
  DynamicJsonDocument doc(256);
  
  switch(currentMode) {
    case MODE_OFF: doc["status"] = "off"; break;
    case MODE_FAST_COLOR_CHANGE: doc["status"] = "fast_colors"; break;
    case MODE_SLOW_COLOR_CHANGE: doc["status"] = "slow_colors"; break;
    case MODE_FAST_SINGLE_COLOR: doc["status"] = "fast_red"; break;
    case MODE_CUSTOM_SEQUENCE: doc["status"] = "custom_sequence"; break;
    default: doc["status"] = "unknown";
  }
  
  doc["brightness"] = currentBrightness;
  doc["mode"] = currentMode;

  String payload;
  serializeJson(doc, payload);
  
  if (client.publish(mqtt_status_topic, payload.c_str())) {
    Serial.println("Status publicado: " + payload);
  } else {
    Serial.println("Falha ao publicar status");
  }
}

void setMode(int newMode) {
  if (currentMode != newMode) {
    currentMode = newMode;
    currentColorIndex = 0;
    lastChangeTime = millis();
    
    Serial.print("Modo alterado para: ");
    switch(currentMode) {
      case MODE_OFF: 
        Serial.println("OFF");
        setColor(0, 0, 0);
        break;
      case MODE_FAST_COLOR_CHANGE: 
        Serial.println("Mudança rápida de cores");
        break;
      case MODE_SLOW_COLOR_CHANGE: 
        Serial.println("Mudança lenta de cores");
        break;
      case MODE_FAST_SINGLE_COLOR: 
        Serial.println("Piscar rápido em uma cor");
        break;
    }
    
    shouldPublishStatus = true;
  }
}

void handleCustomSequence() {
  static int sequenceIndex = 0;
  static unsigned long lastChange = 0;
  
  if (millis() - lastChange > sequenceSpeed) {
    sequenceIndex = (sequenceIndex + 1) % sequenceLength;
    
    int r = customSequence[sequenceIndex][0] * currentBrightness / 100;
    int g = customSequence[sequenceIndex][1] * currentBrightness / 100;
    int b = customSequence[sequenceIndex][2] * currentBrightness / 100;
    
    setColor(r, g, b);
    lastChange = millis();
  }
}

void handleFastColorChange() {
  if (millis() - lastChangeTime > 300) { // 300ms
    currentColorIndex = (currentColorIndex + 1) % colorCount;
    setColor(colors[currentColorIndex][0], 
             colors[currentColorIndex][1], 
             colors[currentColorIndex][2]);
    lastChangeTime = millis();
  }
}

void handleSlowColorChange() {
  if (millis() - lastChangeTime > 1000) { // 1000ms
    currentColorIndex = (currentColorIndex + 1) % colorCount;
    setColor(colors[currentColorIndex][0], 
             colors[currentColorIndex][1], 
             colors[currentColorIndex][2]);
    lastChangeTime = millis();
  }
}

void handleFastSingleColor() {
  if (millis() - lastChangeTime > 150) { // 150ms
    if (currentColorIndex == 0) {
      setColor(255, 0, 0); // Vermelho
      currentColorIndex = 1;
    } else {
      setColor(0, 0, 0);   // Apagado
      currentColorIndex = 0;
    }
    lastChangeTime = millis();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Cria buffer para a mensagem
  char message[JSON_BUFFER_SIZE];
  unsigned int copy_len = safe_min(length, sizeof(message) - 1);
  strncpy(message, (char*)payload, copy_len);
  message[copy_len] = '\0';

  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Verifica se é JSON
  if (message[0] == '{') {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      Serial.print("Erro ao parsear JSON: ");
      Serial.println(error.c_str());
      return;
    }

    // Controle de brilho
    if (doc.containsKey("brightness")) {
      currentBrightness = doc["brightness"].as<int>();
      Serial.println("Brilho alterado para: " + String(currentBrightness) + "%");
      
      // Aplica o brilho imediatamente
      if (currentMode == MODE_OFF) {
        setColor(0, 0, 0);
      } else if (currentMode == MODE_FAST_SINGLE_COLOR) {
        setColor(255 * currentBrightness / 100, 0, 0);
      }
      publishStatus();
      return;
    }

    // Sequência personalizada
    if (doc.containsKey("sequence")) {
      sequenceLength = doc["sequence"].size();
      if (sequenceLength > 10) sequenceLength = 10;
      
      for (int i = 0; i < sequenceLength; i++) {
        customSequence[i][0] = doc["sequence"][i]["r"] | 0;
        customSequence[i][1] = doc["sequence"][i]["g"] | 0;
        customSequence[i][2] = doc["sequence"][i]["b"] | 0;
      }
      
      sequenceSpeed = doc["speed"] | 1000;
      setMode(MODE_CUSTOM_SEQUENCE);
      Serial.println("Nova sequência recebida com " + String(sequenceLength) + " cores");
      publishStatus();
      return;
    }
  }

  // Processar comandos simples como String
  String msgStr(message);
  msgStr.trim(); // Remove espaços em branco

  if (msgStr == "fast_colors") {
    setMode(MODE_FAST_COLOR_CHANGE);
    publishStatus();
  } else if (msgStr == "slow_colors") {
    setMode(MODE_SLOW_COLOR_CHANGE);
    publishStatus();
  } else if (msgStr == "fast_red") {
    setMode(MODE_FAST_SINGLE_COLOR);
    publishStatus();
  } else if (msgStr == "off") {
    setMode(MODE_OFF);
    publishStatus();
  } else if (msgStr == "get_status") {
    publishStatus(); // Responde imediatamente com o status
  } else {
    Serial.println("Comando inválido: " + msgStr);
  }
}

}

void setupWiFi() {
  Serial.print("Conectando ao WiFi ");
  WiFi.begin(ssid, password, 6); // Canal 6 para melhor performance
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha na conexão WiFi");
    ESP.restart();
  }
}

void reconnectMQTT() {
  unsigned long startAttemptTime = millis();
  
  while (!client.connected() && millis() - startAttemptTime < MQTT_TIMEOUT_MS) {
    Serial.print("Conectando ao MQTT...");
    
    if (client.connect("ESP32RGBClient")) {
      Serial.println("Conectado!");
      client.subscribe(mqtt_topic);
      shouldPublishStatus = true; // Publicar status ao reconectar
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
  
  if (!client.connected()) {
    Serial.println("Timeout na conexão MQTT");
  }
}

void setup() {
  pinMode(0, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  delay(1000);

  Serial.begin(115200);
  setupPins();
  
  setupWiFi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  client.setBufferSize(256); // Buffer suficiente para mensagens
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Publicar status se necessário
  if (shouldPublishStatus) {
    publishStatus();
    shouldPublishStatus = false;
  }

  // Gerenciar modos de operação
  switch(currentMode) {
    case MODE_FAST_COLOR_CHANGE:
      handleFastColorChange();
      break;
    case MODE_SLOW_COLOR_CHANGE:
      handleSlowColorChange();
      break;
    case MODE_FAST_SINGLE_COLOR:
      handleFastSingleColor();
      break;
    case MODE_CUSTOM_SEQUENCE:
      handleCustomSequence();
      break;
    case MODE_OFF:
    default:
      break;
  }
}