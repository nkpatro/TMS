#include "httpserver/server.h"
#include <QDebug>

namespace Http {

    Server::Server(QObject* parent)
        : QObject(parent)
    {
    }

    Server::~Server() = default;

    void Server::registerController(std::shared_ptr<Controller> controller) {
        if (controller) {
            controller->setupRoutes(server);
            controllers.push_back(controller);
        }
    }

    bool Server::start(quint16 port, const QHostAddress& address) {
        if (!tcpServer.listen(address, port)) {
            qDebug() << "TCP Server failed to start!";
            return false;
        }

        server.bind(&tcpServer);
        qDebug() << "Server is running on" << address.toString() << "port" << tcpServer.serverPort();
        return true;
    }

    void Server::stop() {
        if (tcpServer.isListening()) {
            tcpServer.close();
            qDebug() << "Server stopped";
        }
    }

    bool Server::isRunning() const {
        return tcpServer.isListening();
    }

    quint16 Server::port() const {
        return tcpServer.isListening() ? tcpServer.serverPort() : 0;
    }

    QHostAddress Server::address() const {
        return tcpServer.isListening() ? tcpServer.serverAddress() : QHostAddress::Any;
    }

} // namespace Http