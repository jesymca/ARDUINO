// PRUEBA FINAL: DIAGNÓSTICO DEL SENSOR HC-SR04

const int trigPin = 6;
const int echoPin = 7;

void setup() {
  // Iniciar comunicación serial para ver los resultados en el PC
  Serial.begin(9600);
  
  // Configurar los pines del sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  Serial.println("--- INICIANDO DIAGNÓSTICO DEL SENSOR ---");
  Serial.println("Mueve tu mano frente al sensor y observa los números.");
}

void loop() {
  // Dispara el pulso ultrasónico
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Mide el tiempo del eco
  long duration = pulseIn(echoPin, HIGH);

  // Calcula la distancia
  long distance = duration * 0.0343 / 2;

  // Muestra la distancia en el Monitor Serial
  Serial.print("Distancia medida: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Espera medio segundo antes de la siguiente medición
  delay(500); 
}