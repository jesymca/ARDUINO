#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define la dirección I2C del LCD (cambia si es necesario)
LiquidCrystal_I2C lcd(0x27, 16, 2); // 16 columnas, 2 filas

// Pines del sensor ultrasónico
const int trigPin = A0;
const int echoPin = A1;

// Bomba y zumbador
const int pumpPin = 9;
const int buzzer = 8;

// Distancia calibrada para nivel óptimo (ajustar según tu recipiente)
const float TARGET_WATER_DIST_CM = 16.0; // Cambiar según calibración

// --- NUEVAS CONSTANTES PARA HISTÉRESIS ---
// La bomba se enciende si la distancia es MAYOR que este valor (nivel bajo).
const float HYSTERESIS_THRESHOLD_ON = TARGET_WATER_DIST_CM + 2.0; // Por ejemplo, 18.0 cm (el agua está 2 cm por debajo del objetivo)

// La bomba se apaga si la distancia es MENOR que este valor (nivel lleno).
const float HYSTERESIS_THRESHOLD_OFF = TARGET_WATER_DIST_CM - 0.5; // Por ejemplo, 15.5 cm (el agua está 0.5 cm por encima del objetivo)
// Puedes ajustar estos valores. HYSTERESIS_THRESHOLD_ON debe ser mayor que HYSTERESIS_THRESHOLD_OFF.

long duration;
float distanceCM;
bool pumpIsOn = false;

void setup() {
  // Inicializa el LCD
  lcd.init();
  // Enciende la luz de fondo
  lcd.backlight();

  lcd.print("Iniciando...");
  delay(1000);
  lcd.clear();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(pumpPin, LOW);
  digitalWrite(buzzer, LOW);

  // Inicializa la comunicación serial
  Serial.begin(9600); // Puedes usar otra velocidad si lo necesitas

  lcd.print("Sistema listo");
  delay(1000);
  lcd.clear();
}

void loop() {
  // --- LECTURA DEL SENSOR ULTRASÓNICO ---
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distanceCM = duration * 0.0343 / 2;

  // Mostrar distancia en consola
  Serial.print("Distancia: ");
  Serial.print(distanceCM);
  Serial.println(" cm");

  // --- CONTROL DE LA BOMBA CON HISTÉRESIS ---
  // Condición para encender la bomba:
  // Si la bomba NO está encendida Y la distancia es MAYOR que el umbral de encendido (nivel bajo)
  if (!pumpIsOn && distanceCM > HYSTERESIS_THRESHOLD_ON) {
    digitalWrite(pumpPin, HIGH);
    pumpIsOn = true;
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Llenando...");

  }
  // Condición para apagar la bomba:
  // Si la bomba SÍ está encendida Y la distancia es MENOR que el umbral de apagado (nivel lleno)
  else if (pumpIsOn && distanceCM < HYSTERESIS_THRESHOLD_OFF) {
    digitalWrite(pumpPin, LOW);
    pumpIsOn = false;
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);

    lcd.clear();
    lcd.print("Nivel Lleno!"); // O "Nivel OK"
    delay(1000); // Pausa para que se vea el mensaje
    lcd.clear();
  }

  // --- ACTUALIZACIÓN DE PANTALLA LCD ---
  // Muestra el estado actual y la distancia
  lcd.setCursor(0, 0);
  if (pumpIsOn) {
    lcd.print("Estado: Llenando  ");
  } else {
    // Si la bomba está apagada, se asume que el nivel está bien o monitoreando.
    // Podrías poner "Nivel OK" si distanceCM <= HYSTERESIS_THRESHOLD_OFF
    // o "Esperando..." si distanceCM está entre los dos umbrales.
    if (distanceCM <= HYSTERESIS_THRESHOLD_OFF) {
      lcd.print("Estado: Nivel OK  ");
    } else {
      lcd.print("Estado: Monitoreo ");
    }
  }

  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distanceCM, 1);
  lcd.print(" cm  "); // Añadir espacios para borrar restos de números anteriores

  delay(200); // Frecuencia de actualización estable
}