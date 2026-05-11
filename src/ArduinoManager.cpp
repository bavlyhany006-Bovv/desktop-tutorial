// ============================================================
//  ArduinoManager.cpp
//  Serial communication with the Arduino 8-bit adder/subtractor.
// ============================================================
#include "ArduinoManager.h"
#include <QDebug>

// How long (ms) to wait for a response before giving up
static const int TIMEOUT_MS = 3000;

// ─────────────────────────────────────────────────────────────
ArduinoManager::ArduinoManager(QObject* parent)
    : QObject(parent)
{
    // Create serial port (no port opened yet)
    m_serial = new QSerialPort(this);
    m_serial->setBaudRate(QSerialPort::Baud9600);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_serial, &QSerialPort::readyRead,
            this,     &ArduinoManager::onReadyRead);

    // Timeout timer — single-shot, restarted each time we send
    m_timeout = new QTimer(this);
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout,
            this,      &ArduinoManager::onTimeoutCheck);
}

// ─────────────────────────────────────────────────────────────
ArduinoManager::~ArduinoManager() {
    disconnect();
}

// ─────────────────────────────────────────────────────────────
//  Scan all available ports and try to open an Arduino
// ─────────────────────────────────────────────────────────────
bool ArduinoManager::autoConnect() {
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto& info : ports) {
        // Arduino boards show up as "Arduino" or "CH340" or "FTDI"
        if (info.description().contains("Arduino", Qt::CaseInsensitive) ||
            info.description().contains("CH340",   Qt::CaseInsensitive) ||
            info.description().contains("FTDI",    Qt::CaseInsensitive) ||
            info.manufacturer().contains("Arduino", Qt::CaseInsensitive)) {

            if (connectToPort(info.portName())) {
                return true;
            }
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
//  Open a specific serial port
// ─────────────────────────────────────────────────────────────
bool ArduinoManager::connectToPort(const QString& portName) {
    if (m_serial->isOpen()) {
        m_serial->close();
    }

    m_serial->setPortName(portName);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred("Cannot open port: " + portName);
        return false;
    }

    m_readBuffer.clear();
    emit connectionChanged(true);
    qDebug() << "[Arduino] Connected to" << portName;
    return true;
}

// ─────────────────────────────────────────────────────────────
void ArduinoManager::disconnect() {
    if (m_serial->isOpen()) {
        m_serial->close();
        emit connectionChanged(false);
    }
    m_timeout->stop();
}

// ─────────────────────────────────────────────────────────────
bool ArduinoManager::isConnected() const {
    return m_serial->isOpen();
}

// ─────────────────────────────────────────────────────────────
//  Send command to Arduino:  "OP:A,B,MODE\n"
// ─────────────────────────────────────────────────────────────
bool ArduinoManager::sendOperation(int operandA, int operandB, bool subtract) {
    if (!isConnected()) {
        emit errorOccurred("Arduino not connected.");
        return false;
    }

    // Clamp to 8-bit range
    operandA = qBound(0, operandA, 255);
    operandB = qBound(0, operandB, 255);
    int mode = subtract ? 1 : 0;

    // Build the command string
    QString cmd = QString("OP:%1,%2,%3\n").arg(operandA).arg(operandB).arg(mode);

    qDebug() << "[Arduino] Sending:" << cmd.trimmed();
    m_serial->write(cmd.toUtf8());

    // Start watchdog — if no reply in TIMEOUT_MS → error
    m_timeout->start(TIMEOUT_MS);
    return true;
}

// ─────────────────────────────────────────────────────────────
QStringList ArduinoManager::availablePorts() {
    QStringList names;
    for (const auto& info : QSerialPortInfo::availablePorts()) {
        names << info.portName();
    }
    return names;
}

// ─────────────────────────────────────────────────────────────
//  Slot: data arrived on serial port
// ─────────────────────────────────────────────────────────────
void ArduinoManager::onReadyRead() {
    // Append incoming bytes to the buffer
    m_readBuffer += QString::fromUtf8(m_serial->readAll());

    // Process any complete lines (ending with '\n')
    while (m_readBuffer.contains('\n')) {
        int idx  = m_readBuffer.indexOf('\n');
        QString line = m_readBuffer.left(idx).trimmed();
        m_readBuffer = m_readBuffer.mid(idx + 1);

        if (!line.isEmpty()) {
            processLine(line);
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  Parse "RES:VALUE,OVERFLOW" from Arduino
// ─────────────────────────────────────────────────────────────
void ArduinoManager::processLine(const QString& line) {
    qDebug() << "[Arduino] Received:" << line;

    if (line.startsWith("RES:")) {
        m_timeout->stop();   // Got a reply — cancel watchdog

        QString data = line.mid(4);          // Remove "RES:" prefix
        QStringList parts = data.split(',');

        if (parts.size() < 2) {
            emit errorOccurred("Malformed response from Arduino: " + line);
            return;
        }

        ArduinoResult res;
        res.value    = parts[0].toInt();
        res.overflow = (parts[1].toInt() == 1);
        res.success  = true;
        res.errorMsg = "";

        emit resultReceived(res);

    } else if (line.startsWith("ERR:")) {
        m_timeout->stop();
        ArduinoResult res;
        res.success  = false;
        res.errorMsg = line.mid(4);
        emit resultReceived(res);

    } else if (line == "READY") {
        qDebug() << "[Arduino] Firmware ready.";
    }
    // Ignore any other debug lines the Arduino might print
}

// ─────────────────────────────────────────────────────────────
//  Slot: Arduino didn't respond in time
// ─────────────────────────────────────────────────────────────
void ArduinoManager::onTimeoutCheck() {
    ArduinoResult res;
    res.success  = false;
    res.errorMsg = "Arduino response timed out. Check connection.";
    emit resultReceived(res);
    emit errorOccurred(res.errorMsg);
}
