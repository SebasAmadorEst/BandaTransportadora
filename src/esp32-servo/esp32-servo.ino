#include <ESP32Servo.h>

Servo miServo;
const int servoPin = 13; 

void setup() {
  Serial.begin(115200);
  
  ESP32PWM::allocateTimer(0);
  miServo.setPeriodHertz(50);
  miServo.attach(servoPin, 500, 2400);
  
  miServo.write(90); // Posición neutral de la banda
  
  Serial.println("--- SISTEMA LISTO ---");
  Serial.println("Esperando señal de clasificación (Espera de 5s activa)");
}

void loop() {
  if (Serial.available() > 0) {
    char dato = Serial.read();

    if (dato == '0') {
      Serial.println(">> Clasificando: Componente 0 detectado.");
      miServo.write(0);      // Mover a la primera posición
      
      delay(9000);           // SE QUEDA 5 SEGUNDOS EN ESA POSICIÓN
      
      miServo.write(90);     // Regresa a la posición neutral
      Serial.println(">> Regreso a 90°. Listo para recibir nueva señal.");
      
      // LIMPIEZA: Ignora cualquier dato que haya llegado mientras el servo estaba moviéndose
      while(Serial.available() > 0) Serial.read(); 
    } 
    
    else if (dato == '1') {
      Serial.println(">> Clasificando: Componente 1 detectado.");
      miServo.write(180);    // Mover a la segunda posición
      
      delay(9000);           // SE QUEDA 5 SEGUNDOS EN ESA POSICIÓN
      
      miServo.write(90);     // Regresa a la posición neutral
      Serial.println(">> Regreso a 90°. Listo para recibir nueva señal.");
      
      // LIMPIEZA: Ignora cualquier dato que haya llegado mientras el servo estaba moviéndose
      while(Serial.available() > 0) Serial.read();
    }
  }
}