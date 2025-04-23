// ==================== BIBLIOTECAS =====================
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <time.h>

// ==================== DEFINIÇÕES =====================
const char* ssid = "S24 de Bruno";
const char* password = "12345678";

#define DHTPIN 21
#define DHTTYPE DHT11
#define RELAY_PIN 5
#define LED_RED 13
#define LED_YELLOW 27
#define LED_GREEN 14
#define TEMP_THRESHOLD 23.0

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

String logHTML = "";
String csvLog = "Hora,Temperatura (C),Umidade (%),Rele,Tempo Resposta (ms)\n";

float temp = 0.0;
float hum = 0.0;
bool estadoRele = false;
unsigned long lastLogMillis = 0;

// ==================== INTERFACE =====================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 Monitoramento</title>";
  html += "<style>body{font-family:Arial;text-align:center;padding:20px;} .status{font-size:1.2em;} .led{width:30px;height:30px;border-radius:50%;display:inline-block;margin:5px;} .on{filter:brightness(100%);} .off{filter:brightness(30%);} .led-r{background:red;} .led-y{background:yellow;} .led-g{background:green;} canvas{max-width:300px;margin-top:20px;}</style>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script></head><body>";
  html += "<h1>ESP32 - Monitoramento</h1>";
  html += "<p><strong>Temperatura:</strong> <span id='temp'>--</span></p>";
  html += "<p><strong>Umidade:</strong> <span id='hum'>--</span></p>";
  html += "<p><strong>Relê:</strong> <span id='rele'>--</span></p>";
  html += "<div><span class='led led-r' id='lr'></span><span class='led led-y' id='ly'></span><span class='led led-g' id='lg'></span></div>";
  html += "<canvas id='grafico'></canvas>";
  html += "<button onclick=\"window.location.href='/download'\">Baixar CSV do Log</button>";
  html += "<script>let tempData=[];let timeData=[];let chart=new Chart(document.getElementById('grafico'),{type:'line',data:{labels:timeData,datasets:[{label:'Temp (°C)',data:tempData,borderColor:'blue'}]},options:{scales:{y:{beginAtZero:true}}}});setInterval(()=>{fetch('/dados').then(r=>r.json()).then(d=>{document.getElementById('temp').innerText=d.temp.toFixed(1);document.getElementById('hum').innerText=d.hum.toFixed(1);document.getElementById('rele').innerText=d.rele?'DESLIGADO':'LIGADO';['r','y','g'].forEach((l,i)=>{document.getElementById('l'+l).className='led led-'+l+(d.semaforo==i?' on':' off')});let now=new Date().toLocaleTimeString();timeData.push(now);tempData.push(d.temp);if(tempData.length>10){tempData.shift();timeData.shift();}chart.update();});},5000);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Iniciando ESP32...");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, LOW);

  dht.begin();
  Serial.println("Sensor DHT iniciado.");

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP());

  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  server.on("/", handleRoot);
  server.on("/dados", handleDados);
  server.on("/download", handleDownload);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastLogMillis >= 5000) {
    lastLogMillis = now;
    unsigned long tStart = millis();

    temp = dht.readTemperature();
    hum = dht.readHumidity();

    if (!isnan(temp)) {
      if (temp >= TEMP_THRESHOLD) {
        digitalWrite(RELAY_PIN, HIGH);
        estadoRele = false;
      } else {
        digitalWrite(RELAY_PIN, LOW);
        estadoRele = true;
      }
    }

    atualizaSemaforo(temp);

    struct tm timeinfo;
    getLocalTime(&timeinfo);
    char hora[20];
    strftime(hora, sizeof(hora), "%H:%M:%S", &timeinfo);

    String estadoStr = estadoRele ? "DESLIGADO" : "LIGADO";
    unsigned long tResp = millis() - tStart;
    String novaLinha = "<tr><td>" + String(hora) + "</td><td>" + String(temp, 1) + "</td><td>" + String(hum, 1) + "</td><td>" + estadoStr + "</td><td>" + String(tResp) + "ms</td></tr>";

    csvLog += String(hora) + "," + String(temp, 1) + "," + String(hum, 1) + "," + estadoStr + "," + String(tResp) + "ms\n";
    logHTML = novaLinha + logHTML;
  }
}

