// ============================================================
//  AIManager.cpp
//  Gemini Vision API integration for handwriting recognition.
// ============================================================
#include <QUrlQuery>
#include "AIManager.h"
#include <QBuffer>
#include <QByteArray>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>

// Gemini model endpoint
static const QString GEMINI_URL =
    "https://generativelanguage.googleapis.com/v1beta/models/"
    "gemini-2.5-flash:generateContent";

// ─────────────────────────────────────────────────────────────
AIManager::AIManager(const QString& apiKey, QObject* parent)
    : QObject(parent), m_apiKey(apiKey)
{
    m_network = new QNetworkAccessManager(this);
    connect(m_network, &QNetworkAccessManager::finished,
            this,      &AIManager::onNetworkReply);
}

// ─────────────────────────────────────────────────────────────
void AIManager::setApiKey(const QString& key) {
    m_apiKey = key;
}

// ─────────────────────────────────────────────────────────────
void AIManager::analyseImage(const QImage& image) {
    if (m_apiKey.isEmpty()) {
        emit errorOccurred("Gemini API key is not set.");
        return;
    }

    emit processingStarted();

    QString base64 = imageToBase64(image);
    QByteArray body = buildRequestBody(base64);

    QUrl url(GEMINI_URL);
    QUrlQuery query;
    query.addQueryItem("key", m_apiKey);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    qDebug() << "[AI] Sending image to Gemini...";
    m_network->post(request, body);
}

// ─────────────────────────────────────────────────────────────
void AIManager::onNetworkReply(QNetworkReply* reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Network error: " + reply->errorString());
        return;
    }

    QString jsonText = QString::fromUtf8(reply->readAll());
    qDebug() << "[AI] Received response, parsing...";

    ParsedEquation result = parseGeminiResponse(jsonText);
    emit analysisComplete(result);
}

// ─────────────────────────────────────────────────────────────
QString AIManager::imageToBase64(const QImage& image) {
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 90);
    return bytes.toBase64();
}

// ─────────────────────────────────────────────────────────────
QByteArray AIManager::buildRequestBody(const QString& base64Image) {
    QString prompt = R"(
You are an expert at reading handwritten math equations.
Analyse the image and return ONLY a JSON object (no extra text, no markdown).

JSON format:
{
  "recognized_equation": "<the equation as written>",
  "operand_a": <integer or null>,
  "operand_b": <integer or null>,
  "operation": "add" | "subtract" | "complex",
  "direct_answer": <number (for complex equations) or null>,
  "explanation": "<one sentence describing what you found>"
}

Rules:
- If the equation is a simple addition or subtraction of two integers fill operand_a, operand_b, operation.
- If the equation involves multiplication, division, exponents, roots, or numbers > 255 set operation to "complex" and fill direct_answer.
- Never output anything outside the JSON object.
)";

    QJsonObject textPart;
    textPart["text"] = prompt;

    QJsonObject inlineData;
    inlineData["mime_type"] = "image/jpeg";
    inlineData["data"]      = base64Image;

    QJsonObject imagePart;
    imagePart["inline_data"] = inlineData;

    QJsonArray parts;
    parts.append(textPart);
    parts.append(imagePart);

    QJsonObject contentObj;
    contentObj["parts"] = parts;

    QJsonArray contents;
    contents.append(contentObj);

    QJsonObject body;
    body["contents"] = contents;

    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

// ─────────────────────────────────────────────────────────────
ParsedEquation AIManager::parseGeminiResponse(const QString& jsonText) {
    ParsedEquation result;
    result.success = false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &err);

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        result.errorMsg = "Invalid JSON from Gemini: " + err.errorString();
        return result;
    }

    QJsonObject root = doc.object();

    QJsonArray candidates = root["candidates"].toArray();
    if (candidates.isEmpty()) {
        result.errorMsg = "No candidates in Gemini response.";
        return result;
    }

    QString aiText = candidates[0].toObject()
                         ["content"].toObject()
                                 ["parts"].toArray()[0].toObject()
                                 ["text"].toString().trimmed();

    qDebug() << "[AI] Raw model text:" << aiText;

    aiText.remove(QRegularExpression("```json|```"));
    aiText = aiText.trimmed();

    QJsonDocument inner = QJsonDocument::fromJson(aiText.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !inner.isObject()) {
        result.errorMsg = "Model returned non-JSON text: " + aiText.left(200);
        return result;
    }

    QJsonObject data = inner.object();

    result.rawText     = data["recognized_equation"].toString();
    result.explanation = data["explanation"].toString();
    QString op         = data["operation"].toString();

    if (op == "add" || op == "subtract") {
        result.isSubtraction = (op == "subtract");
        result.operandA      = data["operand_a"].toInt();
        result.operandB      = data["operand_b"].toInt();
        result.isSimple8Bit  = isHardwareCapable(result.operandA,
                                                result.operandB,
                                                result.isSubtraction);
        if (!result.isSimple8Bit) {
            result.aiSolvedAnswer = result.isSubtraction
                                        ? (result.operandA - result.operandB)
                                        : (result.operandA + result.operandB);
        }
    } else {
        result.isSimple8Bit   = false;
        result.aiSolvedAnswer = data["direct_answer"].toDouble();
    }

    result.success = true;
    return result;
}

// ─────────────────────────────────────────────────────────────
bool AIManager::isHardwareCapable(int a, int b, bool subtract) {
    if (a < 0 || a > 255 || b < 0 || b > 255) return false;
    if (subtract) return (a - b) >= 0;
    else          return (a + b) <= 255;
}