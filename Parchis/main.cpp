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

        auto& db = DatabaseManager::getInstance();
        sql::Connection* con = db.getConnection();
        bool success = isLogin
            ? AuthService::loginUser(con, username, password)
            : AuthService::registerUser(con, username, password);
        std::string message = success ? "Autenticación exitosa" : "Credenciales inválidas";
        db.releaseConnection(con);

        sf::Packet response;
        response << "AUTH_RESPONSE" << success << message; // Ahora envía 3 valores
        std::cerr << "Enviando respuesta: " << success << " - " << message << std::endl;

        if (client->send(response) != sf::Socket::Status::Done) {
            std::cerr << "Error al enviar respuesta" << std::endl;
        }
        });

    // Handler de lobby
    handler.registerHandler("LOBBY", [&lobbyManager](sf::TcpSocket* client, sf::Packet& packet) {
        std::string action, code;
        if (!(packet >> action >> code)) {
            std::cerr << "Paquete LOBBY mal formado" << std::endl;
            return;
        }

        sf::Packet response;
        std::string status;

        if (action == "CREAR") {
            bool created = lobbyManager.createLobby(code, client);
            status = created ? "CREATED" : "EXISTS";
            response << "LOBBY_RESPONSE" << status << code;
        }
        else if (action == "UNIRSE") {
            bool joined = lobbyManager.joinLobby(code, client);
            status = joined ? "JOINED" : "FULL_OR_INVALID";
            response << "LOBBY_RESPONSE" << status << code;
        }
        else {
            status = "INVALID_ACTION";
            response << "LOBBY_RESPONSE" << status << "";
        }

        if (client->send(response) != sf::Socket::Status::Done) {
            std::cerr << "Error al enviar respuesta LOBBY" << std::endl;
        }
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