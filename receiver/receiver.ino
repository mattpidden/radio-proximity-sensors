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
}

void loop() {
  // Check if the previous reception finished
  if(operationDone) {
    // Reset flag immediately
    operationDone = false;

    // Read the data
    String str;
    int state = radio.readData(str);

    if (state == RADIOLIB_ERR_NONE) {
      // Packet was successfully received
      Serial.println(F("\n--- Packet Received ---"));

      // print data of the packet
      Serial.print(F("Data:\t\t"));
      Serial.println(str);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

      float constrainedRssi = constrain(radio.getRSSI(), -140.0, -60.0);
      long beepPeriodMs = (long)((-1000.0 * (constrainedRssi + 50.0)) / 8.0);
      // print beep period in Ms
      Serial.print(F("BP:\t\t"));
      Serial.print(beepPeriodMs/1000);
      Serial.println(F(" s"));

     
      Serial.println(F("-----------------------"));
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
}
