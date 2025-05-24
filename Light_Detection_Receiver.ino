// Improved FSOC Receiver with Extended Timeouts and Real-time Character Display
// This version includes:
// - Preamble detection
// - Auto-calibration for light thresholds
// - Manchester decoding
// - Frame synchronization
// - Error detection
// - Extended timeouts for more reliable reception
// - Real-time character display as each letter is decoded

const int sensorPin = A0;  // Analog pin for photoresistor
const unsigned long bitDuration = 50;  // 100ms per bit (matches transmitter)
const int calibrationSamples = 100;  // Samples for calibration
const int preambleTimeout = 10000;  // 10 seconds timeout for finding preamble (doubled)

// State machine states
enum ReceiverState {
  CALIBRATING,
  SEEKING_PREAMBLE,
  SEEKING_START_FRAME,
  RECEIVING_DATA,
  SEEKING_END_FRAME,
  PROCESSING_DATA
};

// Global variables
ReceiverState currentState = CALIBRATING;
int lightThreshold = 0;
int darkValue = 0;
int lightValue = 1023;
String receivedBinary = "";
String receivedData = "";
unsigned long lastTransitionTime = 0;
unsigned long lastBitTime = 0;
unsigned long lastValidDataTime = 0; // Track last time valid data was received

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(9600);
  Serial.println("Improved FSOC Receiver with Real-time Character Display");
  Serial.println("Starting calibration...");
}

void loop() {
  int sensorValue = analogRead(sensorPin);
  
  switch (currentState) {
    case CALIBRATING:
      performCalibration();
      break;
      
    case SEEKING_PREAMBLE:
      detectPreamble(sensorValue);
      break;
      
    case SEEKING_START_FRAME:
      detectStartFrame(sensorValue);
      break;
      
    case RECEIVING_DATA:
      receiveManchesterData(sensorValue);
      break;
      
    case SEEKING_END_FRAME:
      detectEndFrame(sensorValue);
      break;
      
    case PROCESSING_DATA:
      processReceivedData();
      break;
  }
}

// Calibrate the light and dark thresholds
void performCalibration() {
  Serial.println("Calibrating light levels...");
  int minVal = 1023;
  int maxVal = 0;
  
  // Sample the sensor for calibration period
  for (int i = 0; i < calibrationSamples; i++) {
    int val = analogRead(sensorPin);
    minVal = min(minVal, val);
    maxVal = max(maxVal, val);
    delay(20);
  }
  
  // Set threshold at midpoint with some margin
  darkValue = minVal;
  lightValue = maxVal;
  lightThreshold = (minVal + maxVal) / 2;
  
  Serial.print("Calibration complete. Dark: ");
  Serial.print(darkValue);
  Serial.print(", Light: ");
  Serial.print(lightValue);
  Serial.print(", Threshold: ");
  Serial.println(lightThreshold);
  
  currentState = SEEKING_PREAMBLE;
  Serial.println("Waiting for preamble...");
}

// Wait for a series of transitions that match the preamble pattern
void detectPreamble(int sensorValue) {
  static int transitionCount = 0;
  static bool lastWasHigh = false;
  static unsigned long preambleStartTime = 0;
  
  if (preambleStartTime == 0) {
    preambleStartTime = millis();
  }
  
  // Reset if timeout
  if (millis() - preambleStartTime > preambleTimeout) {
    transitionCount = 0;
    preambleStartTime = millis();
    Serial.println("Preamble timeout, resetting...");
  }
  
  bool isHigh = sensorValue > lightThreshold;
  
  // Detect transitions
  if (isHigh != lastWasHigh) {
    lastTransitionTime = millis();
    transitionCount++;
    lastWasHigh = isHigh;
    
    // Less verbose output - just a dot to show activity
    if (transitionCount % 2 == 0) {
      Serial.print(".");
    }
    
    // After enough transitions (at least 8), we consider it a preamble
    if (transitionCount >= 8) {
      Serial.println("\nPreamble detected!");
      transitionCount = 0;
      currentState = SEEKING_START_FRAME;
    }
  }
  
  // Add delay to avoid multiple readings of the same transition
  delay(bitDuration / 4);
}

// Look for the start frame pattern (11110000)
void detectStartFrame(int sensorValue) {
  static String framePattern = "";
  static unsigned long patternStartTime = millis();
  
  // Reset pattern if it's taking too long
  if (millis() - patternStartTime > 5000) { // Extended from 2000ms to 5000ms
    framePattern = "";
    patternStartTime = millis();
    Serial.println("Start frame timeout, resetting...");
  }
  
  // Sample at appropriate intervals
  if (millis() - lastBitTime >= bitDuration) {
    bool isHigh = sensorValue > lightThreshold;
    framePattern += isHigh ? "1" : "0";
    lastBitTime = millis();
    
    // Minimized output - just show a + for each bit
    Serial.print("+");
    
    // Check if we have the complete start frame pattern
    if (framePattern.length() >= 8) {
      if (framePattern.substring(framePattern.length() - 8) == "11110000") {
        Serial.println("\nStart frame detected! Receiving data...");
        framePattern = "";
        receivedBinary = "";
        receivedData = "";
        lastValidDataTime = millis(); // Initialize the last valid data time
        currentState = RECEIVING_DATA;
      } else if (framePattern.length() > 16) {
        // If we've collected too many bits without finding the pattern,
        // slide the window and remove old bits
        framePattern = framePattern.substring(framePattern.length() - 16);
      }
    }
  }
}

// Receive data using Manchester decoding with real-time character display
void receiveManchesterData(int sensorValue) {
  static String manchesterBits = "";
  static unsigned long dataTimeout = 30000;  // Extended to 30 seconds (from 10s)
  static unsigned long inactivityTimeout = 3000; // 3 seconds of no valid data triggers end frame search
  static String currentByteBuffer = ""; // Buffer to hold the current byte being built
  static int headerBytesReceived = 0; // Track how many header bytes we've received
  static bool headerComplete = false; // Flag to indicate if the header is complete
  
  // First check if we should timeout the entire reception
  if (millis() - lastValidDataTime > dataTimeout) {
    Serial.println("\nData reception timeout (global), resetting to preamble search...");
    currentState = SEEKING_PREAMBLE;
    return;
  }
  
  // Sample at half bit duration for Manchester decoding
  if (millis() - lastBitTime >= bitDuration / 2) {
    bool isHigh = sensorValue > lightThreshold;
    manchesterBits += isHigh ? "1" : "0";
    lastBitTime = millis();
    
    // Process complete Manchester bits
    if (manchesterBits.length() >= 2) {
      // In Manchester, HIGH-LOW is 1, LOW-HIGH is 0
      if (manchesterBits == "10") {
        currentByteBuffer += "1";
        lastValidDataTime = millis(); // Update last valid data time
      } else if (manchesterBits == "01") {
        currentByteBuffer += "0";
        lastValidDataTime = millis(); // Update last valid data time
      } else {
        // Invalid Manchester code, might be transitioning to end frame
        if (manchesterBits == "00" || manchesterBits == "11") {
          static int sameValueCount = 0;
          sameValueCount++;
          
          // Look for end frame after multiple consecutive invalid Manchester codes
          if (sameValueCount >= 3) {
            Serial.println("\nMultiple invalid Manchester codes - looking for end frame");
            sameValueCount = 0;
            currentState = SEEKING_END_FRAME;
            return;
          }
        }
      }
      
      manchesterBits = "";
      
      // Check for inactivity timeout
      if (millis() - lastValidDataTime > inactivityTimeout) {
        Serial.println("\nData inactivity timeout - looking for end frame");
        currentState = SEEKING_END_FRAME;
        return;
      }
      
      // Every 8 bits, convert to a character
      if (currentByteBuffer.length() == 8) {
        char c = binaryToChar(currentByteBuffer);
        receivedBinary += currentByteBuffer;
        receivedData += c;
        
        // Store this byte
        headerBytesReceived++;
        
        // The first 9 bytes are header information (3 filetype + 4 length + 2 CRC)
        if (!headerComplete && headerBytesReceived >= 9) {
          headerComplete = true;
          Serial.println("\n-- Header received, displaying message characters: --");
        }
        
        // Only display characters after the header is complete
        if (headerComplete) {
          // Print the received character immediately
          if (c >= 32 && c <= 126) { // Only print printable ASCII
            Serial.print(c);
          } else {
            Serial.print("?");
          }
        }
        
        // Reset the byte buffer for the next character
        currentByteBuffer = "";
      }
    }
  }
}

// Look for the end frame pattern (00001111)
void detectEndFrame(int sensorValue) {
  static String endFramePattern = "";
  static unsigned long endFrameTimeout = 5000; // Extended from 2000ms to 5000ms
  static unsigned long endFrameStartTime = millis();
  
  // Reset if it's taking too long
  if (millis() - endFrameStartTime > endFrameTimeout) {
    endFramePattern = "";
    endFrameStartTime = millis();
    Serial.println("\nEnd frame timeout, returning to data reception...");
    currentState = RECEIVING_DATA;
    return;
  }
  
  // Sample at appropriate intervals
  if (millis() - lastBitTime >= bitDuration) {
    bool isHigh = sensorValue > lightThreshold;
    endFramePattern += isHigh ? "1" : "0";
    lastBitTime = millis();
    
    // Minimized output - just show a - for each bit
    Serial.print("-");
    
    // Check if we have the complete end frame pattern
    if (endFramePattern.length() >= 8) {
      if (endFramePattern.substring(endFramePattern.length() - 8) == "00001111") {
        Serial.println("\nEnd frame detected!");
        currentState = PROCESSING_DATA;
      } else if (endFramePattern.length() > 16) {
        // If we've collected too many bits without finding the pattern,
        // slide the window and remove old bits
        endFramePattern = endFramePattern.substring(endFramePattern.length() - 16);
      }
    }
  }
}

// Process the received data
void processReceivedData() {
  Serial.println("\n\n--- RECEIVED MESSAGE SUMMARY ---");
  
  if (receivedData.length() < 9) {  // 3 (filetype) + 4 (length) + 2 (CRC)
    Serial.println("Message too short, likely corrupted");
    Serial.print("Received bytes: ");
    Serial.println(receivedData.length());
    Serial.print("Raw data: ");
    for (unsigned int i = 0; i < receivedData.length(); i++) {
      Serial.print((unsigned char)receivedData[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    currentState = SEEKING_PREAMBLE;
    return;
  }
  
  // Extract header information
  String fileType = receivedData.substring(0, 3);
  
  // Extract the data length (4 bytes)
  unsigned long dataLength = (
    (unsigned char)receivedData[3] << 24 |
    (unsigned char)receivedData[4] << 16 |
    (unsigned char)receivedData[5] << 8 |
    (unsigned char)receivedData[6]
  );
  
  // Extract CRC (2 bytes)
  uint16_t receivedCRC = (
    (unsigned char)receivedData[7] << 8 |
    (unsigned char)receivedData[8]
  );
  
  // Extract the actual data
  String actualData = "";
  if (receivedData.length() > 9) {
    actualData = receivedData.substring(9);
  }
  
  // Validate message integrity
  Serial.print("File Type: ");
  Serial.println(fileType);
  
  Serial.print("Data Length: ");
  Serial.print(dataLength);
  Serial.print(" (Actual: ");
  Serial.print(actualData.length());
  Serial.println(")");
  
  uint16_t calculatedCRC = calculateCRC16(actualData);
  Serial.print("CRC: Received=0x");
  Serial.print(receivedCRC, HEX);
  Serial.print(", Calculated=0x");
  Serial.println(calculatedCRC, HEX);
  
  if (calculatedCRC == receivedCRC && actualData.length() == dataLength) {
    Serial.println("CRC Check: PASSED");
  } else {
    Serial.println("CRC Check: FAILED - Message corrupted");
  }
  
  Serial.println("------------------------");
  Serial.println("Waiting for new transmission...");
  
  // Reset for next message
  currentState = SEEKING_PREAMBLE;
}

// Convert 8-bit binary string to character
char binaryToChar(String binary) {
  byte result = 0;
  for (int i = 0; i < 8; i++) {
    if (binary.charAt(i) == '1') {
      result |= (1 << (7 - i));
    }
  }
  return (char)result;
}

// Calculate CRC-16 for validation (same as transmitter)
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