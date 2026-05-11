// ============================================================
//  CameraManager.h
//  Wraps OpenCV camera capture into a Qt-friendly class.
// ============================================================
#pragma once

#include <QObject>
#include <QImage>
#include <QTimer>

// Forward-declare cv::VideoCapture to avoid including OpenCV in the header
namespace cv { class VideoCapture; }

// ─────────────────────────────────────────────────────────────
//  CameraManager
//  Responsibilities:
//    • Open / close the laptop webcam via OpenCV
//    • Emit a live frame every ~33ms (≈30 fps) for the preview
//    • Capture a snapshot on demand
// ─────────────────────────────────────────────────────────────
class CameraManager : public QObject {
    Q_OBJECT

public:
    explicit CameraManager(QObject* parent = nullptr);
    ~CameraManager();

    // Start the camera preview
    bool startCamera(int deviceIndex = 0);

    // Stop the camera and release the resource
    void stopCamera();

    // Returns true if camera is currently open
    bool isOpen() const;

    // Grab and return the current frame as a QImage
    QImage captureFrame();

signals:
    // Emitted ~30 times per second while the camera is open
    void frameReady(QImage frame);

    // Emitted when the camera fails to open
    void cameraError(QString message);

private slots:
    void onTimerTick();   // Grabs frames from OpenCV at regular intervals

private:
    cv::VideoCapture* m_capture;   // OpenCV camera handle
    QTimer*           m_timer;     // Frame-rate timer

    // Convert an OpenCV BGR Mat to a Qt QImage
    QImage matToQImage(void* mat);   // void* to avoid cv::Mat in header
};
