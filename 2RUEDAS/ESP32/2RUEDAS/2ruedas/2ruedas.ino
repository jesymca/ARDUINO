#include <BluetoothSerial.h>
#include "ESP32Servo.h"

// Pines del puente H L298N
const int M_IZQ_IN1 = 12;
const int M_IZQ_IN2 = 14;
const int M_IZQ_ENA = 25; // PWM motor izquierdo

const int M_DER_IN3 = 16;
const int M_DER_IN4 = 17;
const int M_DER_ENB = 26; // PWM motor derecho

// Pines del sensor ultrasónico
const int TRIG_PIN = 23;
const int ECHO_PIN = 19;

// Pin del servomotor
const int SERVO_PIN = 27;

// Configuración Bluetooth
BluetoothSerial SerialBT;
String device_name = "2_RUEDAS_ESP32";

// Velocidad de los motores (0-255)
int velocidad = 100; // Velocidad inicial, se ajustará a 70 en modo autónomo
const int DISTANCIA_SEGURIDAD = 30; // cm
// TIEMPO_ESPERA reducido para mayor reactividad en evasión
const int TIEMPO_ESPERA = 300; // ms entre movimientos

// Factor para la velocidad de giro en modo de avance/corrección (0.0 a 1.0)
// 1.0 = giro en el sitio (pivot), 0.0 = una rueda parada (giro más cerrado con avance)
// Un valor como 0.5 hará que una rueda vaya a la mitad de velocidad, permitiendo un giro suave con avance.
const float TURN_FACTOR = 0.5;

// Variables para el control de movimiento
bool modoAutomatico = false;
bool obstaculoDetectado = false; // Bandera para indicar si el robot está en proceso de evasión

// Objeto Servo
Servo miServo;

void setup() {
  // Configurar pines de motor como salidas
  pinMode(M_IZQ_IN1, OUTPUT);
  pinMode(M_IZQ_IN2, OUTPUT);
  pinMode(M_IZQ_ENA, OUTPUT);
   
  pinMode(M_DER_IN3, OUTPUT);
  pinMode(M_DER_IN4, OUTPUT);
  pinMode(M_DER_ENB, OUTPUT);

  // Configurar pines del ultrasónico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Configurar servo
  miServo.attach(SERVO_PIN);
  miServo.write(90); // Posición central
   
  // Inicializar Bluetooth
  SerialBT.begin(device_name, true); // Modo caché activado para mejor rendimiento
  Serial.begin(115200);
  Serial.println("Dispositivo Bluetooth iniciado");
  SerialBT.setTimeout(10); // Reduce el timeout por defecto para mayor reactividad

  // Detener motores al inicio
  detenerMotores();
}

void loop() {
  // Verificar comandos Bluetooth
  if (SerialBT.available()) {
    char comando = SerialBT.read();
    Serial.println("Comando recibido: " + String(comando));
    procesarComando(comando);
  }

  // Lógica del modo automático:
  // Solo verifica obstáculos si está en modo automático y NO está ya evadiendo un obstáculo.
  if (modoAutomatico && !obstaculoDetectado) {
    int distancia = medirDistancia();
     
    // Si se detecta un obstáculo dentro de la distancia de seguridad
    // y la lectura es válida (mayor que 0, para evitar lecturas erróneas de 0 cm)
    if (distancia < DISTANCIA_SEGURIDAD && distancia > 0) {
      Serial.println("Obstáculo detectado a " + String(distancia) + " cm");
      detenerMotores(); // Detener antes de evadir
      evadirObstaculo(); // Llamar a la función de evasión
    } else {
      // Si no hay obstáculo y estamos en modo automático, seguir adelante
      // Esto asegura que el robot continúe moviéndose después de una evasión
      // o al iniciar el modo automático sin obstáculos inmediatos.
      moverAdelante(); 
    }
  }
}

void procesarComando(char comando) {
  switch(comando) {
    case 'F': 
      moverAdelante(); 
      modoAutomatico = false;
      obstaculoDetectado = false;
      break;
    case 'B': 
      moverAtras(); 
      modoAutomatico = false;
      break;
    case 'L': 
      girarIzquierda(); // Realiza un giro con avance (una rueda más lenta)
      modoAutomatico = false;
      break;
    case 'R': 
      girarDerecha(); // Realiza un giro con avance (una rueda más lenta)
      modoAutomatico = false;
      break;
    case 'S': 
      detenerMotores(); 
      modoAutomatico = false;
      obstaculoDetectado = false;
      break;
    // Nuevos comandos para corrección de rumbo
    case 'H': // Adelante y corrección a la Derecha
      moverAdelanteCorregirDerecha();
      modoAutomatico = false;
      obstaculoDetectado = false;
      break;
    case 'G': // Adelante y corrección a la Izquierda
      moverAdelanteCorregirIzquierda();
      modoAutomatico = false;
      obstaculoDetectado = false;
      break;
    case 'J': // Atrás y corrección a la Derecha
      moverAtrasCorregirDerecha();
      modoAutomatico = false;
      break;
    case 'I': // Atrás y corrección a la Izquierda
      moverAtrasCorregirIzquierda();
      modoAutomatico = false;
      break;
    case '0': velocidad = 70; break;
    case '1': velocidad = 80; break;
    case '2': velocidad = 90; break;
    case '3': velocidad = 100; break;
    case '4': velocidad = 130; break;
    case '5': velocidad = 150; break;
    case '6': velocidad = 170; break;
    case '7': velocidad = 180; break;
    case '8': velocidad = 190; break;
    case '9': velocidad = 255; break;
   // Control de modo automático (activación y desactivación)
    case 'X': // Activación estándar para modo automático
    case 'M': // Alternativa para activación de modo automático
      modoAutomatico = true;
      obstaculoDetectado = false; // Asegura que no hay obstáculo detectado al iniciar
      velocidad = 70; // Establecer velocidad a 70 al activar el modo autónomo
      moverAdelante(); // Empieza a moverse hacia adelante
      Serial.println("Modo automático ACTIVADO - Búsqueda de obstáculos, velocidad inicial: " + String(velocidad));
      break;
    case 'x': // Desactivación estándar para modo automático
    case 'm': // Alternativa para desactivación de modo automático
      modoAutomatico = false;
      obstaculoDetectado = false;
      detenerMotores();
      Serial.println("Modo automático DESACTIVADO");
      break;
  }
}

int medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
   
  long duracion = pulseIn(ECHO_PIN, HIGH, 30000); // Añadir timeout para evitar bloqueo
  int distancia = duracion * 0.034 / 2;
   
  return distancia;
}

void evadirObstaculo() {
  obstaculoDetectado = true; // Activa la bandera de evasión

  Serial.println("Iniciando evasión de obstáculo...");
   
  // Retroceder un poco para tomar distancia
  moverAtras();
  delay(500); // Tiempo para retroceder
  detenerMotores();
  delay(TIEMPO_ESPERA); // Pequeña pausa
   
  // Escanear entorno
  int distIzq = escanear(180); // Mirar a la izquierda
  int distDer = escanear(0);   // Mirar a la derecha
   
  miServo.write(90); // Volver al centro
  delay(TIEMPO_ESPERA); // Tiempo para que el servo se mueva y estabilice la lectura
   
  Serial.println("Distancias - Izq: " + String(distIzq) + " cm, Der: " + String(distDer) + " cm");
   
  // Decidir dirección de evasión
  if (distIzq > distDer && distIzq > DISTANCIA_SEGURIDAD) {
    Serial.println("Girando a la izquierda");
    girarIzquierda();
    delay(600); // Tiempo de giro
  } 
  else if (distDer > DISTANCIA_SEGURIDAD) {
    Serial.println("Girando a la derecha");
    girarDerecha();
    delay(600); // Tiempo de giro
  }
  else {
    Serial.println("No hay camino claro, retrocediendo y reevaluando");
    moverAtras();
    delay(1000); // Retrocede un poco más
    detenerMotores();
  }
  
  // Después de la evasión, el robot volverá a intentar moverse hacia adelante
  // si el modo automático sigue activado en la siguiente iteración del loop,
  // y la bandera obstaculoDetectado se reseteará después de esta función.
  obstaculoDetectado = false; // Resetea la bandera, permitiendo que el loop continúe buscando obstáculos
}

int escanear(int angulo) {
  miServo.write(angulo);
  delay(TIEMPO_ESPERA); // Tiempo para que el servo se mueva y estabilice la lectura
  return medirDistancia();
}

// Funciones de control de motores
void moverAdelante() {
  digitalWrite(M_IZQ_IN1, HIGH);
  digitalWrite(M_IZQ_IN2, LOW);
  digitalWrite(M_DER_IN3, HIGH);
  digitalWrite(M_DER_IN4, LOW);
   
  // Aplicar PWM simultáneamente
  analogWrite(M_IZQ_ENA, velocidad);
  analogWrite(M_DER_ENB, velocidad);
}

void moverAtras() {
  digitalWrite(M_IZQ_IN1, LOW);
  digitalWrite(M_IZQ_IN2, HIGH);
  digitalWrite(M_DER_IN3, LOW);
  digitalWrite(M_DER_IN4, HIGH);
   
  analogWrite(M_DER_ENB, velocidad);
  analogWrite(M_IZQ_ENA, velocidad);
}

// Modificación para girar con avance
void girarIzquierda() {
  // Rueda izquierda avanza a velocidad reducida, Rueda derecha avanza a velocidad completa
  digitalWrite(M_IZQ_IN1, HIGH);
  digitalWrite(M_IZQ_IN2, LOW);
  analogWrite(M_IZQ_ENA, velocidad * TURN_FACTOR); // Velocidad reducida para el giro

  digitalWrite(M_DER_IN3, HIGH);
  digitalWrite(M_DER_IN4, LOW);
  analogWrite(M_DER_ENB, velocidad); // Velocidad completa
}

// Modificación para girar con avance
void girarDerecha() {
  // Rueda izquierda avanza a velocidad completa, Rueda derecha avanza a velocidad reducida
  digitalWrite(M_IZQ_IN1, HIGH);
  digitalWrite(M_IZQ_IN2, LOW);
  analogWrite(M_IZQ_ENA, velocidad); // Velocidad completa

  digitalWrite(M_DER_IN3, HIGH);
  digitalWrite(M_DER_IN4, LOW);
  analogWrite(M_DER_ENB, velocidad * TURN_FACTOR); // Velocidad reducida para el giro
}

// Nuevas funciones para corrección de rumbo mientras se avanza/retrocede
void moverAdelanteCorregirDerecha() {
  // Rueda izquierda avanza a velocidad completa, Rueda derecha avanza a velocidad reducida
  digitalWrite(M_IZQ_IN1, HIGH);
  digitalWrite(M_IZQ_IN2, LOW);
  analogWrite(M_IZQ_ENA, velocidad); 

  digitalWrite(M_DER_IN3, HIGH);
  digitalWrite(M_DER_IN4, LOW);
  analogWrite(M_DER_ENB, velocidad * TURN_FACTOR); 
}

void moverAdelanteCorregirIzquierda() {
  // Rueda izquierda avanza a velocidad reducida, Rueda derecha avanza a velocidad completa
  digitalWrite(M_IZQ_IN1, HIGH);
  digitalWrite(M_IZQ_IN2, LOW);
  analogWrite(M_IZQ_ENA, velocidad * TURN_FACTOR); 

  digitalWrite(M_DER_IN3, HIGH);
  digitalWrite(M_DER_IN4, LOW);
  analogWrite(M_DER_ENB, velocidad); 
}

void moverAtrasCorregirDerecha() {
  // Rueda izquierda retrocede a velocidad reducida, Rueda derecha retrocede a velocidad completa
  digitalWrite(M_IZQ_IN1, LOW);
  digitalWrite(M_IZQ_IN2, HIGH);
  analogWrite(M_IZQ_ENA, velocidad * TURN_FACTOR); 

  digitalWrite(M_DER_IN3, LOW);
  digitalWrite(M_DER_IN4, HIGH);
  analogWrite(M_DER_ENB, velocidad); 
}

void moverAtrasCorregirIzquierda() {
  // Rueda izquierda retrocede a velocidad completa, Rueda derecha retrocede a velocidad reducida
  digitalWrite(M_IZQ_IN1, LOW);
  digitalWrite(M_IZQ_IN2, HIGH);
  analogWrite(M_IZQ_ENA, velocidad); 

  digitalWrite(M_DER_IN3, LOW);
  digitalWrite(M_DER_IN4, HIGH);
  analogWrite(M_DER_ENB, velocidad * TURN_FACTOR); 
}

void detenerMotores() {
  digitalWrite(M_IZQ_IN1, LOW);
  digitalWrite(M_IZQ_IN2, LOW);
  analogWrite(M_IZQ_ENA, 0);
   
  digitalWrite(M_DER_IN3, LOW);
  digitalWrite(M_DER_IN4, LOW);
  analogWrite(M_DER_ENB, 0);
}
