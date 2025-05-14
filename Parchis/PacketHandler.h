#pragma once
#include <SFML/Network.hpp>
#include <functional>
#include <unordered_map>
#include <iostream>

class PacketHandler {
public:
    using HandlerFunction = std::function<void(sf::TcpSocket*, sf::Packet&)>;

    void registerHandler(const std::string& packetType, HandlerFunction handler);
    void handlePacket(sf::TcpSocket* client, sf::Packet& packet);

private:
    std::unordered_map<std::string, HandlerFunction> handlers;
};