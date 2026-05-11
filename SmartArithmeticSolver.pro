# ============================================================
#  Smart 8-Bit Arithmetic Hardware Solver
#  Qt Project File (.pro)
# ============================================================

QT += core gui widgets serialport network multimedia multimediawidgets

TARGET   = SmartArithmeticSolver
TEMPLATE = app

CONFIG += c++17

# ── Source Files ─────────────────────────────────────────────
SOURCES += \
    src/main.cpp           \
    src/MainWindow.cpp     \
    src/ArduinoManager.cpp \
    src/AIManager.cpp      \
    src/CameraManager.cpp

HEADERS += \
    src/MainWindow.h     \
    src/ArduinoManager.h \
    src/AIManager.h      \
    src/CameraManager.h

# ── OpenCV via vcpkg (Windows MinGW) ─────────────────────────
win32 {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
}

# ── OpenCV on Linux / macOS ───────────────────────────────────
unix {
    CONFIG  += link_pkgconfig
    PKGCONFIG += opencv4
}