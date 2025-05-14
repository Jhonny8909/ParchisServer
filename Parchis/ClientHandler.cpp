#include "ClientHandler.h"

ClientHandler::ClientHandler(std::unique_ptr<sf::TcpSocket> socket, PacketHandler& handler)
    : socket(std::move(socket)), packetHandler(handler), running(false) {
    this->socket->setBlocking(true);
}

void ClientHandler::start() {
    if (running) return;
    running = true;
    thread = std::thread(&ClientHandler::run, this);
}

void ClientHandler::stop() {
    running = false;
    if (thread.joinable()) thread.join();
}

void ClientHandler::run() {
    while (running) {
        sf::Packet packet;
        auto status = socket->receive(packet);

        if (status == sf::Socket::Status::Disconnected) break;
        if (status != sf::Socket::Status::Done) continue;

        packetHandler.handlePacket(socket.get(), packet);
    }
    running = false;
}

ClientHandler::~ClientHandler() {
    stop(); // Limpia los recursos
}

bool ClientHandler::isRunning() const {
    return running;
}