// ============================================================
//  MainWindow.h
// ============================================================
#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QStackedWidget>
#include <QProgressBar>
#include <QGroupBox>
#include <QTimer>
#include "ArduinoManager.h"
#include "AIManager.h"
#include "CameraManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

private slots:
    void onCalculateClicked();
    void onAISolveClicked();
    void onCameraFrameReady(QImage frame);
    void onAIAnalysisComplete(ParsedEquation result);
    void onArduinoResult(ArduinoResult result);
    void onArduinoConnection(bool connected);
    void onArduinoError(QString msg);
    void onConnectPortClicked();
    void refreshPorts();
    void onCountdownTick();   // NEW: handles the 3-2-1 countdown

private:
    ArduinoManager* m_arduino;
    AIManager*      m_ai;
    CameraManager*  m_camera;

    // NEW: countdown timer & counter
    QTimer* m_countdownTimer;
    int     m_countdownValue;

    void     buildUI();
    void     buildHeader(QWidget* parent, class QVBoxLayout* layout);
    QWidget* buildCalculatorPage();
    QWidget* buildAIPage();
    QWidget* buildFooter();
    QWidget* buildConnectionBar();
    void setStatus(const QString& msg, const QString& color = "#64FFDA");
    void setBinaryDisplay(QLabel* label, QLabel** leds,
                          int value, const QString& onColor);
    static QString appStyleSheet();

    QStackedWidget* m_pages;

    // Calculator page
    QLineEdit*  m_inputA;
    QLineEdit*  m_inputB;
    QComboBox*  m_opSelector;
    QLabel*     m_calcBinA;
    QLabel*     m_calcBinB;
    QLabel*     m_calcBinResult;
    QLabel*     m_calcResult;
    QLabel*     m_calcEquation;
    QLabel*     m_calcSolvedBy;

    // LED arrays for binary display (8 bits each)
    QLabel*     m_ledsA[8];
    QLabel*     m_ledsB[8];
    QLabel*     m_ledsResult[8];

    // AI page
    QLabel*      m_cameraView;
    QLabel*      m_aiEquation;
    QLabel*      m_aiResult;
    QLabel*      m_aiSolvedBy;
    QLabel*      m_aiStatus;
    QPushButton* m_aiSolveBtn;

    // Shared
    QLabel*    m_statusLabel;
    QLabel*    m_connectionIndicator;
    QComboBox* m_portCombo;

    ParsedEquation m_pendingEquation;
    bool m_waitingForArduinoAfterAI = false;
};