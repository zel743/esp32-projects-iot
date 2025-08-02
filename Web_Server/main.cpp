#include <WiFi.h>
#include <WebServer.h>

// Configuración del Access Point (AP)
const char* apSSID = "ESP32_AP";
const char* apPassword = "12345678";

// Pines
const int ledPin = 13;
const int buttonPin = 4;

// Variables
bool ledState = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

WebServer server(80);

// --- Declaración anticipada de funciones ---
void handleGetState();
void handleRoot();
void handleToggle();
void toggleLED();

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  // Crear Access Point
  WiFi.softAP(apSSID, apPassword);
  Serial.println("\nAccess Point creado");
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.softAPIP());

  // Rutas del servidor web
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/getState", handleGetState); // Mover esta línea aquí
  server.begin();
}

// Implementación de handleGetState (fuera de setup!)
void handleGetState() {
  String json = "{\"state\": " + String(ledState ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}
void loop() {
  server.handleClient();  // Manejar peticiones web

  // Control del botón físico (con debounce)
  bool reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {  // Botón presionado (pull-up)
      toggleLED();
      delay(300);  // Anti-rebote adicional
    }
  }
  lastButtonState = reading;
}

void toggleLED() {
  ledState = !ledState;
  digitalWrite(ledPin, ledState);
}

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Control LED</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body {font-family: Arial; text-align: center; margin-top: 50px;}
        .button {
          padding: 10px 20px;
          font-size: 20px;
          background-color: #4CAF50;
          color: white;
          border: none;
          border-radius: 5px;
          cursor: pointer;
        }
        .button.off {background-color: #f44336;}
      </style>
    </head>
    <body>
      <h1>Control LED</h1>
      <p>Estado: <span id="state">)rawliteral";
  html += (ledState ? "ENCENDIDO" : "APAGADO");
  html += R"rawliteral(</span></p>
      <button class="button )rawliteral";
  html += (ledState ? "off" : "");
  html += R"rawliteral(" onclick="toggleLED()">)rawliteral";
  html += (ledState ? "APAGAR" : "ENCENDER");
  html += R"rawliteral(</button>
      <script>
        function updateLEDState() {
          fetch("/getState")
            .then(response => response.json())
            .then(data => {
              document.getElementById("state").textContent = data.state ? "ENCENDIDO" : "APAGADO";
              const button = document.querySelector(".button");
              button.textContent = data.state ? "APAGAR" : "ENCENDER";
              button.className = data.state ? "button off" : "button";
            });
        }

        function toggleLED() {
          fetch("/toggle").then(() => updateLEDState());
        }

        // Consultar el estado cada 500ms
        setInterval(updateLEDState, 500);
      </script>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}
void handleToggle() {
  toggleLED();
  server.send(200, "text/plain", "OK");
}