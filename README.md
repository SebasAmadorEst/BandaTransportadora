# Clasificador Automático de Componentes Electrónicos (IA + Hardware)

Este proyecto consiste en un sistema de clasificación automatizado que utiliza Inteligencia Artificial (Edge Impulse) y visión artificial para identificar componentes electrónicos en una banda transportadora y clasificarlos físicamente mediante un servomotor.

## 🚀 Arquitectura del Proyecto
El sistema utiliza una arquitectura de **Maestro-Esclavo** para optimizar el rendimiento:
1. **ESP32-CAM (Cerebro):** Encargada del reconocimiento visual, la interfaz en pantalla OLED y el envío de comandos seriales22, 25].
2. **ESP32 Dev Module (Músculo):** Dedicada exclusivamente al control del servomotor para evitar interferencias con la cámara47, 49].

## 📁 Estructura del Repositorio
* `/src`: Códigos fuente (.ino).
* `/multimedia`: Imagenes del montaje finalizado.
* `/conexionado`: Esquema digital de las conexiones del proyecto.

## 🛠️ Hardware Utilizado
* **ESP32-CAM** (Modelo AI-Thinker).
* **ESP32 Dev Module**.
* **Pantalla OLED SH1106** (1.3") vía I2C.
* **Servomotor** conectado al Pin 13 de la ESP32.
* **Fuente de alimentación externa** para el servomotor (GND compartido).
* **Banda transportadora**.

## 💻 Descripción de los Códigos

### 1. Recolección de Imágenes (`recolectarImagenesEdgeImpulse.ino`)
Servidor web para capturar el dataset necesario para entrenar el modelo en Edge Impulse. Incluye control de brillo del LED Flash por monitor serial (0-9) para mejorar la iluminación.

### 2. Reconocimiento y Clasificación (`esp32cam-oled.ino`)
* Ejecuta el modelo de Edge Impulse con un filtro de certeza del 90%.
* Muestra el nombre del componente y el porcentaje de confianza en la pantalla OLED.
* Envía comandos binarios (`'0'` o `'1'`) vía UART (TXD) a la placa de control ESP32].

### 3. Control de Actuador (`esp32-servo.ino`)
* Recibe las órdenes por Serial (RX0).
* **Lógica de Posicionamiento:**
  * Recibe `'0'`: Mueve el servo a **0°**, espera 9 segundos y regresa a 90° a los 9 segundos.
  * Recibe `'1'`: Mueve el servo a **180°**, espera 9 segundos y regresa a 90° a los 9 segundos.
* Implementa limpieza de buffer para ignorar señales redundantes mientras el motor está en movimiento.

## 🎥 Videos

* **Funcionamiento:** [Prueba de clasificación en tiempo real](https://www.youtube.com/shorts/k4VcncxdQCM)
* **Montaje:** [Vista del producto final](https://www.youtube.com/shorts/x_CwsfI-kAk)

---
**Autores:** Sebastián Amador - Juan David Arias - Daniel Bedoya - Camilo Giraldo

**Universidad:** Universidad Tecnológica de Pereira (UTP)
