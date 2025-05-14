#include "PacketHandler.h"

void PacketHandler::registerHandler(const std::string& packetType, HandlerFunction handler) {
    handlers[packetType] = handler;
}

void PacketHandler::handlePacket(sf::TcpSocket* client, sf::Packet& packet) {
    std::string packetType;
    if (!(packet >> packetType)) {
        std::cerr << "Error: No se pudo leer el tipo de paquete" << std::endl;
        return;
    }

    auto it = handlers.find(packetType);
    if (it != handlers.end()) {
        it->second(client, packet);
    }
    else {
        std::cerr << "Error: Handler no encontrado para tipo '" << packetType << "'" << std::endl;

        // Respuesta de error estándar
        sf::Packet errorPacket;
        errorPacket << "ERROR" << ("Tipo de paquete no soportado: " + packetType);
        client->send(errorPacket);
    }
}