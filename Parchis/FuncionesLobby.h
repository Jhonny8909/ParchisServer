/*#pragma once
#include <SFML/Network.hpp>
#include <iostream>
#include <map>
#include <string>
#include "Lobby.h"

#define LISTENER_PORT 55000

std::map<std::string, sf::IpAddress> salas;

int Raaah() {
    sf::TcpListener listener;
    if (listener.listen(LISTENER_PORT) != sf::Socket::Status::Done) {
        std::cerr << "Error al escuchar en el puerto " << LISTENER_PORT << std::endl;
        return -1;
    }

    std::cout << "Servidor bootstrap escuchando en el puerto " << LISTENER_PORT << std::endl;

    while (true) {
        sf::TcpSocket client;
        if (listener.accept(client) != sf::Socket::Status::Done) {
            std::cerr << "Error al aceptar conexión" << std::endl;
            continue;
        }

        std::cout << "Cliente conectado desde: " << client.getRemoteAddress().value() << std::endl;

        std::string codigo = RecibirCodigo(client);

        if (codigo.empty()) continue;

        if (salas.find(codigo) != salas.end()) {
            // Ya existe
            std::cerr << "La sala '" << codigo << "' ya existe. Rechazando creación." << std::endl;
            sf::Packet response;
            response << "SALA_EXISTENTE";
            client.send(response);
        }
        else {
            // Crear sala
            salas[codigo] == client.getRemoteAddress();
            sf::Packet response;
            response << "SALA_CREADA";
            client.send(response);

            std::cout << "Sala creada con código: " << codigo << ". " << "Cliente es ahora el host." << std::endl;
        }
    }

    return 0;
}*/
