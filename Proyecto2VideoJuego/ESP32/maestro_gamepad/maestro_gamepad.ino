#include <Bluepad32.h>

// ===================== CONFIGURACION =====================

#define NUCLEO_RX_PIN   16        // RX2 del ESP32  <- TX (PC10) de la Nucleo
#define NUCLEO_TX_PIN   17        // TX2 del ESP32  -> RX (PC11) de la Nucleo
#define NUCLEO_BAUD     115200
#define SEND_PERIOD_MS  20        // 50 tramas por segundo hacia la Nucleo
#define DEADZONE        10        // zona muerta del joystick (%)

// true = olvida los controles al encender (tambien con BOOT + EN)
#define OLVIDAR_CONTROLES false
#define PIN_BOOT          0       // boton BOOT de la placa

// Bits de los botones (deben coincidir con la Nucleo)
#define BTN_FIRE   0x01
#define BTN_POWER  0x02
#define BTN_MENU   0x04

ControllerPtr gamepad = nullptr;  // solo se usa un control (juego de 1 jugador)

uint8_t  gameState = 0, gameFlags = 0;   // ultimo estado recibido de la Nucleo
uint32_t lastNucleoMs = 0;

// ===================== CONEXION DEL CONTROL =====================

void onConnectedController(ControllerPtr ctl) {
  // Un control nuevo reemplaza al anterior (juego de 1 jugador)
  if (gamepad != nullptr && gamepad != ctl) {
    Serial.println("Llego un control nuevo: reemplaza al anterior");
    gamepad->disconnect();
  }
  gamepad = ctl;
  ControllerProperties p = ctl->getProperties();
  Serial.printf("CONTROL CONECTADO: %s (VID=0x%04x PID=0x%04x)\n",
                ctl->getModelName().c_str(), p.vendor_id, p.product_id);
  ctl->setPlayerLEDs(1);                       // luz de "jugador 1" (Switch, PS5, Xbox)
  ctl->setColorLED(0, 0, 255);                 // barra de luz azul (PS4/PS5)
  ctl->playDualRumble(0, 200, 0x80, 0x80);     // vibracion corta de bienvenida
}

void onDisconnectedController(ControllerPtr ctl) {
  if (ctl == gamepad) {
    gamepad = nullptr;
    Serial.println("CONTROL DESCONECTADO");
  }
}

// ===================== CONVERSION A LAS TRAMAS DE LA NUCLEO =====================

// Eje de Bluepad32 (-512..511) a -100..100 con zona muerta
int8_t axisToPercent(int32_t v) {
  float p = v * 100.0f / 512.0f;
  if (p >  100.0f) p =  100.0f;
  if (p < -100.0f) p = -100.0f;
  float a = (p < 0) ? -p : p;
  if (a < DEADZONE) return 0;
  a = (a - DEADZONE) * 100.0f / (100.0f - DEADZONE);
  return (int8_t)((p < 0) ? -a : a);
}

void sendPadToNucleo(int8_t jx, int8_t jy, uint8_t buttons) {
  static uint8_t seq = 0;
  uint8_t f[7];
  f[0] = 0xAA;
  f[1] = 0x55;
  f[2] = (uint8_t)jx;
  f[3] = (uint8_t)jy;
  f[4] = buttons;
  f[5] = seq++;
  f[6] = f[2] ^ f[3] ^ f[4] ^ f[5];
  Serial2.write(f, sizeof(f));
}

// Lee el control y arma jx, jy y botones
void readGamepad(int8_t &jx, int8_t &jy, uint8_t &buttons) {
  jx = axisToPercent(gamepad->axisX());
  jy = (int8_t)-axisToPercent(gamepad->axisY());   // en el control, abajo es positivo

  // La cruceta tambien sirve (comoda para el menu) si el joystick esta suelto
  uint8_t d = gamepad->dpad();
  if (jx == 0) { if (d & DPAD_RIGHT) jx = 100; else if (d & DPAD_LEFT) jx = -100; }
  if (jy == 0) { if (d & DPAD_UP)    jy = 100; else if (d & DPAD_DOWN) jy = -100; }

  buttons = 0;
  if (gamepad->a())                                   buttons |= BTN_FIRE;
  if (gamepad->b() || gamepad->r1())                  buttons |= BTN_POWER;
  if (gamepad->miscStart() || gamepad->thumbL())      buttons |= BTN_MENU;
}

// ===================== ESTADO QUE MANDA LA NUCLEO =====================

void parseNucleoByte(uint8_t b) {
  static uint8_t st = 0;
  static uint8_t d[3];
  switch (st) {
    case 0: if (b == 0xAA) st = 1; break;
    case 1:
      if (b == 0x56)      st = 2;
      else if (b != 0xAA) st = 0;
      break;
    default:
      d[st - 2] = b;
      if (++st == 5) {
        st = 0;
        if ((uint8_t)(d[0] ^ d[1]) == d[2]) {
          gameState = d[0];
          gameFlags = d[1];
          lastNucleoMs = millis();
        }
      }
      break;
  }
}

// Retroalimentacion en el control: color de la barra y vibracion
void updateFeedback() {
  static uint8_t lastState = 0xFF, lastFlags = 0xFF;
  if (gamepad == nullptr) { lastState = lastFlags = 0xFF; return; }
  if (gameState == lastState && gameFlags == lastFlags) return;

  if (gameState != lastState) {
    if (gameState == 4) gamepad->playDualRumble(0, 400, 0xC0, 0x40);   // victoria
    if (gameState == 5) gamepad->playDualRumble(0, 700, 0x40, 0xFF);   // derrota
  }
  if (gameFlags & 1)       gamepad->setColorLED(255, 160, 0);   // poder disponible: naranja
  else if (gameState == 5) gamepad->setColorLED(255, 0, 0);     // derrota: rojo
  else if (gameState == 4) gamepad->setColorLED(0, 255, 0);     // victoria: verde
  else                     gamepad->setColorLED(0, 0, 255);     // normal: azul

  lastState = gameState;
  lastFlags = gameFlags;
}

// ===================== PROGRAMA PRINCIPAL =====================

uint32_t lastSend = 0, lastPrint = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(NUCLEO_BAUD, SERIAL_8N1, NUCLEO_RX_PIN, NUCLEO_TX_PIN);

  Serial.printf("Bluepad32 version: %s\n", BP32.firmwareVersion());
  const uint8_t *addr = BP32.localBdAddress();
  Serial.printf("MAC Bluetooth del maestro: %02X:%02X:%02X:%02X:%02X:%02X\n",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  BP32.setup(&onConnectedController, &onDisconnectedController);

  pinMode(PIN_BOOT, INPUT_PULLUP);
  if (OLVIDAR_CONTROLES || digitalRead(PIN_BOOT) == LOW) {
    BP32.forgetBluetoothKeys();         // borra los controles guardados
    Serial.println("Controles anteriores OLVIDADOS: empareja el control de nuevo");
  }
  BP32.enableNewBluetoothConnections(true);   // acepta controles nuevos
  BP32.enableVirtualDevice(false);      // el touchpad del PS4/PS5 no se usa como mouse

  Serial.println("Esperando control... pon el control en modo emparejar");
}

void loop() {
  BP32.update();                        // procesa los datos que llegaron del control
  uint32_t now = millis();

  while (Serial2.available()) parseNucleoByte((uint8_t)Serial2.read());

  bool conectado = gamepad != nullptr && gamepad->isConnected();
  int8_t jx = 0, jy = 0;
  uint8_t buttons = 0;
  if (conectado) readGamepad(jx, jy, buttons);

  // Solo se envian tramas si hay un control conectado
  if (conectado && now - lastSend >= SEND_PERIOD_MS) {
    lastSend = now;
    sendPadToNucleo(jx, jy, buttons);
  }

  updateFeedback();

  if (now - lastPrint >= 300) {
    lastPrint = now;
    Serial.printf("control=%s | X=%4d Y=%4d btn=%d%d%d | Nucleo: estado=%u flags=%u %s\n",
                  conectado ? "SI" : "NO", jx, jy,
                  (buttons & BTN_FIRE) ? 1 : 0, (buttons & BTN_POWER) ? 1 : 0, (buttons & BTN_MENU) ? 1 : 0,
                  gameState, gameFlags,
                  (lastNucleoMs && now - lastNucleoMs < 500) ? "(OK)" : "(sin Nucleo)");
  }

  delay(1);                             // deja correr las tareas internas
}
