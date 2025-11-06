#include <RadioLib.h>
#include <SPI.h>

// --- Hardware Pin Definitions (From original code) ---
#define LORA_SCK     14
#define LORA_MISO    24
#define LORA_MOSI    15
#define LORA_SS      13
#define LORA_RST     23
#define LORA_DIO1    16
#define LORA_BUSY    18
#define LORA_ANT_SW  17 // Not used for this module instantiation

// Initialize the SX1262 module using SPI1 instance
SX1262 radio = new Module(LORA_SS, LORA_DIO1, LORA_RST, LORA_BUSY, SPI1);

enum Mode {
  NONE,
  GREEN,
  YELLOW,
  RED
};
Mode currentMode = NONE;
String deviceId = "RACPIDDEN";
unsigned long lastRxTime = 0;
const long rxInterval = 60000;
long beepPeriodMs = 0;
static unsigned long lastBeepTime = 0;
static bool buzzerOn = false;
static unsigned long buzzerChangeTime = 0;

// --- State Variables ---
// Flag to indicate reception finished (set by interrupt)
volatile bool operationDone = false;

// --- Interrupt Handler ---
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // We received a packet, set the flag
  operationDone = true;
}

void setup() {
  Serial.begin(9600);
  delay(2000);

  // --- SPI Initialization (From original code) ---
  SPI1.setRX(LORA_MISO);
  SPI1.setTX(LORA_MOSI);
  SPI1.setSCK(LORA_SCK);
  SPI1.begin();

  // --- Radio Initialization (From original code) ---
  Serial.print(F("[RX] Initializing SX1262 ... "));
  // radio.begin(Freq, BW, SF, CR, SyncWord, Power, PreambleLen, AmplifierGain)
  int state = radio.begin(868.0, 125.0, 12, 5, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 17, 14, 0);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // Set the function that will be called when a new packet is received
  radio.setDio1Action(setFlag);

  // --- Start Listening ---
  Serial.print(F("[RX] Starting to listen for packets ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success! Waiting for data."));
  } else {
    Serial.print(F("failed to start receive, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
  pinMode(9, OUTPUT); // green led
  pinMode(6, OUTPUT); // yellow led
  pinMode(3, OUTPUT); // red led
  pinMode(5, OUTPUT); // buzzer
}

void updateLEDs(){
  switch (currentMode) {
    case NONE:
      digitalWrite(9, LOW);
      digitalWrite(6, LOW);
      digitalWrite(3, LOW);
      break;
    case GREEN:
      digitalWrite(9, HIGH);
      digitalWrite(6, LOW);
      digitalWrite(3, LOW);
      break;

    case YELLOW:
      digitalWrite(9, LOW);
      digitalWrite(6, HIGH);
      digitalWrite(3, LOW);
      break;

    case RED:
      digitalWrite(9, LOW);
      digitalWrite(6, LOW);
      digitalWrite(3, HIGH);
      break;
  }
}



void loop() {
  // Check if the previous reception finished
  updateLEDs();

  if (currentMode != NONE) {
    const unsigned long BEEP_DURATION_MS = 100;
    unsigned long now = millis();
    if (buzzerOn) {
      if (now - buzzerChangeTime >= BEEP_DURATION_MS) {
        noTone(5);
        buzzerOn = false;
        buzzerChangeTime = now; // Mark time buzzer turned OFF
      }
    }
    else { // buzzerOn is false (buzzer is OFF)
      // Check if the silent interval (beepPeriodMs) since the last time the buzzer turned OFF (buzzerChangeTime) has passed.
      if (now - buzzerChangeTime >= beepPeriodMs) {
        tone(5, 1000);
        buzzerOn = true;
        buzzerChangeTime = now; // Mark time buzzer turned ON
        // We only need buzzerChangeTime for the turn-off check, lastBeepTime is not necessary anymore
        // since the logic is driven by the state (buzzerOn) and buzzerChangeTime.
      }
    }
  } else {
    noTone(5);
  }

  if(operationDone) {
    // Reset flag immediately
    operationDone = false;

    // Read the data
    String str;
    int state = radio.readData(str);

    if (state == RADIOLIB_ERR_NONE) {
      if (str.indexOf(deviceId) != -1) {
        lastRxTime = millis();
        int dashIndex = str.indexOf('-');
        if (dashIndex != -1 && dashIndex + 1 < str.length()) {
          char c = str.charAt(dashIndex + 1);
          if (c == 'G') currentMode = GREEN;
          else if (c == 'Y') currentMode = YELLOW;
          else if (c == 'R') currentMode = RED;
        }

        // Packet was successfully received
        Serial.println(F("\n--- Packet Received ---"));

        // print data of the packet
        Serial.print(F("Data:\t\t"));
        Serial.println(str);

        // print data of the packet
        Serial.print(F("Mode:\t\t"));
        if (currentMode == GREEN) Serial.println("GREEN");
        else if (currentMode == YELLOW) Serial.println("YELLOW");
        else if (currentMode == RED) Serial.println("RED");
        else if (currentMode == NONE) Serial.println("NONE");

        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("SNR:\t\t"));
        Serial.print(radio.getSNR());
        Serial.println(F(" dB"));

        float constrainedRssi = constrain(radio.getRSSI(), -140.0, -60.0);
        float norm = (constrainedRssi + 59.9999) / -80.0; // 0 = -60dBm, 1 = -140dBm
        beepPeriodMs = pow(norm, 2.5) * 5000;
        // print beep period in Ms
        Serial.print(F("BP:\t\t"));
        Serial.print(beepPeriodMs/1000);
        Serial.println(F(" s"));

      
        Serial.println(F("-----------------------"));
      }
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // CRC was wrong, but packet was received
      Serial.println(F("[RX] Warning: CRC mismatch! (Data corrupted)"));
    } else {
      // Failed to read data for another reason
      Serial.print(F("[RX] Failed to read data, code "));
      Serial.println(state);
    }

    // Always restart reception after a read operation
    int restartState = radio.startReceive();
    if (restartState == RADIOLIB_ERR_NONE) {
      Serial.println(F("[RX] Listening resumed."));
    } else {
      Serial.print(F("[RX] Failed to restart listening, code "));
      Serial.println(restartState);
    }
  }

  if(millis() - lastRxTime >= rxInterval) {
    currentMode = NONE;
  }
}
