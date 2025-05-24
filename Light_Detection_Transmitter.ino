// Improved FSOC Transmitter
// This version includes:
// - Start/stop sequences for synchronization
// - Manchester encoding for better clock recovery
// - Preamble for signal detection
// - Frame markers
// - Improved error detection

const int laserPin = D2;  // Pin connected to the NPN transistor base
const unsigned long bitDuration = 50;  // 50ms per bit (faster but still reliable)

// Text data to transmit
String textData = "Hello, this is a test message. It can be a few sentences long.";

void setup() {
  pinMode(laserPin, OUTPUT);
  digitalWrite(laserPin, LOW);  // Start with laser off
  Serial.begin(9600);  // For debugging
  Serial.println("Improved FSOC Transmitter");
}

void loop() {
  Serial.println("Starting transmission...");
  
  // Step 1: Calculate metadata
  String fileType = "TXT";  // 3 bytes
  unsigned long dataLength = textData.length();  // 4 bytes
  uint16_t crc = calculateCRC16(textData);  // 2 bytes (better than simple checksum)
  
  // Step 2: Prepare transmission data with start sequence and headers
  String dataToSend = fileType + 
                     String((char)(dataLength >> 24)) +
                     String((char)(dataLength >> 16)) +
                     String((char)(dataLength >> 8)) +
                     String((char)(dataLength)) +
                     String((char)(crc >> 8)) +
                     String((char)(crc & 0xFF)) +
                     textData;
  
  // Step 3: Send preamble (10101010) for synchronization
  transmitPreamble();
  
  // Step 4: Send start frame delimiter (unique pattern)
  transmitStartFrame();
  
  // Step 5: Send data with Manchester encoding
  transmitManchesterData(dataToSend);
  
  // Step 6: Send end frame delimiter
  transmitEndFrame();
  
  Serial.println("Transmission complete. Waiting 5 seconds...");
  digitalWrite(laserPin, LOW);  // Ensure laser is off
  delay(5000);  // Wait before next transmission
}

// Transmit a series of alternating 1's and 0's to help receiver synchronize
void transmitPreamble() {
  for (int i = 0; i < 16; i++) {  // 16 alternating bits
    digitalWrite(laserPin, i % 2 == 0 ? HIGH : LOW);
    delay(bitDuration);
  }
}

// Transmit a unique start frame pattern (11110000)
void transmitStartFrame() {
  for (int i = 0; i < 4; i++) digitalWrite(laserPin, HIGH), delay(bitDuration);
  for (int i = 0; i < 4; i++) digitalWrite(laserPin, LOW), delay(bitDuration);
}

// Transmit an end frame pattern (00001111)
void transmitEndFrame() {
  for (int i = 0; i < 4; i++) digitalWrite(laserPin, LOW), delay(bitDuration);
  for (int i = 0; i < 4; i++) digitalWrite(laserPin, HIGH), delay(bitDuration);
}

// Transmit data using Manchester encoding
void transmitManchesterData(String data) {
  for (char c : data) {
    // Transmit each bit of the character using Manchester encoding
    for (int i = 7; i >= 0; i--) {
      bool bit = (c & (1 << i)) > 0;
      
      // Manchester encoding: 1 is HIGH-LOW, 0 is LOW-HIGH
      if (bit) {
        digitalWrite(laserPin, HIGH);
        delay(bitDuration / 2);
        digitalWrite(laserPin, LOW);
        delay(bitDuration / 2);
      } else {
        digitalWrite(laserPin, LOW);
        delay(bitDuration / 2);
        digitalWrite(laserPin, HIGH);
        delay(bitDuration / 2);
      }
    }
  }
}

// Calculate CRC-16 for better error detection
uint16_t calculateCRC16(String s) {
  uint16_t crc = 0xFFFF;
  
  for (unsigned int i = 0; i < s.length(); i++) {
    uint8_t ch = s.charAt(i);
    for (uint8_t j = 0; j < 8; j++) {
      bool b = (ch & 1) ^ (crc & 1);
      crc >>= 1;
      if (b) {
        crc ^= 0xA001;
      }
      ch >>= 1;
    }
  }
  
  return crc;
}