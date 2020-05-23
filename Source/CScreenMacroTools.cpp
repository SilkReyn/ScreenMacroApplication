#include "CScreenMacroTools.h"
#include "Util/winapi.h"  // adds map, wstring
#include "CCaptureEngine.h"
#include "Util/imgproc.h"

#include <QGuiApplication>
#include <QObject>
#include <QThread>
#include <QtConcurrent>
#include <QMap>
#include <QPoint>
#include <qwindowdefs.h>  // WId

#include "Util/util.h"


CScreenMacroTools::CScreenMacroTools() :
    mpHandles(nullptr),
    mpGrabber(nullptr),
    mpCaptureLoop(nullptr),
    mpPatterns(nullptr)
{
    mpHandles = new QVector<const void*>();
    mpPatterns = new QMap<QString, patch_t>();

    createCaptureTask();
}


CScreenMacroTools::~CScreenMacroTools()
{
    killCaptureTask();  // handles capture loop & grabber

    DEL_PTR_(mpHandles);
    DEL_PTR_(mpPatterns);
}


#pragma region Windows

bool CScreenMacroTools::setTargetWindow(int idx)
{
    if (idx >= 0
        && mpHandles->count() > idx
        && mpGrabber
        && WinOS::checkIsValidWindow(mpHandles->at(idx)))
    {
        mpGrabber->onSetWindow(mpHandles->at(idx));  // [FIXME] asynchronous!
        return true;
    } else
        return false;
}


const void* CScreenMacroTools::getMappedHdl(int idx)
{
    return (idx >= 0 && mpHandles->count() > idx) ?
        mpHandles->at(idx) : nullptr;
}


void CScreenMacroTools::getWindowNames(QVector<QString>* pOutNames)
{
    if (!pOutNames)
        return;

    std::map<const void*, std::wstring> HandleMap;
    WinOS::listDsktParentWindows(&HandleMap);
    if (!HandleMap.size())
        return;

    mpHandles->clear();
    for (auto pair : std::as_const(HandleMap)) // it = HandleMap.cbegin(); it != HandleMap.cend(); it++)
    {
        if (!(pair.second.empty()))
        {
            mpHandles->append(pair.first);
            pOutNames->append(QString::fromStdWString(pair.second));
        }
    }
}


void CScreenMacroTools::simulateClickAt(int wndIdx, const QPoint& rInWndPos)
{
    QtConcurrent::run(
        WinOS::clickWindowHere,
        getMappedHdl(wndIdx),
        rInWndPos.x(),
        rInWndPos.y(),
        true,  // return to initial cursor pos
        0uL    // unspecified click duration
    );
}

# pragma endregion


#pragma region Capture


void CScreenMacroTools::createCaptureTask()
{
    if (!mpCaptureLoop)
        mpCaptureLoop = new QThread();

    if (!mpGrabber)
        mpGrabber = new CCaptureEngine(1000);  // 1 fps sampling

    QObject::connect(
        mpCaptureLoop, &QThread::finished, mpCaptureLoop, &QObject::deleteLater
    );
    QObject::connect(
        mpCaptureLoop, &QThread::started, mpGrabber, &CCaptureEngine::startCapture
    );

    mpGrabber->moveToThread(mpCaptureLoop);
}


void CScreenMacroTools::killCaptureTask()
{
    if (mpCaptureLoop)
    {
        if (mpCaptureLoop->isRunning())
        {
            mpCaptureLoop->quit();
            mpCaptureLoop->wait();  // blocks till thread finishes
        }
        delete mpCaptureLoop;
        mpCaptureLoop = nullptr;

        //do not delete mpGrabber (handled by thread)
        mpGrabber = nullptr;
    }
}


QPixmap CScreenMacroTools::getWndCapture()
{
    const QPixmap* pPix = nullptr;
    if (mpGrabber)
        pPix = mpGrabber->getCapture();

    // Returns a copy referencing same data
    // Origin gets detached on write
    if (pPix)
        return *pPix;
    else
        return QPixmap();
}


void CScreenMacroTools::startCapture(const void* wndPtr)
{
    if (!mpCaptureLoop || !mpCaptureLoop->isRunning())
    {
        createCaptureTask();
        mpCaptureLoop->start();
    }

    if (mpGrabber && wndPtr)
        mpGrabber->onSetWindow(wndPtr);
}


void CScreenMacroTools::stopCapture()
{
    if (mpCaptureLoop && mpCaptureLoop->isRunning())
        killCaptureTask();
    // this leaves a instance of qthread if it wasnt running before
}


void CScreenMacroTools::setPattern(QString patternKey, const QPixmap& rInPattern, float fillFact)
{
    // adds new or overwrites existing key
    // format should be RGB32
    if (!patternKey.isEmpty() && !rInPattern.isNull())
        mpPatterns->insert(patternKey, patch_t{ rInPattern.toImage(), fillFact });  // creates qimage
}

#pragma endregion

#pragma region imgproc

bool CScreenMacroTools::windowHasPattern(QString patternKey, scaling_t scaling, QPoint* pOutPos)
{
    QImage frame;

    if (!mpGrabber || !mpGrabber->tryGetImage(&frame))
        return false;

    auto* patch = &(mpPatterns->value(patternKey));
    if (frame.isNull()
        || !patch
        || patch->img.isNull())
    {
        return false;
    }
    if (frame.format() != QImage::Format_RGB32)
    {
        frame.convertToFormat(QImage::Format_RGB32).swap(frame);
        qWarning("Unsupported screenshot color format, converted to RGB32!");
    }
    float wF = patch->fillPerc * frame.width();
    if ((~0u>>1) < wF)            // Does not fit into int
    {
        qWarning("(Pattern) Scaling too big, request dropped.");
        return false;             //throw std::range_error;
    }
    int wI = patch->img.width();
    QImage pattern;
    if ((scaling == SCL_WINDOW)
        && (wF > 16)
        && (wF * ImProcU8::IMG_MATCH_SZ_TOL < abs(wF - wI)))
    {// Rescale pattern to fit its parent origin.
     // Repeated rescales may gradually reduce image quality!
        wI = wF;
        patch->img.scaledToWidth(wI).swap(pattern);
    } else
        pattern = patch->img;  // Shallow copy

    if (pattern.format() != QImage::Format_RGB32)
        pattern.convertToFormat(QImage::Format_RGB32).swap(pattern);

    ImProcU8::imgArr2I_t frmSz = {
        frame.width(),
        frame.height()
    };
    ImProcU8::imgArr2I_t patSz = {
        wI,
        pattern.height()
    };
    ImProcU8::imgArr2I_t location = { 0, 0 };
    int chan = 4;  // rgb qimage always has 4 channels (32bit align)
    if (scaling == SCL_ZOOM)
    {
        frame.convertToFormat(QImage::Format_Grayscale8).swap(frame);
        pattern.convertToFormat(QImage::Format_Grayscale8).swap(pattern);
        chan = 1;
    }
    ImProcU8::Image parent{
        frame.constBits(),
        frame.bytesPerLine(),
        chan,
        frmSz
    };
    ImProcU8::Image target{
        pattern.constBits(),
        pattern.bytesPerLine(),
        chan,
        patSz
    };
    if ((scaling == SCL_ZOOM) ?
        ImProcU8::locateFeaturesIn(parent,target,location) : 
        ImProcU8::locatePatternIn(parent,target,location))
    {
        if (pOutPos)
        {
            pOutPos->setX(location[ImProcU8::COOR_LEFT]);
            pOutPos->setY(location[ImProcU8::COOR_TOP]);
        }
        return true;
    } else {
        return false;
    }
}
#pragma endregion
