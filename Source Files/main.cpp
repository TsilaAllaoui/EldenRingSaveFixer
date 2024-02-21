#include <iostream>

#include <savefile.h>
#include <version.h>

#include <QApplication>
#include <erSaveFixer.h>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    ERSaveFixer fixer;
    fixer.show();

    app.exec();

    return 0;
}
