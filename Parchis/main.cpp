#include "ServerManager.h"
#include "DatabaseManager.h"
#include "AuthService.h"
#include "LobbyManager.h"
#include <iostream>

void setupHandlers(PacketHandler& handler, LobbyManager& lobbyManager) {
    // Handler de autenticación
    handler.registerHandler("AUTH", [](sf::TcpSocket* client, sf::Packet& packet) {
        bool isLogin;
        std::string username, password;
        if (!(packet >> isLogin >> username >> password)) {
            std::cerr << "Paquete AUTH mal formado" << std::endl;
            return;
        }
        std::cerr << isLogin << username << password << std::endl;

        auto& db = DatabaseManager::getInstance();
        sql::Connection* con = db.getConnection();
        bool success = isLogin
            ? AuthService::loginUser(con, username, password)
            : AuthService::registerUser(con, username, password);
        db.releaseConnection(con);

        sf::Packet response;
        response << "AUTH_RESPONSE" << success;
        client->send(response);
        });

    // Handler de lobby
    handler.registerHandler("LOBBY", [&lobbyManager](sf::TcpSocket* client, sf::Packet& packet) {
        std::string action, code, playerIP;
        if (!(packet >> action >> code >> playerIP)) {
            std::cerr << "Paquete LOBBY mal formado" << std::endl;
            return;
        }

        std::cout << "\n[LOBBY_DEBUG] Solicitud recibida - Acción: " << action
            << ", Código: " << code << ", IP: " << playerIP << std::endl;

        sf::Packet response;

        if (action == "CREAR") {
            bool created = lobbyManager.createLobby(code, client, playerIP);
            response << "LOBBY_RESPONSE" << (created ? "CREATED" : "EXISTS") << code;
            std::cout << "[LOBBY_DEBUG] Lobby " << code << (created ? " creado" : " ya existe") << "\n";
        }
        else if (action == "UNIRSE") {
            bool joined = lobbyManager.joinLobby(code, playerIP);
            response << "LOBBY_RESPONSE" << (joined ? "JOINED" : "FULL_OR_INVALID") << code;
            std::cout << "[LOBBY_DEBUG] Intento de unión a " << code << ": " << (joined ? "éxito" : "fallo") << "\n";

            if (joined) {
                auto& players = lobbyManager.getLobbyPlayers(code);
                std::cout << "[LOBBY_DEBUG] Jugadores actuales en " << code << " (" << players.size() << "):\n";
                for (size_t i = 0; i < players.size(); ++i) {
                    std::cout << " - " << players[i] << (i == 0 ? " (HOST)" : "") << std::endl;
                }

                if (players.size() >= 2) {
                    std::cout << "[LOBBY_DEBUG] Iniciando partida...\n";
                    lobbyManager.startGame(code);
                }
            }
        }

        if (client->send(response) != sf::Socket::Status::Done) {
            std::cerr << "[ERROR] Error enviando respuesta de lobby\n";
        }
        else {
            std::cout << "[LOBBY_DEBUG] Respuesta enviada al cliente\n";
        }
        });

    handler.registerHandler("PREPARE_P2P", [](sf::TcpSocket* client, sf::Packet& packet) {
        std::cout << "[P2P] Preparando conexión P2P..." << std::endl;
        });
}

int main() {
    try {
        DatabaseManager::getInstance().testConnection();
        LobbyManager lobbyManager;

        ServerManager server(55000);
        setupHandlers(server.getPacketHandler(), lobbyManager);

        server.start();
        std::cout << "Servidor iniciado. Presiona Enter para detener..." << std::endl;
        std::cin.get();

        server.stop();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}