#include <Arduino.h>
#include <driver/ledc.h>
#include "BluetoothSerial.h"
#include <WiFi.h> // Se incluye pero no se usa, lo mantengo para consistencia con tu código original.

// --- DECLARACIONES Y CONSTANTES ---
BluetoothSerial SerialBT;
// Pines del sensor ultrasónico
const int trigPin = 27;
const int echoPin = 17;
// Pines de control del puente H (motores)
const int IN1 = 18; // Motor Izquierdo, dirección A
const int IN2 = 19; // Motor Izquierdo, dirección B
const int IN3 = 23; // Motor Derecho, dirección A
const int IN4 = 5;  // Motor Derecho, dirección B
// Pines de los LEDs de estado
const int ledRojo = 2;
const int ledVerde = 14;

// Configuración de PWM para los motores
const int pwmFreq = 30000;
const ledc_timer_t pwmTimer = LEDC_TIMER_0;
const ledc_mode_t pwmMode = LEDC_HIGH_SPEED_MODE;
const ledc_timer_bit_t pwmResolution = LEDC_TIMER_8_BIT; // 0-255 de resolución
// Canales PWM para cada pin
const int pwmChannel_IN1 = LEDC_CHANNEL_0;
const int pwmChannel_IN2 = LEDC_CHANNEL_1;
const int pwmChannel_IN3 = LEDC_CHANNEL_2;
const int pwmChannel_IN4 = LEDC_CHANNEL_3;

// --- VARIABLES GLOBALES ---
int velocidad = 255;
bool autoEvasion = false; // Inicia en modo manual, se activa con un comando
int distancia = 0;
bool enMovimiento = false; // Bandera para el estado de los LEDs

// Variables para la máquina de estados de evasión
bool evadiendo = false;
int pasoEvasion = 0;
unsigned long tiempoInicioPaso = 0;
char ultimoComandoManual = 'S'; // Almacena el último comando de movimiento manual

// --- PROTOTIPOS DE FUNCIONES ---
void procesarComando(char comando);
void reanudarMovimientoManual();
void manejarEvasionStateMachine();
void actualizarLEDs();
void detenerMotores();
void moverAdelante();
void moverAtras();
void girarIzquierda();
void girarDerecha();

// --- FUNCIONES DE MOVIMIENTO (USANDO PWM) ---
// Configura el ciclo de trabajo (duty_cycle) para un canal PWM
void setMotorPWM(int channel, int duty_cycle) {
  ledc_set_duty(pwmMode, (ledc_channel_t)channel, duty_cycle);
  ledc_update_duty(pwmMode, (ledc_channel_t)channel);
}

// Funciones para los movimientos básicos
void moverAdelante() {
  setMotorPWM(pwmChannel_IN1, velocidad);
  setMotorPWM(pwmChannel_IN2, 0);
  setMotorPWM(pwmChannel_IN3, velocidad);
  setMotorPWM(pwmChannel_IN4, 0);
  enMovimiento = true;
}

void moverAtras() {
  setMotorPWM(pwmChannel_IN1, 0);
  setMotorPWM(pwmChannel_IN2, velocidad);
  setMotorPWM(pwmChannel_IN3, 0);
  setMotorPWM(pwmChannel_IN4, velocidad);
  enMovimiento = true;
}

void girarIzquierda() {
  setMotorPWM(pwmChannel_IN1, 0);
  setMotorPWM(pwmChannel_IN2, velocidad);
  setMotorPWM(pwmChannel_IN3, velocidad);
  setMotorPWM(pwmChannel_IN4, 0);
  enMovimiento = true;
}

void girarDerecha() {
  setMotorPWM(pwmChannel_IN1, velocidad);
  setMotorPWM(pwmChannel_IN2, 0);
  setMotorPWM(pwmChannel_IN3, 0);
  setMotorPWM(pwmChannel_IN4, velocidad);
  enMovimiento = true;
}

void detenerMotores() {
  setMotorPWM(pwmChannel_IN1, 0);
  setMotorPWM(pwmChannel_IN2, 0);
  setMotorPWM(pwmChannel_IN3, 0);
  setMotorPWM(pwmChannel_IN4, 0);
  enMovimiento = false;
}


// --- SETUP ---
void setup() {
  delay(1000);
  Serial.begin(115200);
  
  // Configurar pines de sensor y LEDs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  
  // Configuración de los temporizadores y canales PWM del ESP32
  ledc_timer_config_t timer_conf = { 
    .speed_mode = pwmMode, 
    .duty_resolution = pwmResolution, 
    .timer_num = pwmTimer, 
    .freq_hz = pwmFreq, 
    .clk_cfg = LEDC_AUTO_CLK 
  };
  ledc_timer_config(&timer_conf);

  // Configuración de los canales PWM para cada pin del puente H
  ledc_channel_config_t ledc_conf_in1 = { .gpio_num = IN1, .speed_mode = pwmMode, .channel = (ledc_channel_t)pwmChannel_IN1, .timer_sel = pwmTimer, .duty = 0 };
  ledc_channel_config(&ledc_conf_in1);
  ledc_channel_config_t ledc_conf_in2 = { .gpio_num = IN2, .speed_mode = pwmMode, .channel = (ledc_channel_t)pwmChannel_IN2, .timer_sel = pwmTimer, .duty = 0 };
  ledc_channel_config(&ledc_conf_in2);
  ledc_channel_config_t ledc_conf_in3 = { .gpio_num = IN3, .speed_mode = pwmMode, .channel = (ledc_channel_t)pwmChannel_IN3, .timer_sel = pwmTimer, .duty = 0 };
  ledc_channel_config(&ledc_conf_in3);
  ledc_channel_config_t ledc_conf_in4 = { .gpio_num = IN4, .speed_mode = pwmMode, .channel = (ledc_channel_t)pwmChannel_IN4, .timer_sel = pwmTimer, .duty = 0 };
  ledc_channel_config(&ledc_conf_in4);
  
  detenerMotores();
  actualizarLEDs();
  SerialBT.begin("4Ruedas_ESP32");
  Serial.println("--- Robot listo para operar ---");
}

// --- LOOP PRINCIPAL ---
void loop() {
  // 1. Procesar comandos Bluetooth/Serial
  if (SerialBT.available()) { procesarComando(SerialBT.read()); }
  if (Serial.available()) { procesarComando(Serial.read()); }

  // 2. Medir distancia con el sensor ultrasónico
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duracion = pulseIn(echoPin, HIGH, 30000);
  distancia = duracion * 0.034 / 2;

  // 3. Lógica principal: manejar la evasión o detectar nuevos obstáculos
  if (evadiendo) {
    manejarEvasionStateMachine();
  } else {
    // Si estamos en modo de evasión automática, el último comando fue "adelante"
    // y detectamos un obstáculo real (distancia > 0 y cerca), iniciamos la evasión.
    if (autoEvasion && ultimoComandoManual == 'F' && distancia > 0 && distancia < 20) {
      Serial.print("¡Obstáculo a ");
      Serial.print(distancia);
      Serial.println(" cm! Iniciando evasión.");
      evadiendo = true;
      pasoEvasion = 0;
      // Llamamos a la función de la máquina de estados inmediatamente
      // para que el robot se detenga en este mismo ciclo del loop.
      manejarEvasionStateMachine(); 
    }
  }
  delay(20);
}


// --- FUNCIONES AUXILIARES ---

// Actualiza el estado de los LEDs según si el robot se está moviendo
void actualizarLEDs() {
  digitalWrite(ledVerde, enMovimiento); // Verde encendido si se mueve
  digitalWrite(ledRojo, !enMovimiento); // Rojo encendido si está detenido
}

// Procesa un comando de carácter
void procesarComando(char comando) {
  char cmdUpper = toupper(comando);
  
  // Si estamos evadiendo, solo el comando de PARAR ('S') puede interrumpir.
  if (evadiendo && cmdUpper != 'S') {
    return;
  }
  
  switch (cmdUpper) {
    case 'F': moverAdelante(); ultimoComandoManual = 'F'; break;
    case 'B': moverAtras(); ultimoComandoManual = 'B'; break;
    case 'L': girarIzquierda(); ultimoComandoManual = 'L'; break;
    case 'R': girarDerecha(); ultimoComandoManual = 'R'; break;
    case 'S': detenerMotores(); ultimoComandoManual = 'S'; evadiendo = false; break;
    
    // Comando para activar/desactivar la evasión automática
    case 'A':
      autoEvasion = !autoEvasion;
      Serial.println(autoEvasion ? "Auto-evasion: ON" : "Auto-evasion: OFF");
      // Si se desactiva la auto-evasión, detenemos los motores inmediatamente
      if (!autoEvasion) {
        detenerMotores();
        ultimoComandoManual = 'S';
      }
      break;
    
    // Control de velocidad
    case '1': case '2': case '3': case '4': case '5': 
    case '6': case '7': case '8': case '9': case '0':
      if (cmdUpper == '1') velocidad = 28;
      if (cmdUpper == '2') velocidad = 56;
      if (cmdUpper == '3') velocidad = 85;
      if (cmdUpper == '4') velocidad = 113;
      if (cmdUpper == '5') velocidad = 141;
      if (cmdUpper == '6') velocidad = 170;
      if (cmdUpper == '7') velocidad = 198;
      if (cmdUpper == '8') velocidad = 226;
      if (cmdUpper == '9' || cmdUpper == '0') velocidad = 255;
      if (enMovimiento) { 
        reanudarMovimientoManual();
      } // Reaplicar el movimiento con la nueva velocidad
      break;

    default:
      return; // Ignorar comandos no reconocidos
  }
  
  actualizarLEDs(); // Actualizar los LEDs después de cada comando válido
}

// Reanuda el movimiento del robot después de un cambio de velocidad
void reanudarMovimientoManual() {
  procesarComando(ultimoComandoManual);
}

// Máquina de estados para la evasión de obstáculos
void manejarEvasionStateMachine() {
  unsigned long tiempoTranscurrido = millis() - tiempoInicioPaso;
  switch (pasoEvasion) {
    case 0: // Detenerse antes de la maniobra
      detenerMotores();
      pasoEvasion = 1;
      tiempoInicioPaso = millis();
      break;
    case 1: // Retroceder por 500ms
      moverAtras();
      if (tiempoTranscurrido >= 500) {
        pasoEvasion = 2;
        tiempoInicioPaso = millis();
      }
      break;
    case 2: // Girar por 400ms
      girarIzquierda();
      if (tiempoTranscurrido >= 400) {
        pasoEvasion = 3;
        tiempoInicioPaso = millis();
      }
      break;
    case 3: // Maniobra completada, reanudar
      detenerMotores();
      evadiendo = false;
      Serial.println("Evasión completada. Reanudando...");
      reanudarMovimientoManual();
      break;
  }
  actualizarLEDs();
}