/* Ejercicio entregable 001
 Deben de utilizar este archivo los alumnos con c<5, d<5, u>=5
 siendo c,d,u las tres últimas cifras del DNI 22000cdu -W
 Luminosidad y motor paso a paso con potenciometro
 Para cambiar Luminosidad o haz click sobre el sensor NTC durante la simulacion
 rellenar vuestro nombre y DNI
 NOMBRE ALUMNO: Alejandra
 DNI: 20973016Y
 ENLACE WOKWI: https://wokwi.com/projects/451877236852336641
*/
#include <Stepper.h>
#include <WiFi.h>          
#include <PubSubClient.h>

const int PIN_LDR = 35;          // El sensor de luz está en el pin 35
const int PIN_POTENCIO = 34;     // El potenciómetro está en el pin 34
Stepper myStepper(200, 14,27 , 26, 25);
int posicionActual = 0;

int historicoLuz[5] = {0, 0, 0, 0, 0}; // Caja para guardar 5 valores
unsigned long ultimaHora = 0;          // Para contar el tiempo
int indiceHistorico = 0;               // Para saber en qué caja toca guardar

const char* ssid = "Wokwi-GUEST";       
const char* password = "";              
const char* mqtt_server = "broker.emqx.io"; 

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("¡WiFi conectado!");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
  myStepper.setSpeed(60);

  setup_wifi();                           
  client.setServer(mqtt_server, 1883);    
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Intentamos conectar
    if (client.connect("ESP32_Alejandra_016")) {
      Serial.println("¡Conectado al servidor!");
    } else {
      Serial.print("falló con estado ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int lecturaLDR = analogRead(PIN_LDR);
  int luzPorcentaje = map(lecturaLDR, 4095, 0, 0, 100);
  luzPorcentaje = constrain(luzPorcentaje, 0, 100);

  Serial.print("Luz en el invernadero: ");
  Serial.print(luzPorcentaje);
  Serial.println("%");

  // 0% luz = 0 pasos | 100% luz = 50 pasos (que son 90 grados)
  int pasoObjetivo = map(luzPorcentaje, 0, 100, 0, 50);
  int pasosAMover = pasoObjetivo - posicionActual;
  if (pasosAMover != 0) {
  myStepper.step(pasosAMover);
  posicionActual = pasoObjetivo;
  }
  
  int lecturaPot = analogRead(PIN_POTENCIO);
  float gradosReales = map(lecturaPot, 0, 4095, 0, 360);

  Serial.print("Paso Motor: "); Serial.print(posicionActual);
  Serial.print(" | Grados Reales (Pot): "); Serial.println(gradosReales);

  String mensaje = String(luzPorcentaje);
  client.publish("sja003/invernadero/016/luz", mensaje.c_str());

  if (millis() - ultimaHora > 10000) { 
    ultimaHora = millis(); // Reiniciamos el cronómetro
    
    // 1. Guardamos el valor actual en la posición que toca
    historicoLuz[indiceHistorico] = luzPorcentaje;
    
    // 2. Creamos el mensaje combinando los 5 valores: "H:v1,v2,v3,v4,v5"
    String msgH = "H:";
    for (int i = 0; i < 5; i++) {
      msgH += String(historicoLuz[i]);
      if (i < 4) msgH += ","; // Añade coma entre números, pero no al final
    }

    // 3. Enviamos el histórico a su propio canal (topic)
    client.publish("sja003/invernadero/016/historico", msgH.c_str());
    
    // Mostramos en pantalla para confirmar
    Serial.print("--> HISTÓRICO ENVIADO: ");
    Serial.println(msgH);

    // 4. Movemos el índice a la siguiente "caja" (del 0 al 4)
    indiceHistorico++;
    if (indiceHistorico > 4) indiceHistorico = 0; 
  }
  delay(500);
}
  