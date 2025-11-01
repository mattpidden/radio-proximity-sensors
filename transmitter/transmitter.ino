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

// --- State Variables ---
// Flag to indicate transmission finished (set by interrupt)
volatile bool operationDone = false;
// Counter for unique packets
int packetCounter = 0;
// Transmission result state
int transmissionState = RADIOLIB_ERR_NONE;

// Non-blocking timer variables
unsigned long lastTxTime = 0;
const long txInterval = 5000; // Transmit every 5000 milliseconds (5 seconds)

// --- Interrupt Handler ---
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // We sent a packet, set the flag
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
  Serial.print(F("[TX] Initializing SX1262 ... "));
  // radio.begin(Freq, BW, SF, CR, SyncWord, Power, PreambleLen, AmplifierGain)
  int state = radio.begin(868.0, 125.0, 12, 5, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 17, 14, 0);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // Set the function that will be called when packet is transmitted
  radio.setDio1Action(setFlag);
}

void loop() {
  // 1. Check if the previous transmission finished
  if(operationDone) {
    // reset flag
    operationDone = false;

    // Report on the status of the last transmission
    if (transmissionState == RADIOLIB_ERR_NONE) {
      Serial.println(F("... Transmission finished successfully."));
    } else {
      Serial.print(F("... Transmission failed, code "));
      Serial.println(transmissionState);
    }
  }

  // 2. Check if it's time to send a new packet
  if(millis() - lastTxTime >= txInterval) {
    // Generate the payload
    packetCounter++;
    String payload = "Hello World! #" + String(packetCounter);

    Serial.print(F("[TX] Sending packet #"));
    Serial.print(packetCounter);
    Serial.print(F(" ("));
    Serial.print(payload.length());
    Serial.println(F(" bytes) ..."));

    // Start the asynchronous transmission
    transmissionState = radio.startTransmit(payload);

    // Update timer
    lastTxTime = millis();
  }
}