// ============================================================
//  MainWindow.cpp
// ============================================================
#include "MainWindow.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QFrame>
#include <QFont>
#include <QFontDatabase>
#include <QSplitter>
#include <QTimer>
#include <QIntValidator>
#include <QMessageBox>
#include <QDebug>

static const QString GEMINI_API_KEY = "AIzaSyCp9YqeoUACdLvgHixhxZgtGcnOLiI55wg";

// ─────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_countdownValue(0)
{
    m_arduino = new ArduinoManager(this);
    m_ai      = new AIManager(GEMINI_API_KEY, this);
    m_camera  = new CameraManager(this);

    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(1000);
    connect(m_countdownTimer, &QTimer::timeout, this, &MainWindow::onCountdownTick);

    connect(m_arduino, &ArduinoManager::resultReceived,    this, &MainWindow::onArduinoResult);
    connect(m_arduino, &ArduinoManager::connectionChanged, this, &MainWindow::onArduinoConnection);
    connect(m_arduino, &ArduinoManager::errorOccurred,     this, &MainWindow::onArduinoError);

    connect(m_ai, &AIManager::analysisComplete,  this, &MainWindow::onAIAnalysisComplete);
    connect(m_ai, &AIManager::processingStarted, this, [this]{
        m_aiStatus->setText("🤖  Analysing handwriting with Gemini AI...");
        m_aiSolveBtn->setEnabled(false);
    });
    connect(m_ai, &AIManager::errorOccurred, this, [this](const QString& msg){
        m_aiStatus->setText("⚠  AI Error: " + msg);
        m_aiSolveBtn->setEnabled(true);
        setStatus(msg, "#FF5252");
    });

    connect(m_camera, &CameraManager::frameReady,   this, &MainWindow::onCameraFrameReady);
    connect(m_camera, &CameraManager::cameraError,  this, [this](const QString& msg){
        setStatus("Camera: " + msg, "#FF5252");
    });

    buildUI();

    if (!m_arduino->autoConnect())
        setStatus("Arduino not found — select port manually.", "#FFB74D");

    setWindowTitle("Smart 8-Bit Arithmetic Hardware Solver");
    setMinimumSize(1100, 740);
    setStyleSheet(appStyleSheet());
    show();
}

// ─────────────────────────────────────────────────────────────
void MainWindow::buildUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* root = new QVBoxLayout(central);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    buildHeader(central, root);
    root->addWidget(buildConnectionBar());

    QWidget* tabBar = new QWidget;
    tabBar->setObjectName("tabBar");
    QHBoxLayout* tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setSpacing(4);
    tabLayout->setContentsMargins(20, 8, 20, 8);

    auto makeTab = [&](const QString& icon, const QString& label, int pageIdx) {
        QPushButton* btn = new QPushButton(icon + "  " + label);
        btn->setObjectName("tabBtn");
        btn->setCheckable(true);
        btn->setChecked(pageIdx == 0);
        btn->setFixedHeight(36);
        connect(btn, &QPushButton::clicked, this, [this, btn, pageIdx, tabLayout] {
            m_pages->setCurrentIndex(pageIdx);
            for (int i = 0; i < tabLayout->count(); ++i) {
                auto* b = qobject_cast<QPushButton*>(tabLayout->itemAt(i)->widget());
                if (b) b->setChecked(false);
            }
            btn->setChecked(true);
            if (pageIdx == 1) m_camera->startCamera();
            else              m_camera->stopCamera();
        });
        return btn;
    };

    tabLayout->addWidget(makeTab("⊞", "Calculator Mode", 0));
    tabLayout->addWidget(makeTab("◉", "AI Camera Mode",  1));
    tabLayout->addStretch();
    root->addWidget(tabBar);

    m_pages = new QStackedWidget;
    m_pages->addWidget(buildCalculatorPage());
    m_pages->addWidget(buildAIPage());
    root->addWidget(m_pages, 1);

    root->addWidget(buildFooter());
}

// ─────────────────────────────────────────────────────────────
void MainWindow::buildHeader(QWidget*, QVBoxLayout* layout) {
    QWidget* header = new QWidget;
    header->setObjectName("header");
    header->setFixedHeight(72);

    QHBoxLayout* h = new QHBoxLayout(header);
    h->setContentsMargins(24, 0, 24, 0);

    QLabel* icon = new QLabel("◈");
    icon->setObjectName("headerIcon");

    QLabel* title = new QLabel("Smart 8-Bit Arithmetic Hardware Solver");
    title->setObjectName("headerTitle");

    QLabel* sub = new QLabel("Hardware  ·  AI  ·  Arduino");
    sub->setObjectName("headerSub");

    QVBoxLayout* titleGroup = new QVBoxLayout;
    titleGroup->setSpacing(2);
    titleGroup->addWidget(title);
    titleGroup->addWidget(sub);

    h->addWidget(icon);
    h->addSpacing(12);
    h->addLayout(titleGroup);
    h->addStretch();

    m_statusLabel = new QLabel("Initializing...");
    m_statusLabel->setObjectName("statusLabel");
    h->addWidget(m_statusLabel);

    layout->addWidget(header);
}

// ─────────────────────────────────────────────────────────────
QWidget* MainWindow::buildConnectionBar() {
    QWidget* bar = new QWidget;
    bar->setObjectName("connBar");
    bar->setFixedHeight(42);

    QHBoxLayout* h = new QHBoxLayout(bar);
    h->setContentsMargins(20, 0, 20, 0);
    h->setSpacing(10);

    m_connectionIndicator = new QLabel("●  Disconnected");
    m_connectionIndicator->setObjectName("connIndicatorOff");

    QLabel* portLabel = new QLabel("Port:");
    portLabel->setObjectName("dimLabel");

    m_portCombo = new QComboBox;
    m_portCombo->setObjectName("portCombo");
    m_portCombo->setFixedWidth(140);
    refreshPorts();

    QPushButton* connectBtn = new QPushButton("Connect");
    connectBtn->setObjectName("smallBtn");
    connect(connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectPortClicked);

    QPushButton* refreshBtn = new QPushButton("↺");
    refreshBtn->setObjectName("iconBtn");
    refreshBtn->setFixedWidth(30);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshPorts);

    h->addWidget(m_connectionIndicator);
    h->addSpacing(16);
    h->addWidget(portLabel);
    h->addWidget(m_portCombo);
    h->addWidget(refreshBtn);
    h->addWidget(connectBtn);
    h->addStretch();

    return bar;
}

// ─────────────────────────────────────────────────────────────
//  CALCULATOR PAGE
// ─────────────────────────────────────────────────────────────
QWidget* MainWindow::buildCalculatorPage() {
    QWidget* page = new QWidget;
    QHBoxLayout* main = new QHBoxLayout(page);
    main->setContentsMargins(24, 20, 24, 20);
    main->setSpacing(20);

    // ── Left panel ────────────────────────────────────────────
    QWidget* leftPanel = new QWidget;
    leftPanel->setObjectName("card");
    leftPanel->setFixedWidth(320);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(16);
    leftLayout->setContentsMargins(20, 20, 20, 20);

    QLabel* inputTitle = new QLabel("Enter Values");
    inputTitle->setObjectName("cardTitle");
    leftLayout->addWidget(inputTitle);

    QLabel* labelA = new QLabel("First Number  (0 – 255)");
    labelA->setObjectName("inputLabel");
    m_inputA = new QLineEdit("120");
    m_inputA->setObjectName("numInput");
    m_inputA->setValidator(new QIntValidator(0, 255, this));

    QLabel* labelB = new QLabel("Second Number  (0 – 255)");
    labelB->setObjectName("inputLabel");
    m_inputB = new QLineEdit("30");
    m_inputB->setObjectName("numInput");
    m_inputB->setValidator(new QIntValidator(0, 255, this));

    QLabel* labelOp = new QLabel("Operation");
    labelOp->setObjectName("inputLabel");
    m_opSelector = new QComboBox;
    m_opSelector->setObjectName("opCombo");
    m_opSelector->addItem("➕  Addition",    0);
    m_opSelector->addItem("➖  Subtraction", 1);

    leftLayout->addWidget(labelA);
    leftLayout->addWidget(m_inputA);
    leftLayout->addWidget(labelB);
    leftLayout->addWidget(m_inputB);
    leftLayout->addWidget(labelOp);
    leftLayout->addWidget(m_opSelector);
    leftLayout->addSpacing(8);

    QPushButton* calcBtn = new QPushButton("▶  Send to Hardware");
    calcBtn->setObjectName("primaryBtn");
    calcBtn->setFixedHeight(44);
    connect(calcBtn, &QPushButton::clicked, this, &MainWindow::onCalculateClicked);
    leftLayout->addWidget(calcBtn);
    leftLayout->addStretch();

    // ── Right panel ───────────────────────────────────────────
    QWidget* rightPanel = new QWidget;
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(16);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // ── Binary + LED card ─────────────────────────────────────
    // CHANGE 1: كل الـ card بـ background واحد موحد
    QWidget* binCard = new QWidget;
    binCard->setObjectName("card");
    QVBoxLayout* binLayout = new QVBoxLayout(binCard);
    binLayout->setContentsMargins(24, 20, 24, 20);
    binLayout->setSpacing(0); // CHANGE 2: إزالة الـ spacing بين الصفوف

    QLabel* binTitle = new QLabel("Binary Representation  —  LED Display");
    binTitle->setObjectName("cardTitle");
    binLayout->addWidget(binTitle);
    binLayout->addSpacing(16);

    // CHANGE 3: separator خفيف بين الصفوف بدل الـ background المختلف
    auto makeLedRow = [&](const QString& rowLabel,
                          const QString& onColor,
                          QLabel** textOut,
                          QLabel** leds,
                          bool addSeparator)
    {
        QWidget* rowWidget = new QWidget;
        rowWidget->setObjectName("ledRow"); // موحد لكل الصفوف
        QHBoxLayout* row = new QHBoxLayout(rowWidget);
        row->setSpacing(8);
        row->setContentsMargins(12, 14, 12, 14); // padding داخلي متساوي

        // Label ثابت العرض
        QLabel* lbl = new QLabel(rowLabel);
        lbl->setObjectName("ledRowLabel");
        lbl->setFixedWidth(72);
        lbl->setStyleSheet("color:" + onColor + "; font-weight:700; font-size:12px; letter-spacing:0.5px;");
        row->addWidget(lbl);

        // فاصل خفيف
        QFrame* vLine = new QFrame;
        vLine->setFrameShape(QFrame::VLine);
        vLine->setStyleSheet("color: #21262D; margin: 4px 0;");
        row->addWidget(vLine);
        row->addSpacing(8);

        // الـ LEDs مقسمة 4+4
        for (int i = 0; i < 8; i++) {
            leds[i] = new QLabel;
            leds[i]->setFixedSize(24, 24); // CHANGE 4: حجم أكبر شوية
            leds[i]->setStyleSheet(
                QString("background: #1A1F2E;"
                        "border-radius: 12px;"
                        "border: 1.5px solid #2A3040;") // CHANGE 5: off color موحد لكل الصفوف
                );
            row->addWidget(leds[i]);
            if (i == 3) row->addSpacing(12); // فراغ بين الـ nibbles
        }

        row->addSpacing(20);

        // Binary text
        *textOut = new QLabel("0000  0000");
        (*textOut)->setObjectName("binValue");
        (*textOut)->setStyleSheet(
            "color:" + onColor + ";"
                                 "font-family: 'Courier New', monospace;"
                                 "font-size: 16px;"
                                 "font-weight: 700;"
                                 "letter-spacing: 3px;"
            );
        row->addWidget(*textOut);
        row->addStretch();

        binLayout->addWidget(rowWidget);

        // CHANGE 6: separator خفيف بين الصفوف (مش آخر صف)
        if (addSeparator) {
            QFrame* hLine = new QFrame;
            hLine->setFrameShape(QFrame::HLine);
            hLine->setStyleSheet("color: #21262D; margin: 0 12px;");
            binLayout->addWidget(hLine);
        }
    };

    makeLedRow("A  (Blue)",   "#448AFF", &m_calcBinA,      m_ledsA,      true);
    makeLedRow("B  (Red)",    "#FF5252", &m_calcBinB,      m_ledsB,      true);
    makeLedRow("=  (Green)",  "#69F0AE", &m_calcBinResult, m_ledsResult, false);

    // CHANGE 7: stretch = 0 للـ binCard عشان ميبقاش فيه مساحة فاضية
    rightLayout->addWidget(binCard, 0);

    // Result card
    QWidget* resCard = new QWidget;
    resCard->setObjectName("resultCard");
    QVBoxLayout* resLayout = new QVBoxLayout(resCard);
    resLayout->setContentsMargins(24, 20, 24, 20);
    resLayout->setSpacing(6);

    m_calcEquation = new QLabel("—");
    m_calcEquation->setObjectName("equationLabel");
    m_calcEquation->setAlignment(Qt::AlignCenter);

    m_calcResult = new QLabel("—");
    m_calcResult->setObjectName("bigResult");
    m_calcResult->setAlignment(Qt::AlignCenter);

    m_calcSolvedBy = new QLabel("Awaiting input");
    m_calcSolvedBy->setObjectName("solvedBy");
    m_calcSolvedBy->setAlignment(Qt::AlignCenter);

    resLayout->addWidget(m_calcEquation);
    resLayout->addWidget(m_calcResult);
    resLayout->addWidget(m_calcSolvedBy);

    // CHANGE 8: stretch = 1 بس للـ resultCard
    rightLayout->addWidget(resCard, 1);

    main->addWidget(leftPanel);
    main->addWidget(rightPanel, 1);

    return page;
}

// ─────────────────────────────────────────────────────────────
//  AI CAMERA PAGE
// ─────────────────────────────────────────────────────────────
QWidget* MainWindow::buildAIPage() {
    QWidget* page = new QWidget;
    QHBoxLayout* main = new QHBoxLayout(page);
    main->setContentsMargins(24, 20, 24, 20);
    main->setSpacing(20);

    QWidget* camCard = new QWidget;
    camCard->setObjectName("card");
    QVBoxLayout* camLayout = new QVBoxLayout(camCard);
    camLayout->setContentsMargins(12, 12, 12, 12);
    camLayout->setSpacing(10);

    QLabel* camTitle = new QLabel("Live Camera Preview");
    camTitle->setObjectName("cardTitle");
    camLayout->addWidget(camTitle);

    m_cameraView = new QLabel;
    m_cameraView->setObjectName("cameraView");
    m_cameraView->setMinimumSize(480, 360);
    m_cameraView->setAlignment(Qt::AlignCenter);
    m_cameraView->setText("Camera off");
    camLayout->addWidget(m_cameraView, 1);

    m_aiSolveBtn = new QPushButton("◉  Capture & Solve  (3 sec countdown)");
    m_aiSolveBtn->setObjectName("primaryBtn");
    m_aiSolveBtn->setFixedHeight(44);
    connect(m_aiSolveBtn, &QPushButton::clicked, this, &MainWindow::onAISolveClicked);
    camLayout->addWidget(m_aiSolveBtn);

    QWidget* aiPanel = new QWidget;
    aiPanel->setFixedWidth(300);
    QVBoxLayout* aiLayout = new QVBoxLayout(aiPanel);
    aiLayout->setSpacing(14);
    aiLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* statusCard = new QWidget;
    statusCard->setObjectName("card");
    QVBoxLayout* sLayout = new QVBoxLayout(statusCard);
    sLayout->setContentsMargins(16, 14, 16, 14);

    QLabel* aiLabel = new QLabel("AI Recognition Status");
    aiLabel->setObjectName("cardTitle");
    m_aiStatus = new QLabel("Press 'Capture & Solve' to begin.");
    m_aiStatus->setObjectName("aiStatus");
    m_aiStatus->setWordWrap(true);

    sLayout->addWidget(aiLabel);
    sLayout->addWidget(m_aiStatus);
    aiLayout->addWidget(statusCard);

    QWidget* eqCard = new QWidget;
    eqCard->setObjectName("card");
    QVBoxLayout* eqLayout = new QVBoxLayout(eqCard);
    eqLayout->setContentsMargins(16, 14, 16, 14);

    QLabel* eqTitle = new QLabel("Recognised Equation");
    eqTitle->setObjectName("cardTitle");
    m_aiEquation = new QLabel("—");
    m_aiEquation->setObjectName("aiEquationLabel");
    m_aiEquation->setWordWrap(true);

    eqLayout->addWidget(eqTitle);
    eqLayout->addWidget(m_aiEquation);
    aiLayout->addWidget(eqCard);

    QWidget* aiResCard = new QWidget;
    aiResCard->setObjectName("resultCard");
    QVBoxLayout* arLayout = new QVBoxLayout(aiResCard);
    arLayout->setContentsMargins(16, 16, 16, 16);
    arLayout->setSpacing(6);

    m_aiResult = new QLabel("—");
    m_aiResult->setObjectName("bigResult");
    m_aiResult->setAlignment(Qt::AlignCenter);

    m_aiSolvedBy = new QLabel("Awaiting capture");
    m_aiSolvedBy->setObjectName("solvedBy");
    m_aiSolvedBy->setAlignment(Qt::AlignCenter);

    arLayout->addWidget(m_aiResult);
    arLayout->addWidget(m_aiSolvedBy);
    aiLayout->addWidget(aiResCard);
    aiLayout->addStretch();

    main->addWidget(camCard, 1);
    main->addWidget(aiPanel);

    return page;
}

// ─────────────────────────────────────────────────────────────
QWidget* MainWindow::buildFooter() {
    QWidget* footer = new QWidget;
    footer->setObjectName("footer");
    footer->setFixedHeight(40);

    QHBoxLayout* h = new QHBoxLayout(footer);
    h->setContentsMargins(20, 0, 20, 0);

    QLabel* credits = new QLabel(
        "Developed by  Kerolos Mansour · Bavly Hany · Mariam Ibrahim · "
        "Ahmed Mohammed · Mina Helal   |   Supervised by Innovation University"
        );
    credits->setObjectName("footerLabel");
    credits->setAlignment(Qt::AlignCenter);

    h->addWidget(credits);
    return footer;
}

// ─────────────────────────────────────────────────────────────
//  SLOTS
// ─────────────────────────────────────────────────────────────
void MainWindow::onCalculateClicked() {
    bool okA, okB;
    int a = m_inputA->text().toInt(&okA);
    int b = m_inputB->text().toInt(&okB);

    if (!okA || !okB) {
        setStatus("Please enter valid numbers (0-255).", "#FF5252");
        return;
    }

    bool sub = (m_opSelector->currentIndex() == 1);

    if (sub && a < b)
        setStatus("Subtraction result would be negative — hardware clamps to 0.", "#FFB74D");

    m_waitingForArduinoAfterAI = false;

    setBinaryDisplay(m_calcBinA,      m_ledsA,      a, "#448AFF");
    setBinaryDisplay(m_calcBinB,      m_ledsB,      b, "#FF5252");
    setBinaryDisplay(m_calcBinResult, m_ledsResult, 0, "#69F0AE");

    m_calcEquation->setText(QString::number(a) + (sub ? " − " : " + ") + QString::number(b) + "  =  ?");
    m_calcResult->setText("...");
    m_calcSolvedBy->setText("Waiting for Arduino hardware...");

    setStatus("Sending operation to Arduino hardware...");
    m_arduino->sendOperation(a, b, sub);
}

// ─────────────────────────────────────────────────────────────
void MainWindow::onAISolveClicked() {
    if (!m_camera->isOpen()) {
        setStatus("Camera not available.", "#FF5252");
        return;
    }

    m_aiSolveBtn->setEnabled(false);
    m_countdownValue = 3;

    m_aiStatus->setText(QString("📷  Hold still… capturing in  %1").arg(m_countdownValue));
    setStatus(QString("Capturing in %1 seconds...").arg(m_countdownValue));

    m_countdownTimer->start();
}

// ─────────────────────────────────────────────────────────────
void MainWindow::onCountdownTick() {
    m_countdownValue--;

    if (m_countdownValue > 0) {
        m_aiStatus->setText(QString("📷  Hold still… capturing in  %1").arg(m_countdownValue));
        setStatus(QString("Capturing in %1 seconds...").arg(m_countdownValue));
    } else {
        m_countdownTimer->stop();

        QImage frame = m_camera->captureFrame();
        if (frame.isNull()) {
            setStatus("Failed to capture frame.", "#FF5252");
            m_aiSolveBtn->setEnabled(true);
            m_aiStatus->setText("⚠  Capture failed. Try again.");
            return;
        }

        m_aiEquation->setText("Analysing...");
        m_aiResult->setText("...");
        m_aiSolvedBy->setText("Processing");
        m_aiStatus->setText("📸  Image captured! Sending to Gemini AI...");
        setStatus("Sending image to Gemini AI...");
        m_ai->analyseImage(frame);
    }
}

// ─────────────────────────────────────────────────────────────
void MainWindow::onCameraFrameReady(QImage frame) {
    QPixmap pix = QPixmap::fromImage(frame);
    m_cameraView->setPixmap(
        pix.scaled(m_cameraView->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

// ─────────────────────────────────────────────────────────────
void MainWindow::onAIAnalysisComplete(ParsedEquation result) {
    m_aiSolveBtn->setEnabled(true);

    if (!result.success) {
        m_aiStatus->setText("⚠  Could not parse: " + result.errorMsg);
        setStatus("AI analysis failed.", "#FF5252");
        return;
    }

    m_aiEquation->setText(result.rawText + "\n\n" + result.explanation);
    m_aiStatus->setText("✓  Equation recognised successfully.");

    if (result.isSimple8Bit) {
        m_pendingEquation          = result;
        m_waitingForArduinoAfterAI = true;
        setStatus("Forwarding to Arduino hardware...");
        m_aiSolvedBy->setText("→ Sending to 8-bit hardware...");
        m_arduino->sendOperation(result.operandA, result.operandB, result.isSubtraction);
    } else {
        m_aiResult->setText(QString::number(result.aiSolvedAnswer, 'g', 10));
        m_aiSolvedBy->setText("⚛  Solved by AI (beyond 8-bit hardware)");
        setStatus("Complex equation solved by Gemini AI.");
    }
}

// ─────────────────────────────────────────────────────────────
void MainWindow::onArduinoResult(ArduinoResult result) {
    if (!result.success) {
        setStatus("Hardware error: " + result.errorMsg, "#FF5252");
        return;
    }

    if (m_waitingForArduinoAfterAI) {
        m_waitingForArduinoAfterAI = false;
        m_aiResult->setText(QString::number(result.value) + (result.overflow ? "  ⚠" : ""));
        m_aiSolvedBy->setText("⊞  Solved by 8-bit Hardware (Arduino)");
        setStatus(result.overflow ? "Result computed with overflow." : "Hardware result received.");
    } else {
        bool sub = (m_opSelector->currentIndex() == 1);
        int  a   = m_inputA->text().toInt();
        int  b   = m_inputB->text().toInt();

        setBinaryDisplay(m_calcBinResult, m_ledsResult, result.value, "#69F0AE");

        m_calcEquation->setText(
            QString::number(a) + (sub ? " − " : " + ") +
            QString::number(b) + "  =  " + QString::number(result.value));
        m_calcResult->setText(QString::number(result.value) + (result.overflow ? " ⚠" : ""));
        m_calcSolvedBy->setText("⊞  Solved by 8-bit Hardware (Arduino)");

        setStatus(result.overflow ? "Operation complete — overflow detected!" : "Operation complete.",
                  result.overflow ? "#FFB74D" : "#64FFDA");
    }
}

// ─────────────────────────────────────────────────────────────
void MainWindow::onArduinoConnection(bool connected) {
    if (connected) {
        m_connectionIndicator->setText("●  Arduino Connected");
        m_connectionIndicator->setObjectName("connIndicatorOn");
        setStatus("Arduino connected and ready.");
    } else {
        m_connectionIndicator->setText("●  Disconnected");
        m_connectionIndicator->setObjectName("connIndicatorOff");
        setStatus("Arduino disconnected.", "#FF5252");
    }
    m_connectionIndicator->style()->polish(m_connectionIndicator);
}

void MainWindow::onArduinoError(QString msg) { setStatus("Arduino: " + msg, "#FF5252"); }

void MainWindow::onConnectPortClicked() {
    QString port = m_portCombo->currentText();
    if (port.isEmpty()) { setStatus("No port selected.", "#FF5252"); return; }
    m_arduino->connectToPort(port);
}

void MainWindow::refreshPorts() {
    m_portCombo->clear();
    for (const auto& p : ArduinoManager::availablePorts())
        m_portCombo->addItem(p);
}

// ─────────────────────────────────────────────────────────────
//  Update binary text + LED circles
// ─────────────────────────────────────────────────────────────
void MainWindow::setBinaryDisplay(QLabel* label, QLabel** leds,
                                  int value, const QString& onColor)
{
    QString bin = QString("%1").arg(value, 8, 2, QChar('0'));
    label->setText(bin.left(4) + "  " + bin.right(4));

    for (int i = 0; i < 8; i++) {
        bool bitOn = (value >> (7 - i)) & 1;
        if (bitOn) {
            // LED مضيء — glow effect بـ box-shadow
            leds[i]->setStyleSheet(
                QString("background: %1;"
                        "border-radius: 12px;"
                        "border: 1.5px solid %1;").arg(onColor));
        } else {
            // LED مطفي — لون موحد لكل الصفوف
            leds[i]->setStyleSheet(
                QString("background: #1A1F2E;"
                        "border-radius: 12px;"
                        "border: 1.5px solid #2A3040;"));
        }
    }
}

// ─────────────────────────────────────────────────────────────
void MainWindow::setStatus(const QString& msg, const QString& color) {
    m_statusLabel->setText(msg);
    m_statusLabel->setStyleSheet("color:" + color + "; font-size:12px;");
}

// ─────────────────────────────────────────────────────────────
//  STYLESHEET
// ─────────────────────────────────────────────────────────────
QString MainWindow::appStyleSheet() {
    return R"(
        QMainWindow, QWidget {
            background: #0D1117;
            color: #C9D1D9;
            font-family: "Segoe UI", "Helvetica Neue", sans-serif;
            font-size: 13px;
        }

        #header { background: #0D1117; border-bottom: 1px solid #21262D; }
        #headerIcon  { font-size: 32px; color: #64FFDA; }
        #headerTitle { font-size: 18px; font-weight: 600; color: #E6EDF3; letter-spacing: 0.5px; }
        #headerSub   { font-size: 11px; color: #8B949E; letter-spacing: 2px; }
        #statusLabel { font-size: 12px; color: #64FFDA; }

        #connBar { background: #0D1117; border-bottom: 1px solid #21262D; }
        #connIndicatorOn  { color: #69F0AE; font-weight: 600; font-size: 12px; }
        #connIndicatorOff { color: #FF5252; font-weight: 600; font-size: 12px; }
        #dimLabel { color: #8B949E; }

        #tabBar { background: #0D1117; border-bottom: 1px solid #21262D; }
        #tabBtn {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 6px;
            color: #8B949E;
            padding: 0 16px;
            font-size: 13px;
        }
        #tabBtn:checked {
            background: #1C2128;
            border-color: #30363D;
            color: #64FFDA;
            font-weight: 600;
        }
        #tabBtn:hover { color: #E6EDF3; }

        /* ── Cards ── */
        #card {
            background: #161B22;
            border: 1px solid #21262D;
            border-radius: 10px;
        }
        #resultCard {
            background: #0F1923;
            border: 1px solid #1C3A4A;
            border-radius: 10px;
            min-height: 100px;
        }
        #cardTitle {
            font-size: 11px;
            color: #8B949E;
            letter-spacing: 1.5px;
            text-transform: uppercase;
            margin-bottom: 4px;
        }

        /* ── LED rows — موحدة بدون خلفيات مختلفة ── */
        #ledRow {
            background: transparent;
            border-radius: 8px;
        }
        #ledRow:hover {
            background: #1C2128;
        }

        /* ── Inputs ── */
        #inputLabel { color: #8B949E; font-size: 11px; margin-bottom: 2px; }
        #numInput, #portCombo, #opCombo {
            background: #0D1117;
            border: 1px solid #30363D;
            border-radius: 6px;
            color: #E6EDF3;
            padding: 6px 10px;
            font-size: 14px;
        }
        #numInput:focus { border-color: #64FFDA; }
        #opCombo QAbstractItemView {
            background: #161B22;
            border: 1px solid #30363D;
            color: #E6EDF3;
        }

        /* ── Buttons ── */
        #primaryBtn {
            background: #1C3A4A;
            border: 1px solid #64FFDA;
            border-radius: 8px;
            color: #64FFDA;
            font-size: 14px;
            font-weight: 600;
        }
        #primaryBtn:hover   { background: #1E4A5E; }
        #primaryBtn:pressed { background: #163040; }
        #primaryBtn:disabled { opacity: 0.4; }
        #smallBtn {
            background: #21262D;
            border: 1px solid #30363D;
            border-radius: 6px;
            color: #C9D1D9;
            padding: 4px 12px;
        }
        #smallBtn:hover { background: #30363D; }
        #iconBtn {
            background: #21262D;
            border: 1px solid #30363D;
            border-radius: 6px;
            color: #8B949E;
            font-size: 14px;
        }
        #iconBtn:hover { color: #64FFDA; }

        /* ── Binary display ── */
        #binValue {
            font-family: "Courier New", monospace;
            font-size: 16px;
            font-weight: 700;
            letter-spacing: 3px;
        }

        /* ── Result ── */
        #equationLabel { font-size: 15px; color: #8B949E; }
        #bigResult     { font-size: 52px; font-weight: 700; color: #64FFDA; }
        #solvedBy      { font-size: 12px; color: #448AFF; letter-spacing: 0.5px; }

        /* ── AI page ── */
        #cameraView {
            background: #0D1117;
            border: 1px solid #21262D;
            border-radius: 8px;
            color: #30363D;
            font-size: 14px;
        }
        #aiStatus        { color: #C9D1D9; font-size: 13px; line-height: 1.5; }
        #aiEquationLabel { font-size: 20px; font-weight: 700; color: #E6EDF3; }

        /* ── Footer ── */
        #footer      { background: #0D1117; border-top: 1px solid #21262D; }
        #footerLabel { color: #484F58; font-size: 11px; }

        /* ── Scrollbar ── */
        QScrollBar:vertical { width: 6px; background: #0D1117; }
        QScrollBar::handle:vertical { background: #30363D; border-radius: 3px; }

        QWidget::item:selected { background: transparent; color: #C9D1D9; }
        QLabel { background: transparent; }
    )";
}