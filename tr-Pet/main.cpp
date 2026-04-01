#include "trPet.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    trPet window;
    window.show();
    return app.exec();
}
