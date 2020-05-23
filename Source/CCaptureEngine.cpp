#include "CCaptureEngine.h"

#include <QTimer>
#include <QApplication>
#include <QScreen>
#include <QPixmap>
#include <QMutex>
#include <QMutexLocker>

#include "Util/util.h"


CCaptureEngine::CCaptureEngine(int tDelayMs, QObject* parent) :
    QObject(parent),
    mTDelayMs(tDelayMs),
    mpTrigger(nullptr),
    mpCapture(nullptr),
    mpScrnPtr(nullptr),
    mpWnd(nullptr),
    mpLock(nullptr)
{
    mpCapture = new QPixmap();
    mpLock = new QMutex();
}


CCaptureEngine::~CCaptureEngine()
{
    stopCapture();
    DEL_PTR_(mpTrigger);
    DEL_PTR_(mpCapture);
    DEL_PTR_(mpLock);
}


bool CCaptureEngine::startCapture()
{
    if (!mpTrigger)
    {   // Must created in the thread that starts it.
        mpTrigger = new QTimer();
        connect(mpTrigger,
            &QTimer::timeout,
            [this](void) {this->captureWindow(mpWnd); }
        );
    }
    if (mpTrigger)
    {  // can start without valid window handle
        if (mpTrigger->isActive())
            mpTrigger->setInterval(mTDelayMs);
        else
            mpTrigger->start(mTDelayMs);
        return true;
    }
    return false;
}


void CCaptureEngine::stopCapture()
{
    if (!mpTrigger)
    {
        mpTrigger->stop();
    }
}


bool CCaptureEngine::captureWindow(const void* hwnd)
{
    if (!hwnd)
        return false;

    if (!mpScrnPtr)
        mpScrnPtr = QGuiApplication::primaryScreen();

    try
    {
        if (mpLock->tryLock(mpTrigger->interval()>>1))
        {
            BENCHMARK_(2, mpScrnPtr->grabWindow(WId(hwnd)).swap(*mpCapture));
            mpLock->unlock();
            return true;
        }
    } catch (const std::exception& e) {
        qWarning() << "Frame dropped:" << e.what();
        mpScrnPtr = nullptr;
    }

    return false;
}


bool  CCaptureEngine::tryGetImage(QImage* pOutImg)
{
    if (pOutImg)
    {
        QMutexLocker guard(mpLock);
        if (mpCapture && !mpCapture->isNull())
        {
            // This is a deep copy moved to rOutImg
            mpCapture->toImage().swap(*pOutImg);
            return !pOutImg->isNull();
        }
    }
    return false;
}


bool CCaptureEngine::getIsRunning()
{
    if (mpTrigger)
        return mpTrigger->isActive();
    else
        return false;
}


const QPixmap* CCaptureEngine::getCapture()
{
    // gives no guard for pointer access after call,
    // only assures the ptr is not written during cpy
    QMutexLocker guard(mpLock);
    return mpCapture;
}