#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <HX711.h>
#include <Ultrasonic.h>

// --- Configuración WiFi ---
const char* apSSID = "PetFeeder_AP";
const char* apPassword = "12345678";

// --- Pines ---
const int servoComidaPin = 14;
const int servoLimpiezaPin = 27;
const int bombaAguaPin = 26;
const int trigPin = 12;
const int echoPin = 13;
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 17;

// --- Objetos ---
Servo servoComida;
Servo servoLimpieza;
HX711 balanza;
Ultrasonic ultrasonic(trigPin, echoPin);

// --- Variables de estado ---
bool comidaEnPlato = false;
bool aguaEnDeposito = false;
float pesoActual = 0;
const float pesoUmbral = 10.0;

// --- WebServer ---
WebServer server(80);

// --- Declaraciones anticipadas de funciones ---
void handleRoot();
void handleDispensarComida();
void handleDispensarAgua();
void handleLimpiarPlato();
void handleGetStatus();
void leerSensores();
void moverServoComida();
void moverServoLimpieza();
void activarBombaAgua();

// --- Variables para temporización no bloqueante ---
unsigned long servoComidaStartTime = 0;
bool servoComidaEnMovimiento = false;
unsigned long servoLimpiezaStartTime = 0;
bool servoLimpiezaEnMovimiento = false;
unsigned long bombaStartTime = 0;
bool bombaEnMovimiento = false;

void setup() {
  Serial.begin(115200);

  // Inicializar HX711
  balanza.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  balanza.set_scale(-2075.0);
  balanza.tare();

  // Inicializar servos y pines
  servoComida.attach(servoComidaPin);
  servoLimpieza.attach(servoLimpiezaPin);
  pinMode(bombaAguaPin, OUTPUT);

  // WiFi
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point creado");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Rutas del servidor web
  server.on("/", handleRoot);
  server.on("/dispensarComida", handleDispensarComida);
  server.on("/dispensarAgua", handleDispensarAgua);
  server.on("/limpiarPlato", handleLimpiarPlato);
  server.on("/getStatus", handleGetStatus);
  server.begin();
}

void loop() {
  server.handleClient();
  leerSensores();

  if (servoComidaEnMovimiento && (millis() - servoComidaStartTime > 700)) {
    servoComida.write(0);
    servoComidaEnMovimiento = false;
  }

  if (servoLimpiezaEnMovimiento && (millis() - servoLimpiezaStartTime > 700)) {
    servoLimpieza.write(0);
    servoLimpiezaEnMovimiento = false;
  }

  if (bombaEnMovimiento && (millis() - bombaStartTime > 2000)) {
    digitalWrite(bombaAguaPin, LOW);
    bombaEnMovimiento = false;
  }
}

// --- Implementaciones de funciones ---
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Pet Feeder</title><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body {font-family: Arial; text-align: center;} .button {padding: 10px 20px; font-size: 20px; margin: 10px;}</style>";
  html += "</head><body><h1>Dispensador de Mascotas</h1>";
  html += "<button class='button' onclick=\"fetch('/dispensarComida')\">Dispensar Comida</button>";
  html += "<button class='button' onclick=\"fetch('/dispensarAgua')\">Dispensar Agua</button>";
  html += "<button class='button' onclick=\"fetch('/limpiarPlato')\">Limpiar Plato</button>";
  html += "<h2>Estado Sensores</h2><div id='status'></div>";
  html += "<script>function updateStatus() {fetch('/getStatus').then(r => r.json()).then(data => {";
  html += "document.getElementById('status').innerHTML = 'Comida: ' + (data.comida ? 'Sí' : 'No') + '<br>Agua: ' + (data.agua ? 'Sí' : 'No');";
  html += "});} setInterval(updateStatus, 1000); updateStatus();</script></body></html>";
  server.send(200, "text/html", html);
}

void handleDispensarComida() {
  moverServoComida();
  server.send(200, "text/plain", "Comida dispensada");
}

void handleDispensarAgua() {
  activarBombaAgua();
  server.send(200, "text/plain", "Agua dispensada");
}

void handleLimpiarPlato() {
  moverServoLimpieza();
  server.send(200, "text/plain", "Plato limpiado");
}

void handleGetStatus() {
  String json = "{\"comida\": " + String(comidaEnPlato ? "true" : "false") + ", \"agua\": " + String(aguaEnDeposito ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void leerSensores() {
  pesoActual = balanza.get_units(1);
  comidaEnPlato = (pesoActual > pesoUmbral);
  
  float distancia = ultrasonic.read();
  aguaEnDeposito = (distancia < 10.0);

  Serial.print("Peso: "); Serial.print(pesoActual); Serial.println(" g");
  Serial.print("Distancia agua: "); Serial.print(distancia); Serial.println(" cm");
}

void moverServoComida() {
  if (!servoComidaEnMovimiento) {
    servoComida.write(180);
    servoComidaStartTime = millis();
    servoComidaEnMovimiento = true;
  }
}

void moverServoLimpieza() {
  if (!servoLimpiezaEnMovimiento) {
    servoLimpieza.write(180);
    servoLimpiezaStartTime = millis();
    servoLimpiezaEnMovimiento = true;
  }
}

void activarBombaAgua() {
  if (!bombaEnMovimiento) {
    digitalWrite(bombaAguaPin, HIGH);
    bombaStartTime = millis();
    bombaEnMovimiento = true;
  }
}