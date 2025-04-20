// bibliotecas usadas
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <time.h>

// definicoes de rede wifi e pinos
const char* ssid = "S24 de Bruno";
const char* password = "12345678";

#define DHTPIN 21
#define DHTTYPE DHT11
#define RELAY_PIN 5
#define LED_RED 13
#define LED_YELLOW 27
#define LED_GREEN 14
#define TEMP_THRESHOLD 23.0

// instancias dos componentes
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// variaveis globais
String logHTML = "";
String csvLog = "Hora,Temperatura (C),Umidade (%),Rele,Tempo Resposta (ms)\n";

float temp = 0.0;
float hum = 0.0;
bool estadoRele = false;
unsigned long lastLogMillis = 0;
void handleRoot() {
  // (html da interface, identico ao original)
}

void handleDados() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char hora[20];
  strftime(hora, sizeof(hora), "%H:%M:%S", &timeinfo);

  int status = 0;
  if (temp <= 23) status = 2;
  else if (temp <= 26) status = 1;
  else status = 0;

  String json = "{";
  json += "\"temp\":" + String(temp, 1) + ",";
  json += "\"hum\":" + String(hum, 1) + ",";
  json += "\"rele\":" + String(estadoRele ? "true" : "false") + ",";
  json += "\"hora\":\"" + String(hora) + "\",";
  json += "\"semaforo\":" + String(status);
  json += "}";
  server.send(200, "application/json", json);
}

void handleDownload() {
  server.send(200, "text/csv", csvLog);
}

void atualizaSemaforo(float temperatura) {
  digitalWrite(LED_GREEN, temperatura <= 23);
  digitalWrite(LED_YELLOW, temperatura > 23 && temperatura <= 26);
  digitalWrite(LED_RED, temperatura > 26);
}
