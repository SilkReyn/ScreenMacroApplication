#pragma once

template <typename T> class QVector;
template <typename T1, typename T2> class QMap;
class QThread;
class QSize;
class CCaptureEngine;


#include <QPixmap>
#include <QString>
#include <QImage>


struct patch_t {
    QImage img;
    float fillPerc;
};


class CScreenMacroTools
{
    QMap<QString, patch_t>* mpPatterns;
    CCaptureEngine* mpGrabber;
    QThread* mpCaptureLoop;
    QVector<const void*>* mpHandles;
    //QImage mFrame;

    const void* getMappedHdl(int idx);

    void startCapture(const void* wndPtr=nullptr);
    void stopCapture();
    void createCaptureTask();
    void killCaptureTask();

protected:
    

public:
    enum scaling_t{ SCL_OFF, SCL_WINDOW, SCL_ZOOM };

    CScreenMacroTools();
    ~CScreenMacroTools();

    QPixmap getWndCapture();
    void getWindowNames(QVector<QString>* pOutNames);

    bool setTargetWindow(int idx);
    void setPattern(QString patternKey, const QPixmap& rInPattern, float fillFact);

    void start(int idx) { startCapture(getMappedHdl(idx)); }  // temporary
    void stop() { stopCapture(); }  // temporary
    void simulateClickAt(int wndIdx, const QPoint& rInWndPos=QPoint(0,0));
    bool windowHasPattern(QString patternKey, scaling_t scaling, QPoint* pOutPos=nullptr);
};