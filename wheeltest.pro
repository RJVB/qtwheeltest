TEMPLATE = app
SOURCES += wheeltest.cpp
greaterThan(QT_MAJOR_VERSION, 4) {
    TARGET=wheeltest-qt5
    QT += widgets
} else {
    TARGET=wheeltest-qt4
}
CONFIG += debug c++11
