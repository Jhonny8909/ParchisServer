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

    handler.registerHandler("GAME", [&lobbyManager](sf::TcpSocket* client, sf::Packet& packet) {
        std::string lobbyCode;
        if (!(packet >> lobbyCode)) {
            std::cerr << "Paquete GAME mal formado" << std::endl;
            return;
        }

        // Pasar el paquete completo incluyendo el tipo
        lobbyManager.manejarPaqueteJuego(client, packet, lobbyCode);
        });

    handler.registerHandler("CONSULTA_TURNO", [&lobbyManager](sf::TcpSocket* client, sf::Packet& packet) {
        std::string lobbyCode;
        if (packet >> lobbyCode) {
            try {
                const Lobby& lobby = lobbyManager.getLobby(lobbyCode);
                bool esSuTurno = false;

                // Buscar qué jugador está preguntando
                for (size_t i = 0; i < lobby.partida.jugadores.size(); ++i) {
                    if (lobby.partida.jugadores[i].socket == client) {
                        esSuTurno = (lobby.partida.jugadorActual == static_cast<int>(i));
                        break;
                    }
                }

                sf::Packet respuesta;
                respuesta << "RESPUESTA_TURNO" << esSuTurno;
                client->send(respuesta);

            }
            catch (const std::exception& e) {
                std::cerr << "Error en CONSULTA_TURNO: " << e.what() << std::endl;
            }
        }
        else {
            std::cerr << "Paquete CONSULTA_TURNO mal formado" << std::endl;
        }
        });

    handler.registerHandler("TIRAR_DADO", [&lobbyManager](sf::TcpSocket* client, sf::Packet& packet) {
        std::string lobbyCode;
        ColorJugador color;
        if (!(packet >> lobbyCode >> color)) {
            std::cerr << "Paquete TIRAR_DADO mal formado" << std::endl;
            return;
        }

        int valorDado = 1 + (std::rand() % 6);

        sf::Packet respuesta;
        respuesta << "DADO_RESULTADO" << valorDado << color;
        lobbyManager.broadcastToLobby(lobbyCode, respuesta);
        });

    handler.registerHandler("FIN_TURNO", [&lobbyManager](sf::TcpSocket* client, sf::Packet& packet) {
        std::string lobbyCode;
        ColorJugador color;
        if (packet >> lobbyCode >> color) {
            try {
                Lobby& lobby = lobbyManager.getLobby(lobbyCode);

                // Verificar que el que termina es efectivamente el jugador actual
                if (lobby.partida.jugadores[lobby.partida.jugadorActual].color == color) {
                    lobbyManager.cambiarTurno(lobbyCode);
                }
                else {
                    std::cerr << "[SERVER] Intento ilegítimo de finalizar turno" << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error en FIN_TURNO: " << e.what() << std::endl;
            }
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