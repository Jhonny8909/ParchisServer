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
        std::string action, code;
        if (!(packet >> action >> code)) {
            std::cerr << "Paquete LOBBY mal formado" << std::endl;
            return;
        }

		std::cerr<< "accion: " << action << "codigo: " << code << std::endl;

        sf::Packet response;
        if (action == "CREAR") {
            bool created = lobbyManager.createLobby(code, client);
            response << "LOBBY_RESPONSE" << (created ? "CREATED" : "EXISTS") << code;
        }
        else if (action == "UNIRSE") {
            bool joined = lobbyManager.joinLobby(code, client);
            response << "LOBBY_RESPONSE" << (joined ? "JOINED" : "FULL_OR_INVALID");
        }
        client->send(response);
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