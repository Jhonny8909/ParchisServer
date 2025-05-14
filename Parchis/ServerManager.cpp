#include "ServerManager.h"

ServerManager::ServerManager(unsigned short port) : running(false) {
    if (listener.listen(port) != sf::Socket::Status::Done) {
        throw std::runtime_error("No se pudo iniciar en puerto " + std::to_string(port));
    }
}

void ServerManager::start() {
    running = true;
    acceptThread = std::thread(&ServerManager::acceptConnections, this);
}

void ServerManager::stop() {
    running = false;
    listener.close();

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& client : clients) {
            client->stop();
        }
        clients.clear();
    }

    if (acceptThread.joinable()) acceptThread.join();
}

void ServerManager::acceptConnections() {
    while (running) {
        auto socket = std::make_unique<sf::TcpSocket>();
        if (listener.accept(*socket) == sf::Socket::Status::Done) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.emplace_back(std::make_unique<ClientHandler>(std::move(socket), packetHandler));
            clients.back()->start();
        }

        // Limpieza cada 100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cleanupClients();
    }
}

void ServerManager::cleanupClients() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.erase(
        std::remove_if(clients.begin(), clients.end(),
            [](const auto& client) { return !client->isRunning(); }),
        clients.end()
    );
}

ServerManager::~ServerManager() {
    stop(); // Asegura que todo se limpie correctamente
}

PacketHandler& ServerManager::getPacketHandler() {
    return packetHandler;
}