#pragma once
#include <SFML/Network.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <mutex>

#define LISTENER_PORT 55000
#define MAX_PLAYERS_PER_LOBBY 4

enum class PlayerColor {
    RED = 0,
    GREEN = 1,
    YELLOW = 2,
    BLUE = 3,
    NONE = -1
};

std::string colorToString(PlayerColor color) {
    switch (color) {
    case PlayerColor::RED: return "Rojo";
    case PlayerColor::GREEN: return "Verde";
    case PlayerColor::YELLOW: return "Amarillo";
    case PlayerColor::BLUE: return "Azul";
    default: return "Ninguno";
    }
}

struct Lobby {
    std::string name;
    std::string hostAddressStr;
    unsigned short hostPort;
    unsigned short maxPlayers;
    unsigned short currentPlayers;
};

std::map<std::string, Lobby> lobbies;

PlayerColor asignarColor(unsigned short orden) {
    switch (orden) {
    case 1: return PlayerColor::RED;
    case 2: return PlayerColor::GREEN;
    case 3: return PlayerColor::YELLOW;
    case 4: return PlayerColor::BLUE;
    default: return PlayerColor::NONE;
    }
}

void manejarCliente(sf::TcpSocket* client) {
    sf::Packet packet;
    std::string accion;
    std::string codigoSala;
    sf::Packet respuesta;


    while (true) {
        std::cout << "Esperando datos del cliente..." << std::endl;
        sf::Socket::Status status = client->receive(packet);
        if (status == sf::Socket::Status::Disconnected) {
            std::cout << "Cliente desconectado\n";
            break;
        }
        else if (status != sf::Socket::Status::Done) {
            std::cerr << "Error al recibir paquete\n";
            break;
        }

        std::cout << "Recibido paquete con " << packet.getDataSize() << " bytes" << std::endl;

        // Limpiar valores previos
        accion.clear();
        codigoSala.clear();
		respuesta.clear();

        // Extraer acción y código de sala
        if (!(packet >> accion >> codigoSala)) {
            std::cerr << "Paquete mal formado\n";
            break;
        }

        std::cout << "Accion: " << accion << " | Codigo sala: " << codigoSala << std::endl;


        //CREAR SALA
        if (accion == "CREAR") {
            if (lobbies.find(codigoSala) != lobbies.end()) {
                std::cerr << "Error: ya existe un lobby con ese codigo.\n";
                respuesta << "ERROR: Sala ya existente";
            }
            else {
                Lobby nuevo;
                nuevo.name = codigoSala;
                nuevo.hostAddressStr = client->getRemoteAddress().value().toString();
                nuevo.hostPort = LISTENER_PORT;
                nuevo.maxPlayers = 4;
                nuevo.currentPlayers = 1;
                lobbies[codigoSala] = nuevo;

                PlayerColor color = asignarColor(nuevo.currentPlayers);
                std::cout << "Lobby creado con codigo: " << codigoSala << std::endl;
                std::cout << "Jugadores actuales: " << nuevo.currentPlayers << "/" << nuevo.maxPlayers << std::endl;
				std::cout << "Color asignado: " << colorToString(color) << std::endl;
                respuesta << "Lobby Creado" << codigoSala;
            }
        }

        //UNIRSE A SALA
        else if (accion == "UNIRSE") {
            auto it = lobbies.find(codigoSala);
            if (it == lobbies.end()) {
                std::cerr << "Error: ese lobby no existe.\n";
            }
            else if (it->second.currentPlayers >= it->second.maxPlayers) {
                std::cerr << "Error: el lobby está lleno.\n";
            }
            else {
                it->second.currentPlayers++;
                PlayerColor color = asignarColor(it->second.currentPlayers);
                std::cout << "Cliente unido al lobby: " << codigoSala << std::endl;
                std::cout << "Jugadores actuales: " << it->second.currentPlayers << "/" << it->second.maxPlayers << std::endl;
				std::cout << "Color asignado: " << colorToString(color) << std::endl;
                respuesta << "UNIDO" << codigoSala;
            }
        }
		packet.clear(); // Limpia para poder recibir otro paquete

        if (client->send(respuesta) != sf::Socket::Status::Done) {
            std::cerr << "Error al enviar respuesta al cliente." << std::endl;
        }
    }

    client->disconnect();
    delete client;
}