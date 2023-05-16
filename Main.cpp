#include <QtWidgets/QApplication>

#include "./MiniThing/Qt/MiniThingQt.h"
#include "./MiniThing/Core/MiniThingCore.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MiniThingQt miniThingQt;
    miniThingQt.show();

    return app.exec();
}
