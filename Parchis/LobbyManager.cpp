#include "LobbyManager.h"

bool LobbyManager::createLobby(const std::string& code, sf::TcpSocket* host, const std::string& hostIP) {
    if (lobbies.count(code) > 0) return false;

    Lobby newLobby;
    newLobby.code = code;
    newLobby.host = host;
    newLobby.playerIPs.push_back(hostIP); // Host es el primer jugador
    lobbies[code] = newLobby;

    return true;
}

bool LobbyManager::joinLobby(const std::string& code, const std::string& playerIP) {
    auto it = lobbies.find(code);
    if (it == lobbies.end() || it->second.gameStarted) return false;
    if (it->second.playerIPs.size() >= MAX_PLAYERS) return false;

    it->second.playerIPs.push_back(playerIP);
    return true;
}

void LobbyManager::startGame(const std::string& code) {
    auto& lobby = lobbies[code];
    lobby.gameStarted = true;

    // Notificar al host
    sf::Packet hostPacket;
    hostPacket << "GAME_START" << "HOST";
    for (size_t i = 1; i < lobby.playerIPs.size(); ++i) {
        hostPacket << lobby.playerIPs[i];
    }

    // Versión corregida para SFML 3.0.0
    if (lobby.host->send(hostPacket) != sf::Socket::Status::Done) {
        std::cerr << "[ERROR] Failed to notify host" << std::endl;
    }

    // Notificar a los peers
    for (size_t i = 1; i < lobby.playerIPs.size(); ++i) {
        sf::TcpSocket tempSocket;
        auto peerAddress = sf::IpAddress::resolve(lobby.playerIPs[i]);

        if (!peerAddress) {
            std::cerr << "[ERROR] Could not resolve peer IP: " << lobby.playerIPs[i] << std::endl;
            continue;
        }

        // Conexión con timeout explícito
        if (tempSocket.connect(*peerAddress, 53000, sf::seconds(3)) != sf::Socket::Status::Done) {
            std::cerr << "[ERROR] Failed to connect to peer: " << lobby.playerIPs[i] << std::endl;
            continue;
        }

        sf::Packet peerPacket;
        peerPacket << "GAME_START" << "PEER";
        for (size_t j = i + 1; j < lobby.playerIPs.size(); ++j) {
            peerPacket << lobby.playerIPs[j];
        }

        // Versión corregida para SFML 3.0.0
        if (tempSocket.send(peerPacket) != sf::Socket::Status::Done) {
            std::cerr << "[ERROR] Failed to send to peer: " << lobby.playerIPs[i] << std::endl;
        }
    }

    lobbies.erase(code);
}
const std::vector<std::string>& LobbyManager::getLobbyPlayers(const std::string& code) const {
    static std::vector<std::string> emptyVector; // Para retornar algo seguro si no se encuentra el lobby

    auto it = lobbies.find(code);
    if (it != lobbies.end()) {
        return it->second.playerIPs;
    }

    return emptyVector; // Retorna vector vacío si el lobby no existe
}