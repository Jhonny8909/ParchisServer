#pragma once
#include <SFML/Network.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

struct Lobby {
    std::string code;
    sf::TcpSocket* host; // Solo el host mantiene conexión con el servidor
    std::vector<std::string> playerIPs; // IPs de los jugadores (incluyendo host)
    bool gameStarted = false;
};

class LobbyManager {
public:
    bool createLobby(const std::string& code, sf::TcpSocket* host, const std::string& hostIP);
    bool joinLobby(const std::string& code, const std::string& playerIP);
    void startGame(const std::string& code);
    const std::vector<std::string>& getLobbyPlayers(const std::string& code) const;

private:
    std::unordered_map<std::string, Lobby> lobbies;
    static const int MAX_PLAYERS = 4;
};