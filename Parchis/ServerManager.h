#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include "ClientHandler.h"
#include "PacketHandler.h"

class ServerManager {
public:
    ServerManager(unsigned short port);
    ~ServerManager();

    void start();
    void stop();
    PacketHandler& getPacketHandler();

private:
    void acceptConnections();
    void cleanupClients();

    sf::TcpListener listener;
    std::vector<std::unique_ptr<ClientHandler>> clients;
    PacketHandler packetHandler;
    bool running;
    std::thread acceptThread;
    std::mutex clientsMutex;
};