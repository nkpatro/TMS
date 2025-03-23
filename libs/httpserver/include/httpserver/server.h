#pragma once
#include <QObject>
#include <QHttpServer>
#include <QTcpServer>
#include <QHostAddress>
#include <memory>
#include <vector>
#include "controller.h"

namespace Http {

    class Server : public QObject {
        Q_OBJECT
    public:
        explicit Server(QObject* parent = nullptr);
        ~Server() override;

        void registerController(std::shared_ptr<Controller> controller);
        bool start(quint16 port = 8080, const QHostAddress& address = QHostAddress::Any);
        void stop();
        bool isRunning() const;
        quint16 port() const;
        QHostAddress address() const;

    private:
        QHttpServer server;
        QTcpServer tcpServer;
        std::vector<std::shared_ptr<Controller>> controllers;
    };

} // namespace Http