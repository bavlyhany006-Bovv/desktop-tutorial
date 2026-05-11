// ============================================================
//  AIManager.h
//  Handles all Gemini API communication for handwriting OCR
//  and complex equation solving.
// ============================================================
#pragma once
#include <QObject>
#include <QString>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// ─────────────────────────────────────────────────────────────
struct ParsedEquation {
    bool    success;
    QString rawText;
    int     operandA;
    int     operandB;
    bool    isSubtraction;
    bool    isSimple8Bit;
    double  aiSolvedAnswer;
    QString explanation;
    QString errorMsg;
};

// ─────────────────────────────────────────────────────────────
class AIManager : public QObject {
    Q_OBJECT
public:
    explicit AIManager(const QString& apiKey, QObject* parent = nullptr);

    void analyseImage(const QImage& image);
    void setApiKey(const QString& key);

signals:
    void analysisComplete(ParsedEquation result);
    void processingStarted();
    void errorOccurred(QString message);

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    QString                m_apiKey;
    QNetworkAccessManager* m_network;

    QString        imageToBase64(const QImage& image);
    QByteArray     buildRequestBody(const QString& base64Image);
    ParsedEquation parseGeminiResponse(const QString& jsonText);
    bool           isHardwareCapable(int a, int b, bool subtract);
};