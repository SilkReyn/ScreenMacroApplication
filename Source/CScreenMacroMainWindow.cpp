#include "CScreenMacroMainWindow.h"
#include "ui_ScreenMacroForm.h"
#include "CScreenMacroTools.h"

#include <QString>
#include <QVector>
#include <QMouseEvent>
#include <QRubberBand>
#include <QRect>
#include <QPoint>

#include "Util/util.h"


using namespace Ui;


CScreenMacroMainWindow::CScreenMacroMainWindow() :
    mpUi(nullptr),
    mpSelectionRect(nullptr),
    mpTools(nullptr),
    mpMseClickPos(nullptr)
{
    mpUi = new Ui::ScreenMacroForm();
    mpUi->setupUi(this);
    mpSelectionRect = new QRubberBand(QRubberBand::Rectangle, mpUi->lblCaptureView);
    mpTools = new CScreenMacroTools();
    mpMseClickPos = new QPoint();

    // This is a QMainWindow, therefore it has a instance of QObject
    // We call connect of our QObject

    /*this->connect(
        mpUi->cmbWindows,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int idx) { this->mpTools->setTargetWindow(idx); }
    );*/
    this->connect(
        mpUi->btnStartCapture,
        &QPushButton::pressed,
        this,
        &CScreenMacroMainWindow::toggleMacro
    );
    this->connect(
        mpUi->btnRefreshWindows,
        &QPushButton::pressed,
        this,
        &CScreenMacroMainWindow::updateWindowNames
    );
    this->connect(
        mpUi->btnMakeScreenshot,
        &QPushButton::pressed,
        this,
        &CScreenMacroMainWindow::updateCapture
    );
    this->connect(
        mpUi->btnStorePattern,
        &QPushButton::pressed,
        this,
        &CScreenMacroMainWindow::addPattern
    );
    this->connect(
        mpUi->btnFindPattern,
        &QPushButton::pressed,
        this,
        &CScreenMacroMainWindow::onBtnFindPattern_pressed
    );

    updateWindowNames();
}


CScreenMacroMainWindow::~CScreenMacroMainWindow()
{
    DEL_PTR_(mpUi);
    DEL_PTR_(mpSelectionRect);
    DEL_PTR_(mpTools);
    DEL_PTR_(mpMseClickPos);
}


void CScreenMacroMainWindow::updateCapture()
{
    QLabel* pLbl = mpUi->lblCaptureView;
    pLbl->move(0, 0);
    pLbl->setPixmap(mpTools->getWndCapture());
    pLbl->adjustSize();
    //mWndWidth = pLbl->width();
    mpUi->scrollAreaWidgetContents->adjustSize();
}


void CScreenMacroMainWindow::updateWindowNames()
{
    QVector<QString> wndNams;
    mpTools->getWindowNames(&wndNams);
    mpUi->cmbWindows->clear();
    QString name;
    foreach (name, wndNams)
    {
        mpUi->cmbWindows->addItem(name);
    }
    
    if (mpUi->cmbWindows->count() > 0)
    {
        mpUi->cmbWindows->setCurrentIndex(0);
    }
}


void CScreenMacroMainWindow::toggleMacro()
{
    static bool running;
    if (running)
    {
        mpTools->stop();
        mpUi->btnStartCapture->setText("start");
        running = false;
    } else {
        mpTools->start(mpUi->cmbWindows->currentIndex());
        mpUi->btnStartCapture->setText("stop");
        running = true;
    }
}


//debug
void CScreenMacroMainWindow::addPattern()
{
    const QPixmap* pPattern = nullptr;
    if (pPattern= mpUi->lblCaptureView->pixmap())
    {
        mpUi->lblPattern->setPixmap(*pPattern);
        int w = mpUi->scrollAreaWidgetContents->width();
        if (w < 1)  // div zero guard
            w = 1;
        mpTools->setPattern(
            "test",  // remember to care about uppercase and whitespace
            *pPattern,
            (float)(pPattern->width()) / w
        );
    }
}

#pragma region Events

void CScreenMacroMainWindow::mousePressEvent(QMouseEvent* pEvent)
{
    if (!pEvent)
        return;

    if (mpUi->lblCaptureView->underMouse() &&
        pEvent->button() == Qt::RightButton)
    {
        *mpMseClickPos = mpUi->lblCaptureView->mapFrom(this, pEvent->pos());
        mpSelectionRect->setGeometry(QRect(*mpMseClickPos, QSize()));
        mpSelectionRect->show();
    }
}


void CScreenMacroMainWindow::mouseMoveEvent(QMouseEvent* pEvent)
{
    if (!pEvent)
        return;

    if (mpSelectionRect->isVisible() && mpUi->lblCaptureView->underMouse())
    {
        QPoint pt = mpUi->lblCaptureView->mapFrom(
            this, pEvent->pos()
        );
        mpSelectionRect->setGeometry(
            QRect(
                qMin(mpMseClickPos->x(), pt.x()),
                qMin(mpMseClickPos->y(), pt.y()),
                qAbs(mpMseClickPos->x() - pt.x()),
                qAbs(mpMseClickPos->y() - pt.y())
            )
        );
    }
}


// Overrides QWidget event handler.
void CScreenMacroMainWindow::mouseReleaseEvent(QMouseEvent* pEvent)
{
    if (!pEvent)
        return;

    if (mpUi->lblCaptureView->underMouse())
    {
        if (pEvent->button() == Qt::LeftButton)
        {// Can run in another thread because it blocks UI for the click duration
            mpTools->simulateClickAt(
                mpUi->cmbWindows->currentIndex(),
                mpUi->lblCaptureView->mapFrom(this, pEvent->pos())
            );
        }
        if (pEvent->button() == Qt::RightButton && mpSelectionRect->isVisible()) {
            QLabel* pLbl = mpUi->lblCaptureView;
            QRect rec = mpSelectionRect->geometry();
            pLbl->setPixmap(pLbl->pixmap()->copy(rec));
            pLbl->setGeometry(rec);
            mpSelectionRect->hide();
        }
    }
}


void CScreenMacroMainWindow::onCmbWindows_currentIndexChanged(int idx)
{
    mpTools->setTargetWindow(idx);
}

void CScreenMacroMainWindow::onBtnFindPattern_pressed()
{
    CScreenMacroTools::scaling_t method;
    switch (mpUi->cmbScalingMethod->currentIndex())
    {
        case 1:
            method = CScreenMacroTools::SCL_WINDOW;
            break;
        case 2:
            method = CScreenMacroTools::SCL_ZOOM;
            break;
        default:
            method = CScreenMacroTools::SCL_OFF;
            break;
    }
    // TODO: create seperate task with qfuture
    QPoint loc;
    if (mpTools->windowHasPattern("test", method, &loc))
    {
        mpTools->simulateClickAt(mpUi->cmbWindows->currentIndex(), loc);
    }
}

#pragma endregion
