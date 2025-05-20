#include "LobbyManager.h"
#include <iostream>

bool LobbyManager::createLobby(const std::string& code, sf::TcpSocket* host) {
    if (lobbies.count(code) > 0) {
        std::cout << "[SERVIDOR] Intento fallido de crear lobby - Código ya existe: " << code << std::endl;
        return false;
    }

    Lobby newLobby;
    newLobby.code = code;
    newLobby.players.push_back(host);
    lobbies[code] = newLobby;

    // Mostrar en consola del servidor
    std::cout << "=====================================\n";
    std::cout << "[SERVIDOR] NUEVO LOBBY CREADO:\n";
    std::cout << "Código: " << code << "\n";
    std::cout << "Host: " << host->getRemoteAddress().value() << ":" << host->getRemotePort() << "\n";
    std::cout << "Jugadores conectados: 1/" << MAX_PLAYERS << "\n";
    std::cout << "=====================================\n";

    return true;
}

bool LobbyManager::joinLobby(const std::string& code, sf::TcpSocket* player) {
    auto it = lobbies.find(code);
    if (it == lobbies.end()) {
        std::cout << "[SERVIDOR] Intento fallido de unirse - Lobby no existe: " << code << std::endl;
        return false;
    }

    if (it->second.gameStarted) {
        std::cout << "[SERVIDOR] Intento fallido de unirse - Partida ya comenzó: " << code << std::endl;
        return false;
    }

    if (it->second.players.size() >= MAX_PLAYERS) {
        std::cout << "[SERVIDOR] Intento fallido de unirse - Lobby lleno: " << code
            << " (" << it->second.players.size() << "/" << MAX_PLAYERS << ")" << std::endl;
        return false;
    }

    it->second.players.push_back(player);

    // Mostrar actualización en consola
    std::cout << "=====================================\n";
    std::cout << "[SERVIDOR] JUGADOR UNIDO AL LOBBY:\n";
    std::cout << "Código: " << code << "\n";
    std::cout << "Nuevo jugador: " << player->getRemoteAddress().value() << ":" << player->getRemotePort() << "\n";
    std::cout << "Total jugadores: " << it->second.players.size() << "/" << MAX_PLAYERS << "\n";
    std::cout << "=====================================\n";

    // Notificar a todos los jugadores del lobby
    sf::Packet updatePacket;
    updatePacket << "LOBBY_UPDATE" << static_cast<int>(it->second.players.size());

    for (auto* p : it->second.players) {
        p->send(updatePacket);
    }

    // Iniciar juego si se alcanzó el máximo de jugadores (1 en este caso)
    if (it->second.players.size() == MAX_PLAYERS) {
        startGame(code);
    }

    return true;
}


void LobbyManager::startGame(const std::string& code) {
    auto& lobby = lobbies[code];
    lobby.gameStarted = true;

    // Notificar a todos los jugadores
    sf::Packet startPacket;
    startPacket << "GAME_START";

    std::cout << "[SERVIDOR] Enviando GAME_START para lobby: " << code << std::endl;

    for (auto* player : lobby.players) {
        if (player->send(startPacket) != sf::Socket::Status::Done) {
            std::cerr << "[SERVIDOR] Error al enviar GAME_START a jugador" << std::endl;
        }
    }
}