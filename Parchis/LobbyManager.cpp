#include "LobbyManager.h"
#include <iostream>

bool LobbyManager::createLobby(const std::string& code, sf::TcpSocket* host) {
    if (lobbies.count(code) > 0) {
        std::cout << "[SERVIDOR] Intento fallido de crear lobby - Código ya existe: " << code << std::endl;
        return false;
    }

    Lobby newLobby;
    newLobby.code = code;
    newLobby.players[0] = host;

    // Asignar color ROJO al host
    newLobby.partida.jugadores[0].socket = host;
    newLobby.partida.jugadores[0].color = ColorJugador::ROJO;
    newLobby.coloresDisponibles.pop_back(); // Eliminar ROJO de disponibles

    lobbies[code] = newLobby;

    std::cout << "=====================================\n";
    std::cout << "[SERVIDOR] NUEVO LOBBY CREADO:\n";
    std::cout << "Código: " << code << "\n";
    std::cout << "Host: " << host->getRemoteAddress().value() << ":" << host->getRemotePort() << "\n";
    std::cout << "Color asignado: ROJO\n";
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

    // Buscar primer slot vacío
    size_t slot = 0;
    for (; slot < it->second.players.size(); ++slot) {
        if (it->second.players[slot] == nullptr) break;
    }

    if (slot >= it->second.players.size()) {
        std::cout << "[SERVIDOR] Intento fallido de unirse - Lobby lleno: " << code << std::endl;
        return false;
    }

    it->second.players[slot] = player;

    // Asignar color según orden de llegada
    ColorJugador color = it->second.asignarColor(slot); // Pasamos el slot como parámetro
    it->second.partida.jugadores[slot].socket = player;
    it->second.partida.jugadores[slot].color = color;

    std::string colorStr;
    switch (color) {
    case ColorJugador::ROJO: colorStr = "ROJO"; break;
    case ColorJugador::AMARILLO: colorStr = "AMARILLO"; break;
    case ColorJugador::VERDE: colorStr = "VERDE"; break;
    case ColorJugador::AZUL: colorStr = "AZUL"; break;
    default: colorStr = "NINGUNO";
    }

    std::cout << "=====================================\n";
    std::cout << "[SERVIDOR] JUGADOR UNIDO AL LOBBY:\n";
    std::cout << "Código: " << code << "\n";
    std::cout << "Nuevo jugador: " << player->getRemoteAddress().value() << ":" << player->getRemotePort() << "\n";
    std::cout << "Color asignado: " << colorStr << "\n";
    std::cout << "Total jugadores: " << (slot + 1) << "/" << MAX_PLAYERS_LOBBY << "\n";
    std::cout << "=====================================\n";

    // Notificar a todos los jugadores del lobby (incluyendo el nuevo color)
    sf::Packet updatePacket;
    updatePacket << "LOBBY_UPDATE" << static_cast<int>(slot + 1) << static_cast<int>(color);

    for (auto* p : it->second.players) {
        if (p) p->send(updatePacket);
    }

    // Iniciar juego si se alcanzó el máximo de jugadores
    if ((slot + 1) == MAX_PLAYERS_LOBBY) {
        startGame(code);
    }

    return true;
}


void LobbyManager::startGame(const std::string& code) {
    auto& lobby = lobbies[code];
    lobby.gameStarted = true;

    for (size_t i = 0; i < lobby.players.size(); ++i) {
        if (lobby.players[i]) {
            // Verificación adicional antes de enviar
            if (lobby.partida.jugadores[i].color == ColorJugador::NINGUNO) {
                std::cerr << "ERROR: Jugador " << i << " no tiene color asignado" << std::endl;
                continue;
            }

            sf::Packet startPacket;
            startPacket << "GAME_START"
                << static_cast<int>(lobby.partida.jugadores[i].color)
                << code;

            lobby.players[i]->send(startPacket);
        }
    }
}

// Función helper para convertir color a string
std::string colorToString(ColorJugador color) {
    switch (color) {
    case ColorJugador::ROJO: return "ROJO";
    case ColorJugador::AMARILLO: return "AMARILLO";
    case ColorJugador::VERDE: return "VERDE";
    case ColorJugador::AZUL: return "AZUL";
    default: return "NINGUNO";
    }
}


void LobbyManager::manejarPaqueteJuego(sf::TcpSocket* client, sf::Packet& packet, const std::string& lobbyCode) {
    auto it = lobbies.find(lobbyCode);
    if (it == lobbies.end()) return;

    auto& lobby = it->second;
    auto& partida = lobby.partida;

    // Determinar qué jugador está enviando el paquete
    int jugadorId = -1;
    for (size_t i = 0; i < lobby.players.size(); ++i) {
        if (lobby.players[i] == client) {
            jugadorId = i;
            break;
        }
    }

    if (jugadorId == -1) return;

    std::string tipoPaquete;
    if (!(packet >> tipoPaquete)) return;

    if (tipoPaquete == "TIRAR_DADO" && partida.estado == EstadoJuego::ESPERANDO_TIRADA && jugadorId == partida.jugadorActual) {
        // Simular lanzamiento de dado
        partida.dadoValue = rand() % 6 + 1;
        partida.estado = EstadoJuego::DADO_LANZADO;

        // Enviar resultado a todos
        sf::Packet dadoPacket;
        dadoPacket << "DADO_RESULTADO" << partida.dadoValue;
        for (auto& jugador : partida.jugadores) {
            if (jugador.socket) jugador.socket->send(dadoPacket);
        }
    }
    else if (tipoPaquete == "SELECCION_FICHA" && partida.estado == EstadoJuego::DADO_LANZADO && jugadorId == partida.jugadorActual) {
        int fichaId;
        if (!(packet >> fichaId)) return;

        // Validar que la ficha pertenece al jugador y puede moverse
        if (fichaId >= 0 && fichaId < 4) {
            partida.fichaSeleccionada = jugadorId * 4 + fichaId;
            partida.estado = EstadoJuego::MOVIENDO_FICHA;

            // Mover la ficha
            int nuevaPosicion = partida.posicionesFichas[partida.fichaSeleccionada] + partida.dadoValue;
            partida.posicionesFichas[partida.fichaSeleccionada] = nuevaPosicion;

            // Notificar a todos
            sf::Packet movimientoPacket;
            movimientoPacket << "MOVIMIENTO_FICHA" << jugadorId << fichaId << nuevaPosicion;
            for (auto& jugador : partida.jugadores) {
                if (jugador.socket) jugador.socket->send(movimientoPacket);
            }

            // Cambiar turno
            cambiarTurno(lobbyCode);
        }
    }

    enviarEstadoPartida(lobbyCode);
}

void LobbyManager::cambiarTurno(const std::string& lobbyCode) {
    auto& partida = lobbies[lobbyCode].partida;

    // Lógica para cambiar de turno (como antes)
    int intentos = 0;
    do {
        partida.jugadorActual = (partida.jugadorActual + 1) % MAX_PLAYERS_LOBBY;
        intentos++;
    } while (!partida.jugadores[partida.jugadorActual].socket && intentos < MAX_PLAYERS_LOBBY);

    // Resetear estado del juego
    partida.estado = EstadoJuego::ESPERANDO_TIRADA;
    partida.dadoValue = 0;
    partida.fichaSeleccionada = -1;

    // Usar el nuevo sistema de notificación
    notificarCambioTurno(lobbyCode);

    // Opcional: enviar estado completo del juego
    enviarEstadoPartida(lobbyCode);
}

void LobbyManager::enviarEstadoPartida(const std::string& lobbyCode) {
    auto& partida = lobbies[lobbyCode].partida;

    sf::Packet estadoPacket;
    estadoPacket << "ESTADO_PARTIDA"
        << partida.jugadorActual
        << static_cast<int>(partida.estado)
        << partida.dadoValue;

    // Añadir posiciones de todas las fichas
    for (auto pos : partida.posicionesFichas) {
        estadoPacket << pos;
    }

    for (auto& jugador : partida.jugadores) {
        if (jugador.socket) jugador.socket->send(estadoPacket);
    }
}

void LobbyManager::actualizarEstadoJuego(const std::string& code) {
    auto it = lobbies.find(code);
    if (it == lobbies.end()) return;

    // Enviar estado actual a todos los jugadores cada cierto tiempo
    enviarEstadoPartida(code);
}

void LobbyManager::manejarConsultaTurno(sf::TcpSocket* client, const std::string& lobbyCode) {
    auto it = lobbies.find(lobbyCode);
    if (it == lobbies.end()) return;

    auto& partida = it->second.partida;

    // Buscar al jugador que pregunta
    int jugadorId = -1;
    for (size_t i = 0; i < partida.jugadores.size(); ++i) {
        if (partida.jugadores[i].socket == client) {
            jugadorId = i;
            break;
        }
    }

    if (jugadorId == -1) return;

    // Responder si es su turno
    sf::Packet respuesta;
    bool esSuTurno = (partida.jugadorActual == jugadorId);
    respuesta << "RESPUESTA_TURNO" << esSuTurno;

    if (client->send(respuesta) != sf::Socket::Status::Done) {
        std::cerr << "Error al responder consulta de turno" << std::endl;
    }
}

void LobbyManager::notificarCambioTurno(const std::string& lobbyCode) {
    auto it = lobbies.find(lobbyCode);
    if (it == lobbies.end()) return;

    auto& partida = it->second.partida;

    // Notificar a todos los jugadores
    for (size_t i = 0; i < partida.jugadores.size(); ++i) {
        if (partida.jugadores[i].socket) {
            sf::Packet notificacion;
            bool esSuTurno = (partida.jugadorActual == static_cast<int>(i));
            notificacion << "CAMBIAR_TURNO" << esSuTurno;

            if (partida.jugadores[i].socket->send(notificacion) != sf::Socket::Status::Done) {
                std::cerr << "Error al notificar cambio de turno al jugador " << i << std::endl;
            }
        }
    }
}