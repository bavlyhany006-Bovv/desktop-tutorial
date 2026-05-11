// ============================================================
//  Smart 8-Bit Arithmetic Hardware Solver
//  Arduino Firmware — 8-Bit Adder / Subtractor
// ============================================================
//
//  HARDWARE CONNECTIONS:
//  ─────────────────────────────────────────────────────────
//  Blue  LEDs  (D22-D29) → First operand  (A bits 0-7)
//  Red   LEDs  (D30-D37) → Second operand (B bits 0-7)
//  Green LEDs  (D38-D45) → Result         (bits 0-7)
//  Yellow LED  (D46)     → Mode: LOW=Add / HIGH=Subtract
//
//  SERIAL PROTOCOL:
//  ─────────────────────────────────────────────────────────
//  PC → Arduino:   "OP:A,B,MODE\n"
//    A    = first number  (0-255)
//    B    = second number (0-255)
//    MODE = 0 (addition) / 1 (subtraction)
//
//  Arduino → PC:   "RES:RESULT,OVERFLOW\n"
//    RESULT   = computed value (0-255)
//    OVERFLOW = 1 if overflow occurred, else 0
// ============================================================

// ── Pin Definitions ─────────────────────────────────────────
const int BLUE_LED_START   = 22;  // First number LEDs (8 pins)
const int RED_LED_START    = 30;  // Second number LEDs (8 pins)
const int GREEN_LED_START  = 38;  // Result LEDs (8 pins)
const int YELLOW_LED_PIN   = 46;  // Operation mode LED

const int BAUD_RATE        = 9600;

// ── State Variables ─────────────────────────────────────────
int  operandA  = 0;
int  operandB  = 0;
int  opMode    = 0;   // 0 = add, 1 = subtract
int  result    = 0;
bool overflow  = false;

// ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(BAUD_RATE);

    // Set up all LED pins as OUTPUT
    for (int i = 0; i < 8; i++) {
        pinMode(BLUE_LED_START  + i, OUTPUT);
        pinMode(RED_LED_START   + i, OUTPUT);
        pinMode(GREEN_LED_START + i, OUTPUT);
    }
    pinMode(YELLOW_LED_PIN, OUTPUT);

    // Initial state: all LEDs off
    clearAllLEDs();

    Serial.println("READY");   // Signal to PC that Arduino is ready
}

// ─────────────────────────────────────────────────────────────
void loop() {
    // Wait for a complete command from the PC
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command.startsWith("OP:")) {
            parseAndExecute(command);
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  Parse incoming command and run the operation
// ─────────────────────────────────────────────────────────────
void parseAndExecute(String cmd) {
    // Remove "OP:" prefix
    String data = cmd.substring(3);

    // Split by commas: "A,B,MODE"
    int firstComma  = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);

    if (firstComma == -1 || secondComma == -1) {
        Serial.println("ERR:BAD_FORMAT");
        return;
    }

    operandA = data.substring(0, firstComma).toInt();
    operandB = data.substring(firstComma + 1, secondComma).toInt();
    opMode   = data.substring(secondComma + 1).toInt();

    // Clamp inputs to valid 8-bit range
    operandA = constrain(operandA, 0, 255);
    operandB = constrain(operandB, 0, 255);
    opMode   = constrain(opMode,   0, 1);

    // Perform arithmetic
    computeResult();

    // Light up LEDs to show binary representation
    displayOnLEDs();

    // Send result back to PC
    sendResult();
}

// ─────────────────────────────────────────────────────────────
//  Perform 8-bit add or subtract with overflow detection
// ─────────────────────────────────────────────────────────────
void computeResult() {
    int rawResult;

    if (opMode == 0) {
        // Addition
        rawResult = operandA + operandB;
        overflow  = (rawResult > 255);
        result    = rawResult & 0xFF;   // Keep only lower 8 bits
    } else {
        // Subtraction (two's complement, clamp at 0)
        rawResult = operandA - operandB;
        overflow  = (rawResult < 0);
        result    = overflow ? 0 : rawResult;
    }
}

// ─────────────────────────────────────────────────────────────
//  Write a byte value across 8 LED pins
// ─────────────────────────────────────────────────────────────
void writeByte(int startPin, int value) {
    for (int bit = 0; bit < 8; bit++) {
        // bit 0 = LSB on startPin, bit 7 = MSB on startPin+7
        bool state = (value >> bit) & 1;
        digitalWrite(startPin + bit, state ? HIGH : LOW);
    }
}

// ─────────────────────────────────────────────────────────────
//  Display all three values on corresponding LED banks
// ─────────────────────────────────────────────────────────────
void displayOnLEDs() {
    writeByte(BLUE_LED_START,  operandA);
    writeByte(RED_LED_START,   operandB);
    writeByte(GREEN_LED_START, result);

    // Yellow LED: HIGH = subtraction, LOW = addition
    digitalWrite(YELLOW_LED_PIN, opMode == 1 ? HIGH : LOW);
}

// ─────────────────────────────────────────────────────────────
//  Send the result back to the desktop application
// ─────────────────────────────────────────────────────────────
void sendResult() {
    Serial.print("RES:");
    Serial.print(result);
    Serial.print(",");
    Serial.println(overflow ? 1 : 0);
}

// ─────────────────────────────────────────────────────────────
//  Turn off all LEDs (reset state)
// ─────────────────────────────────────────────────────────────
void clearAllLEDs() {
    for (int i = 0; i < 8; i++) {
        digitalWrite(BLUE_LED_START  + i, LOW);
        digitalWrite(RED_LED_START   + i, LOW);
        digitalWrite(GREEN_LED_START + i, LOW);
    }
    digitalWrite(YELLOW_LED_PIN, LOW);
}
