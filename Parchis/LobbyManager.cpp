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

    // 1. Primero notificar a los PEERs que escuchen
    for (size_t i = 1; i < lobby.playerIPs.size(); ++i) {
        sf::TcpSocket notifier;
        auto peerAddress = sf::IpAddress::resolve(lobby.playerIPs[i]);
        if (peerAddress && notifier.connect(*peerAddress, 53000, sf::seconds(2)) == sf::Socket::Status::Done) {
            sf::Packet preparePacket;
            preparePacket << "PREPARE_P2P";
            notifier.send(preparePacket);
            std::cout << "[P2P] Peer " << lobby.playerIPs[i] << " preparado" << std::endl;
        }
    }

    // 2. Esperar 1 segundo para que los peers preparen sus listeners
    sf::sleep(sf::seconds(1));

    // 3. Ahora notificar al HOST para que inicie conexiones
    sf::Packet hostPacket;
    hostPacket << "GAME_START" << "HOST";
    for (size_t i = 1; i < lobby.playerIPs.size(); ++i) {
        hostPacket << lobby.playerIPs[i];
    }
    lobby.host->send(hostPacket);

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