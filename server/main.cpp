#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    Server server;
    if (!server.startServer(12345)) {
        return EXIT_FAILURE;
    }

    return a.exec();
}
