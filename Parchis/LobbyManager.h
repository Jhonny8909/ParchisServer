#pragma once
#include <SFML/Network.hpp>
#include <map>
#include <string>

struct Lobby {
    std::string code;
    std::vector<sf::TcpSocket*> players;
    bool gameStarted = false;
};

class LobbyManager {
public:
    bool createLobby(const std::string& code, sf::TcpSocket* host);
    bool joinLobby(const std::string& code, sf::TcpSocket* player);
    void startGame(const std::string& code);

private:
    std::map<std::string, Lobby> lobbies;
    static const int MAX_PLAYERS = 2;
};