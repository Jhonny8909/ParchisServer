#pragma once
#include <SFML/Network.hpp>
#include <memory>
#include <thread>
#include "PacketHandler.h"

class ClientHandler {
public:
    ClientHandler(std::unique_ptr<sf::TcpSocket> socket, PacketHandler& packetHandler);
    ~ClientHandler();

    void start();
    void stop();
    bool isRunning() const;

private:
    void run();

    std::unique_ptr<sf::TcpSocket> socket;
    PacketHandler& packetHandler;
    std::thread thread;
    bool running;
};