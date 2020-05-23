#pragma once

class QTimer;
class QScreen;
class QPixmap;
class QMutex;


#include <QObject>


class CCaptureEngine : public QObject
{
    Q_OBJECT

    QTimer* mpTrigger;
    QPixmap* mpCapture;
    QScreen* mpScrnPtr;
    QMutex* mpLock;
    const void* mpWnd;
    const int mTDelayMs;

    bool captureWindow(const void* hwnd);

public:
    CCaptureEngine(int tDelayMs, QObject* parent = 0);
    ~CCaptureEngine();

    bool tryGetImage(QImage* pOutImg);
    const QPixmap* getCapture();
    bool getIsRunning();

    bool startCapture();
    void stopCapture();

public slots:
    // this is only thread save with queued signal connection
    void onSetWindow(const void* wndPtr) { mpWnd = wndPtr; }
};