#include <LiquidCrystal.h>

// Configuración del LCD (RS, Enable, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Pines del sensor ultrasónico
const int trigPin = A0;
const int echoPin = A1;

// Pin del módulo relé (control de la bomba)
const int pumpPin = 9;
const int buzzer = 8; // Zumbador opcional

// Distancia objetivo calibrada (ajustar según tu recipiente)
const float TARGET_WATER_DIST_CM = 16.0; // Cambiar este valor según tu medición

long duration;
float distanceCM;
bool pumpIsOn = false;

unsigned long pumpStartTime = 0;     // Marca de tiempo de inicio de la bomba
const long maxPumpTime = 10000;     // Máximo tiempo de funcionamiento de la bomba en ms (ej: 10 segundos)

void setup() {
    Serial.begin(9600);

    // Inicializar LCD
    lcd.begin(16, 2);
    lcd.print("Iniciando...");
    delay(1000);
    lcd.clear();

    // Configurar pines
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(pumpPin, OUTPUT);
    pinMode(buzzer, OUTPUT);

    digitalWrite(pumpPin, HIGH);   // Relé: HIGH = apagado (por defecto)
    digitalWrite(buzzer, LOW);     // Zumbador apagado

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

    // Mostrar distancia en consola (opcional)
    Serial.print("Distancia: ");
    Serial.print(distanceCM);
    Serial.println(" cm");

    // --- CONTROL DE LA BOMBA CON RELÉ ---
    if (distanceCM > TARGET_WATER_DIST_CM) {
        // Nivel bajo: Encender la bomba si no está activa
        if (!pumpIsOn) {
            digitalWrite(pumpPin, LOW); // Relé: LOW = ON
            pumpIsOn = true;
            pumpStartTime = millis();   // Registrar hora de inicio
            lcd.clear();
            lcd.print("Llenando...");
        }

        // Actualiza solo la línea de distancia sin borrar todo
        lcd.setCursor(0, 1);
        lcd.print("Dist: ");
        lcd.print(distanceCM, 1);
        lcd.println(" cm  ");

    } else {
        // Nivel suficiente: Apagar la bomba si está activa
        if (pumpIsOn) {
            digitalWrite(pumpPin, HIGH); // Relé: HIGH = OFF
            pumpIsOn = false;

            digitalWrite(buzzer, HIGH); // Sonido breve de confirmación
            delay(300);
            digitalWrite(buzzer, LOW);

            lcd.clear();
            lcd.print("Nivel OK!");
        }

        // Mostrar distancia final
        lcd.setCursor(0, 1);
        lcd.print("Dist: ");
        lcd.print(distanceCM, 1);
        lcd.println(" cm  ");
    }

    // --- PROTECCIÓN POR TIEMPO MÁXIMO DE BOMBA ---
    if (pumpIsOn && (millis() - pumpStartTime >= maxPumpTime)) {
        digitalWrite(pumpPin, HIGH); // Forzar apagado por seguridad
        pumpIsOn = false;
        lcd.clear();
        lcd.print("Bomba forzada");
        lcd.setCursor(0, 1);
        lcd.print("por tiempo max.");
        delay(2000);
        lcd.clear();
        lcd.print("Esperando...");
        delay(1000);
    }

    delay(200); // Frecuencia de actualización estable
}
