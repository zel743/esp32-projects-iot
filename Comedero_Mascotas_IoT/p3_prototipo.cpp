#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// --- Configuración WiFi ---
const char* apSSID = "PetFeeder_AP";
const char* apPassword = "12345678";

// --- Pines ---
const int servoComidaPin = 14;      // Servo para dispensar comida
const int servoLimpiezaPin = 27;    // Servo para limpiar plato
const int bombaAguaPin = 26;        // Bomba de agua (MOSFET o relevador)
const int sensorComidaPin = 32;     // Sensor IR TCRT5000 (comida)

// --- Objetos Servo ---
Servo servoComida;
Servo servoLimpieza;

// --- Variables de estado ---
bool comidaEnPlato = false;

// --- WebServer ---
WebServer server(80);

// --- Funciones anticipadas ---
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

  // Inicializar pines
  servoComida.attach(servoComidaPin);
  servoLimpieza.attach(servoLimpiezaPin);
  pinMode(bombaAguaPin, OUTPUT);
  digitalWrite(bombaAguaPin, HIGH); // Inicia con la bomba APAGADA (HIGH para lógica inversa)
  pinMode(sensorComidaPin, INPUT);

  // Crear Access Point
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point creado");
  Serial.print("IP del ESP32: ");
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

  // --- Control no bloqueante del servo de comida ---
  if (servoComidaEnMovimiento && (millis() - servoComidaStartTime > 700)) {
    servoComida.write(0); // Cerrar compuerta
    servoComidaEnMovimiento = false;
  }

  // --- Control no bloqueante del servo de limpieza ---
  if (servoLimpiezaEnMovimiento && (millis() - servoLimpiezaStartTime > 700)) {
    servoLimpieza.write(0); // Cerrar compuerta
    servoLimpiezaEnMovimiento = false;
  }

  // --- Control no bloqueante de la bomba ---
  if (bombaEnMovimiento && (millis() - bombaStartTime > 2000)) {
    digitalWrite(bombaAguaPin, HIGH); // Apagar bomba (HIGH para lógica inversa)
    bombaEnMovimiento = false;
  }
}

// --- Funciones de control ---
void moverServoComida() {
  if (!servoComidaEnMovimiento) {
    servoComida.write(45); // Abrir compuerta
    servoComidaStartTime = millis();
    servoComidaEnMovimiento = true;
  }
}

void moverServoLimpieza() {
  if (!servoLimpiezaEnMovimiento) {
    servoLimpieza.write(180); // Abrir compuerta de limpieza
    servoLimpiezaStartTime = millis();
    servoLimpiezaEnMovimiento = true;
  }
}

void activarBombaAgua() {
  if (!bombaEnMovimiento) {
    digitalWrite(bombaAguaPin, LOW); // Encender bomba (LOW para lógica inversa)
    bombaStartTime = millis();
    bombaEnMovimiento = true;
  }
}

// --- Lectura de sensores ---
void leerSensores() {
  // Solo lectura del sensor de comida (TCRT5000)
  comidaEnPlato = (digitalRead(sensorComidaPin) == LOW); // LOW = comida presente
}

// --- Handlers Web ---
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Pet Feeder</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body {font-family: Arial; text-align: center;}
        .button {padding: 10px 20px; font-size: 20px; margin: 10px;}
      </style>
    </head>
    <body>
      <h1>Dispensador de Mascotas</h1>
      <button class="button" onclick="fetch('/dispensarComida')">Dispensar Comida</button>
      <button class="button" onclick="fetch('/dispensarAgua')">Dispensar Agua</button>
      <button class="button" onclick="fetch('/limpiarPlato')">Limpiar Plato</button>
      <h2>Estado del Plato</h2>
      <div id="status"></div>
      <script>
        function updateStatus() {
          fetch('/getStatus')
            .then(r => r.json())
            .then(data => {
              document.getElementById('status').innerHTML =
                'Comida en plato: ' + (data.comida ? 'Si' : 'No');
            });
        }
        setInterval(updateStatus, 1000);
        updateStatus();
      </script>
    </body>
    </html>
  )rawliteral";
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
  String json = "{\"comida\": " + String(comidaEnPlato ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}