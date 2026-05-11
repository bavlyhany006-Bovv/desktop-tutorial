# Smart 8-Bit Arithmetic Hardware Solver
### with AI Handwriting Recognition

> A university project combining **hardware logic gates**, **Arduino**, **C++/Qt desktop UI**, and **Gemini AI** into one seamless system.

---

## Project Team

| Name | Role |
|---|---|
| Kerolos Mansour | Hardware & Arduino |
| Bavly Hany | C++ / Qt UI |
| Mariam Ibrahim | AI Integration |
| Ahmed Mohammed | Serial Communication |
| Mina Helal | Testing & Documentation |

**Supervised by:** Innovation University

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Desktop Application                  │
│                    (C++ / Qt6 / OOP)                    │
│                                                         │
│  ┌──────────────┐   ┌──────────────┐  ┌─────────────┐  │
│  │ MainWindow   │   │ArduinoManager│  │  AIManager  │  │
│  │  (UI/UX)     │◄──│ (Serial Comm)│  │(Gemini API) │  │
│  │              │   │              │  │             │  │
│  │  Calculator  │   │ Port Detect  │  │ OCR/Vision  │  │
│  │  AI Camera   │   │ Send/Receive │  │ Parse Eq.   │  │
│  └──────┬───────┘   └──────┬───────┘  └──────┬──────┘  │
│         │                  │                  │         │
│  ┌──────▼──────┐           │         ┌────────▼──────┐  │
│  │CameraManager│           │         │  Gemini 1.5   │  │
│  │  (OpenCV)   │           │         │  Flash API    │  │
│  └─────────────┘           │         └───────────────┘  │
└────────────────────────────┼────────────────────────────┘
                             │ USB Serial
                             ▼
                  ┌─────────────────────┐
                  │   Arduino Mega      │
                  │   (Firmware .ino)   │
                  └──────────┬──────────┘
                             │ GPIO Pins
                             ▼
                  ┌─────────────────────┐
                  │  8-Bit Hardware     │
                  │  Adder/Subtractor   │
                  │  (Logic Gates)      │
                  │                     │
                  │  Blue  LEDs = A     │
                  │  Red   LEDs = B     │
                  │  Green LEDs = Result│
                  │  Yellow LED = Mode  │
                  └─────────────────────┘
```

---

## Features

### Calculator Mode
- Enter two numbers (0–255)
- Choose Addition or Subtraction
- Hardware performs the computation via logic gates
- Binary representation shown in real time
- Result displayed with overflow detection

### AI Camera Mode
- Webcam captures handwritten math problem
- Gemini Vision API performs OCR and equation recognition
- **Decision logic:**
  - Simple 8-bit equations (e.g. `120 + 30`) → forwarded to hardware
  - Complex equations (e.g. `5000 × 12`, `√144`) → solved by AI directly
- Both paths display clearly in the UI

---

## Serial Protocol

**PC → Arduino:**
```
OP:A,B,MODE\n
```
- `A`    = first operand (0–255)
- `B`    = second operand (0–255)
- `MODE` = 0 for addition, 1 for subtraction

**Arduino → PC:**
```
RES:RESULT,OVERFLOW\n
```
- `RESULT`   = computed value (0–255)
- `OVERFLOW` = 1 if result overflowed, else 0

---

## File Structure

```
SmartArithmeticSolver/
│
├── SmartArithmeticSolver.pro   ← Qt project file
│
├── src/
│   ├── main.cpp                ← Application entry point
│   ├── MainWindow.h/.cpp       ← Full UI, wires everything together
│   ├── ArduinoManager.h/.cpp   ← Serial communication
│   ├── AIManager.h/.cpp        ← Gemini API + image analysis
│   └── CameraManager.h/.cpp    ← OpenCV webcam capture
│
├── arduino/
│   └── ArduinoFirmware.ino     ← Upload this to the Arduino board
│
└── docs/
    └── README.md               ← This file
```

---

## How to Build and Run

### Prerequisites
- Qt 5.15 or Qt 6.x (with SerialPort and Network modules)
- OpenCV 4.x
- A Gemini API key (free at https://aistudio.google.com)

### Steps

1. **Get a Gemini API key**
   - Go to https://aistudio.google.com/app/apikey
   - Copy your key

2. **Paste your API key** into `src/MainWindow.cpp`:
   ```cpp
   static const QString GEMINI_API_KEY = "YOUR_KEY_HERE";
   ```

3. **Upload Arduino firmware**
   - Open `arduino/ArduinoFirmware.ino` in Arduino IDE
   - Select your board (Arduino Mega) and port
   - Click Upload

4. **Build the Qt application**
   ```bash
   cd SmartArithmeticSolver
   qmake SmartArithmeticSolver.pro
   make          # or nmake on Windows
   ./SmartArithmeticSolver
   ```

5. **Connect the Arduino**
   - The app auto-detects it on startup
   - Or select the port manually in the connection bar

---

## Hardware Connections (Arduino Mega)

| LED Color | Pins     | Meaning           |
|-----------|----------|-------------------|
| Blue      | D22–D29  | First number (A)  |
| Red       | D30–D37  | Second number (B) |
| Green     | D38–D45  | Result            |
| Yellow    | D46      | Mode (ON=Subtract)|

---

## OOP Design Principles Used

| Principle | Where Applied |
|---|---|
| **Encapsulation** | Each class hides its implementation details behind a clean public interface |
| **Single Responsibility** | Each class has one job (UI, Serial, AI, Camera) |
| **Signals & Slots** | Qt's observer pattern connects components without tight coupling |
| **Separation of Concerns** | UI knows nothing about serial protocol; AI knows nothing about LEDs |

---

## Flow Diagrams

### Calculator Mode Flow
```
User Enters A, B, Operation
         ↓
   Validation (0-255)
         ↓
ArduinoManager.sendOperation()
         ↓
   Arduino receives "OP:A,B,MODE"
         ↓
   Logic gates compute result
         ↓
   LEDs light up (binary)
         ↓
   Arduino sends "RES:value,overflow"
         ↓
   ArduinoManager.resultReceived signal
         ↓
   MainWindow displays result
```

### AI Camera Mode Flow
```
User clicks "AI Solve"
         ↓
CameraManager.captureFrame()
         ↓
AIManager.analyseImage() → Gemini API
         ↓
AI returns recognised equation + type
         ↓
    ┌────┴────┐
 Simple?   Complex?
    ↓           ↓
Arduino      AI answer
hardware     displayed
    ↓
Result shown
```
