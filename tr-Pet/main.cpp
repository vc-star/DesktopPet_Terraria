#include "trPet.h"
#include <QtWidgets/QApplication>
#include "BasePet.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    trPet window;
    window.show();

    // 在程序的入口启动肝疼小曲
    BasePet::playGlobalMusic("tr-pet_material/Music-Overworld_Day.wav");
    return app.exec();
}
