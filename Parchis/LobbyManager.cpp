#include "LobbyManager.h"

bool LobbyManager::createLobby(const std::string& code, sf::TcpSocket* host) {
    if (lobbies.count(code) > 0) return false;

    Lobby newLobby;
    newLobby.code = code;
    newLobby.players.push_back(host);
    lobbies[code] = newLobby;

    return true;
}

bool LobbyManager::joinLobby(const std::string& code, sf::TcpSocket* player) {
    auto it = lobbies.find(code);
    if (it == lobbies.end() || it->second.gameStarted) return false;

    if (it->second.players.size() >= MAX_PLAYERS) return false;

    it->second.players.push_back(player);
    return true;
}

void LobbyManager::startGame(const std::string& code) {
    auto& lobby = lobbies[code];
    lobby.gameStarted = true;

    // Notificar a todos los jugadores
    sf::Packet startPacket;
    startPacket << "GAME_START";

    for (auto* player : lobby.players) {
        player->send(startPacket);
    }
}