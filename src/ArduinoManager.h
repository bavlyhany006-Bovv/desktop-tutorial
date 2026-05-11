// ============================================================
//  ArduinoManager.h
//  Manages serial communication with the Arduino board.
// ============================================================
#pragma once

#include <QObject>
#include <QString>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

// ─────────────────────────────────────────────────────────────
//  ArduinoResult — plain data returned after each operation
// ─────────────────────────────────────────────────────────────
struct ArduinoResult {
    int  value;       // Computed result (0-255)
    bool overflow;    // True if the 8-bit result overflowed
    bool success;     // False means a communication error occurred
    QString errorMsg; // Human-readable error description
};

// ─────────────────────────────────────────────────────────────
//  ArduinoManager
//  Responsibilities:
//    • Detect and open the Arduino serial port
//    • Send operation commands ("OP:A,B,MODE\n")
//    • Parse the response ("RES:VALUE,OVERFLOW\n")
//    • Emit signals for the UI to react to
// ─────────────────────────────────────────────────────────────
class ArduinoManager : public QObject {
    Q_OBJECT

public:
    explicit ArduinoManager(QObject* parent = nullptr);
    ~ArduinoManager();

    // Try to connect to an Arduino on any available port
    bool        autoConnect();

    // Connect to a specific port (e.g. "COM3" or "/dev/ttyUSB0")
    bool        connectToPort(const QString& portName);

    // Disconnect gracefully
    void        disconnect();

    // Returns true if the serial port is open and ready
    bool        isConnected() const;

    // Send an 8-bit add/subtract operation
    // operandA  : 0-255
    // operandB  : 0-255
    // subtract  : false = add, true = subtract
    // Returns false immediately if not connected
    bool        sendOperation(int operandA, int operandB, bool subtract);

    // List available COM / ttyUSB ports
    static QStringList availablePorts();

signals:
    // Emitted when the Arduino sends back a result
    void resultReceived(ArduinoResult result);

    // Emitted when connection state changes
    void connectionChanged(bool connected);

    // Emitted on any communication error
    void errorOccurred(QString message);

private slots:
    void onReadyRead();          // Called when data arrives on serial port
    void onTimeoutCheck();       // Called if Arduino doesn't respond in time

private:
    QSerialPort* m_serial;       // The actual serial port object
    QTimer*      m_timeout;      // Guards against Arduino hanging

    QString      m_readBuffer;   // Accumulates partial incoming data

    // Parse one complete line from the Arduino
    void processLine(const QString& line);
};
