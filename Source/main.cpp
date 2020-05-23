#include <QtWidgets/QApplication>
#include "CScreenMacroMainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Ui::CScreenMacroMainWindow window;
    window.show();
    return app.exec();
}