#define WIFI_SSID "Sebastian"
#define WIFI_PASS "Kilokilo"
#define HOSTNAME "esp32cam"

#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/extra/esp32/wifi/sta.h>
#include <eloquent_esp32cam/viz/image_collection.h>

using eloq::camera;
using eloq::wifi;
using eloq::viz::collectionServer;

// Configuración del LED
const int ledPin = 4;
const int freq = 5000;
const int ledResolution = 8; 

void setup() {
    delay(3000);
    Serial.begin(115200);

    // Configurar PWM para el LED
    ledcAttach(ledPin, freq, ledResolution);

    camera.pinout.aithinker();
    camera.brownout.disable();
    camera.resolution.face();
    camera.quality.high();

    while (!camera.begin().isOk()) Serial.println(camera.exception.toString());
    while (!wifi.connect().isOk()) Serial.println(wifi.exception.toString());
    while (!collectionServer.begin().isOk()) Serial.println(collectionServer.exception.toString());

    Serial.println("--- SISTEMA LISTO ---");
    Serial.println("Servidor de Imagenes: " + collectionServer.address());
    Serial.println("Escribe un numero del 0 al 9 en el monitor serial para ajustar el brillo:");
}

void loop() {
    // Escuchar el monitor serial
    if (Serial.available() > 0) {
        // Leemos el caracter (ej. '5') y lo convertimos a número (5)
        char c = Serial.read();
        
        if (isDigit(c)) {
            int nivel = c - '0'; // Convierte el caracter ASCII a entero 0-9
            
            // Mapeamos 0-9 a 0-255 (potencia PWM)
            // 0 -> 0, 1 -> 28, ..., 9 -> 255
            int brillo = map(nivel, 0, 9, 0, 255);
            
            ledcWrite(ledPin, brillo);
            
            Serial.print("Potencia establecida al: ");
            Serial.print(nivel * 10);
            Serial.println("%");
        }
    }
    delay(10);
}