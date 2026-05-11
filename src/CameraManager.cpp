// ============================================================
//  CameraManager.cpp
//  OpenCV camera capture implementation.
// ============================================================
#include "CameraManager.h"
#include <opencv2/opencv.hpp>
#include <QDebug>

static const int FRAME_INTERVAL_MS = 33;   // ≈30 fps

// ─────────────────────────────────────────────────────────────
CameraManager::CameraManager(QObject* parent)
    : QObject(parent), m_capture(nullptr)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CameraManager::onTimerTick);
}

// ─────────────────────────────────────────────────────────────
CameraManager::~CameraManager() {
    stopCamera();
}

// ─────────────────────────────────────────────────────────────
bool CameraManager::startCamera(int deviceIndex) {
    if (m_capture && m_capture->isOpened()) {
        return true;   // Already open
    }

    m_capture = new cv::VideoCapture(deviceIndex);

    if (!m_capture->isOpened()) {
        emit cameraError("Cannot open camera (device " +
                         QString::number(deviceIndex) + ").");
        delete m_capture;
        m_capture = nullptr;
        return false;
    }

    // Set a comfortable preview resolution
    m_capture->set(cv::CAP_PROP_FRAME_WIDTH,  640);
    m_capture->set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    m_timer->start(FRAME_INTERVAL_MS);
    qDebug() << "[Camera] Started on device" << deviceIndex;
    return true;
}

// ─────────────────────────────────────────────────────────────
void CameraManager::stopCamera() {
    m_timer->stop();
    if (m_capture) {
        m_capture->release();
        delete m_capture;
        m_capture = nullptr;
    }
    qDebug() << "[Camera] Stopped.";
}

// ─────────────────────────────────────────────────────────────
bool CameraManager::isOpen() const {
    return m_capture && m_capture->isOpened();
}

// ─────────────────────────────────────────────────────────────
//  Grab the current frame and return it as a QImage
// ─────────────────────────────────────────────────────────────
QImage CameraManager::captureFrame() {
    if (!isOpen()) return QImage();

    cv::Mat frame;
    *m_capture >> frame;

    if (frame.empty()) return QImage();
    return matToQImage(&frame);
}

// ─────────────────────────────────────────────────────────────
//  Timer tick: grab frame and emit it for the live preview
// ─────────────────────────────────────────────────────────────
void CameraManager::onTimerTick() {
    QImage img = captureFrame();
    if (!img.isNull()) {
        emit frameReady(img);
    }
}

// ─────────────────────────────────────────────────────────────
//  Convert cv::Mat (BGR) → QImage (RGB)
//  The void* trick avoids exposing cv::Mat in the header so
//  files that include CameraManager.h don't need OpenCV headers.
// ─────────────────────────────────────────────────────────────
QImage CameraManager::matToQImage(void* rawMat) {
    cv::Mat& mat = *static_cast<cv::Mat*>(rawMat);

    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

    return QImage(rgb.data,
                  rgb.cols, rgb.rows,
                  static_cast<int>(rgb.step),
                  QImage::Format_RGB888).copy();   // .copy() because Mat is temporary
}
