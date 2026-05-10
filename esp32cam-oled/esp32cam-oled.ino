#include <Componentes_Detect_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"
#include <Wire.h>
#include <U8g2lib.h> 

// --- CONFIGURACIÓN LED FLASH ---
const int ledPin = 4; 

// Selección de modelo de cámara
#define CAMERA_MODEL_AI_THINKER 

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#else
#error "Camera model not selected"
#endif

/* Constant defines -------------------------------------------------------- */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE 3

// Configuración de Pines I2C para tu ESP32-CAM
#define I2C_SDA 15
#define I2C_SCL 14

// Constructor específico para SH1106 de 1.3" (U8g2)
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; 
static bool is_initialised = false;
uint8_t *snapshot_buf; 

static camera_config_t camera_config = {
  .pin_pwdn = PWDN_GPIO_NUM,
  .pin_reset = RESET_GPIO_NUM,
  .pin_xclk = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7 = Y9_GPIO_NUM,
  .pin_d6 = Y8_GPIO_NUM,
  .pin_d5 = Y7_GPIO_NUM,
  .pin_d4 = Y6_GPIO_NUM,
  .pin_d3 = Y5_GPIO_NUM,
  .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM,
  .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync = VSYNC_GPIO_NUM,
  .pin_href = HREF_GPIO_NUM,
  .pin_pclk = PCLK_GPIO_NUM,
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_JPEG, 
  .frame_size = FRAMESIZE_QVGA, 
  .jpeg_quality = 12, 
  .fb_count = 1, 
  .fb_location = CAMERA_FB_IN_PSRAM,
  .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

/* Prototipos de funciones */
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

void setup() {
  Serial.begin(115200);

  // Inicializar Flash apagado
  analogWrite(ledPin, 0);

  // Inicializar I2C y Pantalla con U8g2
  Wire.begin(I2C_SDA, I2C_SCL);
  display.begin();
  
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr); 
  display.drawStr(0, 15, "Edge Impulse Demo");
  display.sendBuffer();

  while (!Serial);
  Serial.println("Edge Impulse Inferencing Demo");
  Serial.println("--- Control de LED ---");
  Serial.println("Escribe '1' para 10% de brillo");
  Serial.println("Escribe '0' para apagar");

  if (ei_camera_init() == false) {
    ei_printf("Failed to initialize Camera!\r\n");
  } else {
    ei_printf("Camera initialized\r\n");
  }

  display.clearBuffer();
  display.drawStr(0, 15, "Starting inference");
  display.drawStr(0, 30, "in 2 seconds...");
  display.sendBuffer();
  
  ei_sleep(2000);
  display.clearBuffer();
}

void loop() {
  // --- CONTROL DEL LED POR SERIAL ---
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '1') {
      analogWrite(ledPin, 25); // ~10% de 255
      Serial.println("Flash: 10%");
    } else if (c == '0') {
      analogWrite(ledPin, 0);
      Serial.println("Flash: OFF");
    }
  }

  // Limpiar buffer de pantalla al inicio de cada ciclo
  display.clearBuffer();

  if (ei_sleep(5) != EI_IMPULSE_OK) {
    return;
  }

  snapshot_buf = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

  if (snapshot_buf == nullptr) {
    ei_printf("ERR: Failed to allocate snapshot buffer!\n");
    return;
  }

  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data = &ei_camera_get_data;

  if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
    ei_printf("Failed to capture image\r\n");
    free(snapshot_buf);
    return;
  }

  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK) {
    ei_printf("ERR: Failed to run classifier (%d)\n", err);
    free(snapshot_buf);
    return;
  }

  //ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
    //        result.timing.dsp, result.timing.classification, result.timing.anomaly);


#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    bool bb_found = false;
    int display_line = 0; 

    for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
      auto bb = result.bounding_boxes[ix];
      if (bb.value == 0) continue;
      
      // --- FILTRO DE CERTEZA AL 90% ---
      if (bb.value >= 0.90) { 
          bb_found = true;
          
          for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
              if (strcmp(bb.label, ei_classifier_inferencing_categories[i]) == 0) {
                  // ENVIAMOS SOLO EL CARÁCTER PURO
                  if (i == 0) {
                      Serial.write('0'); 
                  } else if (i == 1) {
                      Serial.write('1'); 
                  }
                  break; 
              }
          }

          // Esto es para la PANTALLA OLED (esto no viaja por el cable TXD, así que está bien)
          char str[32];
          sprintf(str, "%s: %d%%", bb.label, int(bb.value * 100));
          display.drawStr(0, 15 + (display_line * 15), str);
          display_line++;
        }
    }

      if (!bb_found) {
        ei_printf("    Buscando o certeza insuficiente...\n");
        display.drawStr(0, 30, "Escaneando..."); 
      }
    #else
      // Si usas clasificación simple, también lo dejamos listo por índice:
      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result.classification[ix].value >= 0.90) {
            if (ix == 0) Serial.write('0');
            else if (ix == 1) Serial.write('1');
            
            char str[32];
            sprintf(str, "%s: %.2f", result.classification[ix].label, result.classification[ix].value);
            display.drawStr(0, 15 + (ix * 12), str);
        }
      }
    #endif

  display.sendBuffer();

  #if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
  #endif

  free(snapshot_buf);
}

bool ei_camera_init(void) {
  if (is_initialised) return true;
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, 0);
  }
  is_initialised = true;
  return true;
}

void ei_camera_deinit(void) {
  esp_err_t err = esp_camera_deinit();
  if (err != ESP_OK) {
    ei_printf("Camera deinit failed\n");
    return;
  }
  is_initialised = false;
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
  if (!is_initialised) {
    ei_printf("ERR: Camera is not initialized\r\n");
    return false;
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    ei_printf("Camera capture failed\n");
    return false;
  }
  bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
  esp_camera_fb_return(fb);
  if (!converted) {
    ei_printf("Conversion failed\n");
    return false;
  }
  if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
    ei::image::processing::crop_and_interpolate_rgb888(out_buf, EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS, out_buf, img_width, img_height);
  }
  return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;
  while (pixels_left != 0) {
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];
    out_ptr_ix++;
    pixel_ix += 3;
    pixels_left--;
  }
  return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif