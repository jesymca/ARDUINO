#include <LiquidCrystal.h> // Incluye la librería para la pantalla LCD

// Definición de los pines para la pantalla LCD
// LiquidCrystal lcd(RS, Enable, D4, D5, D6, D7);
// Todos estos son pines DIGITALES
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Definición de los pines para el Sensor Ultrasónico HC-SR04
// Estos son pines DIGITALES (A0 y A1 se pueden usar como digitales)
const int trigPin = A0; // Pin Trigger del sensor
const int echoPin = A1; // Pin Echo del sensor

// Definición del pin para el control del motor de la bomba de agua
// Este es un pin DIGITAL
const int pumpPin = 7; // Conectado a la base del transistor NPN a través de una resistencia

// Definición de los pines para el LED RGB (deben ser pines PWM ~)
// Estos son pines DIGITALES PWM (con el símbolo ~ en la placa Arduino)
const int R_PIN = 9;  // Pin para el color Rojo (PWM Digital)
const int G_PIN = 10; // Pin para el color Verde (PWM Digital)
const int B_PIN = 6;  // Pin para el color Azul (PWM Digital)

// --- CALIBRACIÓN IMPORTANTE ---
// Esta es la distancia en CENTÍMETROS que el sensor debe leer cuando hay 200ml de agua.
// Necesitas CALIBRAR este valor para tu recipiente específico siguiendo los pasos anteriores.
// Si el sensor está arriba del recipiente, una distancia MENOR significa MÁS agua.
// ¡MODIFICA ESTE VALOR CON TU CALIBRACIÓN!
const float TARGET_WATER_DIST_CM = 15.0; // Valor de ejemplo, ¡Cámbialo!

// Variables para el cálculo de la distancia del sensor
long duration;     // Almacena la duración del pulso ultrasónico
float distanceCM;  // Almacena la distancia calculada en centímetros

// Variable para controlar el estado de la bomba
bool pumpIsOn = false;

// Variables para el efecto de fade del LED RGB (colores iniciales)
int rValue = 255;   // Valor inicial de Rojo (0-255)
int gValue = 0;     // Valor inicial de Verde
int bValue = 0;     // Valor inicial de Azul
int fadeStep = 5;   // Paso del fade (cuánto cambia el color en cada iteración)
int fadeState = 0;  // Estado del fade: 0=Rojo->Verde, 1=Verde->Azul, 2=Azul->Rojo

void setup() {
  Serial.begin(9600); // Inicia la comunicación serial para depuración (opcional, útil para ver distancias)

  // Configura la pantalla LCD
  lcd.begin(16, 2); // Inicializa LCD con 16 columnas y 2 filas
  lcd.print("Iniciando Proyecto");
  lcd.setCursor(0, 1);
  lcd.print("Bomba de Agua...");
  delay(2000); // Pequeña pausa para mostrar el mensaje inicial
  lcd.clear(); // Limpia la pantalla

  // Configura los pines del sensor ultrasónico
  pinMode(trigPin, OUTPUT); // Pin Trig como salida
  pinMode(echoPin, INPUT);  // Pin Echo como entrada

  // Configura el pin del motor de la bomba
  pinMode(pumpPin, OUTPUT);      // Pin de control de la bomba como salida
  digitalWrite(pumpPin, LOW);    // Asegúrate de que la bomba esté apagada al inicio

  // Configura los pines del LED RGB
  pinMode(R_PIN, OUTPUT); // Pin Rojo como salida (PWM)
  pinMode(G_PIN, OUTPUT); // Pin Verde como salida (PWM)
  pinMode(B_PIN, OUTPUT); // Pin Azul como salida (PWM)
  
  // Establece el color inicial del LED RGB
  analogWrite(R_PIN, rValue);
  analogWrite(G_PIN, gValue);
  analogWrite(B_PIN, bValue);

  lcd.print("Listo!");
  lcd.setCursor(0, 1);
  lcd.print("Esperando...");
  delay(1000);
}

void loop() {
  // --- Paso 1: Leer el Sensor Ultrasónico ---
  // Limpiar el pin Trig enviando un pulso bajo corto
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Enviar un pulso alto de 10 microsegundos para activar el sensor
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Medir la duración del pulso de Echo (tiempo que tarda la onda en ir y volver)
  // pulseIn() espera a que el pin pase a HIGH y luego a LOW, y devuelve la duración en microsegundos.
  duration = pulseIn(echoPin, HIGH);

  // Calcular la distancia en centímetros
  // (Velocidad del sonido en el aire = 343 metros/segundo = 0.0343 cm/microsegundo)
  // Dividir por 2 porque la onda viaja de ida y vuelta
  distanceCM = duration * 0.0343 / 2;

// Imprimir distancia en el Monitor Serial (para depuración y calibración)
  Serial.print("Distancia: ");
  Serial.print(distanceCM);
  Serial.println(" cm");

  // --- Paso 2: Controlar el Motor de la Bomba de Agua ---
  // Si la distancia medida es MAYOR que la distancia objetivo, significa que hay MENOS agua de la deseada
  // (porque el sensor está arriba, y una distancia mayor significa que el nivel de agua está más bajo)
  if (distanceCM > TARGET_WATER_DIST_CM) {
    // Si la bomba no está encendida, enciéndela
    if (!pumpIsOn) {
      digitalWrite(pumpPin, HIGH); // Enciende la bomba (HIGH activa el transistor NPN)
      pumpIsOn = true;
      lcd.clear();
      lcd.print("Llenando Agua...");
    }
    // Actualiza la pantalla con la distancia actual mientras se llena
    lcd.setCursor(0, 1);
    lcd.print("Dist: ");
    lcd.print(distanceCM, 1); // Muestra 1 decimal
    lcd.print("cm   "); // Espacios para borrar números anteriores
  } else {
    // Si la distancia es MENOR o IGUAL a la distancia objetivo, significa que el nivel de agua es suficiente
    // Si la bomba está encendida, apágala
    if (pumpIsOn) {
      digitalWrite(pumpPin, LOW); // Apaga la bomba
      pumpIsOn = false;
      lcd.clear();
      lcd.print("Contenedor Lleno!");
    }
    // Muestra la distancia final cuando el recipiente está lleno
    lcd.setCursor(0, 1);
    lcd.print("Dist: ");
    lcd.print(distanceCM, 1);
    lcd.print("cm   ");
    // Una vez lleno, el sistema se mantiene en este estado.
  }

  // --- Paso 3: Controlar el LED RGB variable ---
  if (pumpIsOn) {
    // Si la bomba está encendida, el LED hará un efecto de fade cíclico (arcoíris)
    switch (fadeState) {
      case 0: // De Rojo a Verde
        rValue -= fadeStep;
        gValue += fadeStep;
        if (rValue <= 0) {
          rValue = 0;
          gValue = 255;
          fadeState = 1; // Cambia al siguiente estado: Verde a Azul
        }
        break;
      case 1: // De Verde a Azul
        gValue -= fadeStep;
        bValue += fadeStep;
        if (gValue <= 0) {
          gValue = 0;
          bValue = 255;
          fadeState = 2; // Cambia al siguiente estado: Azul a Rojo
        }
        break;
      case 2: // De Azul a Rojo
        bValue -= fadeStep;
        rValue += fadeStep;
        if (bValue <= 0) {
          bValue = 0;
          rValue = 255;
          fadeState = 0; // Vuelve al primer estado: Rojo a Verde
        }
        break;
    }
  } else {
    // Si la bomba está apagada (contenedor lleno), el LED se queda en un color fijo (ej. Azul brillante)
    rValue = 0;
    gValue = 0;
    bValue = 255;
  }

  // Aplica los valores de color al LED RGB
  analogWrite(R_PIN, rValue);
  analogWrite(G_PIN, gValue);
  analogWrite(B_PIN, bValue);

  // Pequeña pausa para estabilizar las lecturas y la pantalla
  delay(100);
}