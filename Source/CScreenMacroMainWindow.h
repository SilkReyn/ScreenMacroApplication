#pragma once

class QMouseEvent;
class QRubberBand;
class CScreenMacroTools;
class QPixmap;
class QPoint;

#include <QtWidgets/QMainWindow>



namespace Ui {
class ScreenMacroForm;

class CScreenMacroMainWindow : public QMainWindow
{
    Q_OBJECT

    ScreenMacroForm* mpUi;
    CScreenMacroTools* mpTools;
    QRubberBand* mpSelectionRect;
    QPoint* mpMseClickPos;
    //int mWndWidth;

    void addPattern();
    void updateWindowNames();
    void updateCapture();
    void toggleMacro();
    //void showCapture(const QPixmap& rInCapt);


protected:
    virtual void mouseReleaseEvent(QMouseEvent* pEvent) override;
    virtual void mousePressEvent(QMouseEvent* pEvent) override;
    virtual void mouseMoveEvent(QMouseEvent* pEvent) override;


public:
    CScreenMacroMainWindow();
    ~CScreenMacroMainWindow();


public slots:
    void onCmbWindows_currentIndexChanged(int idx);
    void onBtnFindPattern_pressed();
    //void onUpdateCapture(const QPixmap& rInCpt);
    
};

}// namespace Ui
