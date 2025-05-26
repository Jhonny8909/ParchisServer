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
    lobby.partida.jugadorActual = 0; // Jugador rojo inicia

    for (size_t i = 0; i < lobby.players.size(); ++i) {
        if (lobby.players[i]) {
            sf::Packet startPacket;
            // Asegurar el orden: tipo, color, código, booleano
            startPacket << "GAME_START"
                << static_cast<int>(lobby.partida.jugadores[i].color)
                << code
                << (i == lobby.partida.jugadorActual); // true solo para el jugador actual

            std::cout << "[SERVER] Enviando GAME_START - Color: "
                << static_cast<int>(lobby.partida.jugadores[i].color)
                << ", Código: " << code
                << ", Es turno: " << (i == lobby.partida.jugadorActual) << std::endl;

            if (lobby.players[i]->send(startPacket) != sf::Socket::Status::Done) {
                std::cerr << "Error notificando inicio a jugador " << i << std::endl;
            }
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
    std::string tipoPaquete;
    if (!(packet >> tipoPaquete)) return;

    auto& lobby = lobbies[lobbyCode];
    int jugadorId = -1;

    // Buscar jugador por socket y color
    for (size_t i = 0; i < lobby.partida.jugadores.size(); ++i) {
        if (lobby.partida.jugadores[i].socket == client) {
            jugadorId = i;
            break;
        }
    }

    if (tipoPaquete == "TIRAR_DADO") {
        ColorJugador color;
        if (!(packet >> color)) return;

        if (jugadorId == lobby.partida.jugadorActual &&
            lobby.partida.jugadores[jugadorId].color == color) {

            int valorDado = (std::rand() % 6) + 1;
            lobby.partida.dadoValue = valorDado;

            sf::Packet respuesta;
            respuesta << "DADO_RESULTADO" << valorDado << color; // Asegúrate de incluir el color
            broadcastToLobby(lobbyCode, respuesta);
        }
    }
    else if (tipoPaquete == "MOVIMIENTO") {
        ColorJugador color;
        int fichaId, nuevaPos;
        if (!(packet >> color >> fichaId >> nuevaPos)) return;

        // Validar movimiento
        if (jugadorId == lobby.partida.jugadorActual) {
            // Actualizar posición
            lobby.partida.posicionesFichas[fichaId] = nuevaPos;

            // Notificar a todos
            sf::Packet update;
            update << "ACTUALIZAR_POSICION" << color << fichaId << nuevaPos;
            broadcastToLobby(lobbyCode, update);

            // Cambiar turno solo después de movimiento válido
            cambiarTurno(lobbyCode);
        }
    }
}

void LobbyManager::cambiarTurno(const std::string& lobbyCode) {
    auto& lobby = lobbies[lobbyCode];
    int jugadorAnterior = lobby.partida.jugadorActual;

    // Cambiar al siguiente jugador disponible
    do {
        lobby.partida.jugadorActual = (lobby.partida.jugadorActual + 1) % MAX_PLAYERS_LOBBY;
    } while (!lobby.partida.jugadores[lobby.partida.jugadorActual].socket &&
        lobby.partida.jugadorActual != jugadorAnterior);

    // Notificar a todos los jugadores
    for (size_t i = 0; i < lobby.partida.jugadores.size(); ++i) {
        if (lobby.partida.jugadores[i].socket) {
            sf::Packet turnPacket;
            bool esSuTurno = (static_cast<int>(i) == lobby.partida.jugadorActual);
            turnPacket << "CAMBIAR_TURNO" << esSuTurno;

            if (lobby.partida.jugadores[i].socket->send(turnPacket) != sf::Socket::Status::Done) {
                std::cerr << "Error notificando turno a jugador " << i << std::endl;
            }
            else {
                std::cout << "[SERVER] Notificado jugador " << i
                    << " (Color: " << static_cast<int>(lobby.partida.jugadores[i].color)
                    << "), es su turno: " << esSuTurno << std::endl;
            }
        }
    }
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

void LobbyManager::notificarCambioTurno(const std::string& lobbyCode) {
    auto it = lobbies.find(lobbyCode);
    if (it == lobbies.end()) {
        std::cerr << "[SERVER ERROR] Lobby no encontrado: " << lobbyCode << std::endl;
        return;
    }

    std::cout << "[SERVER] Preparando notificación de turno para lobby: " << lobbyCode
        << ". Jugador actual: " << it->second.partida.jugadorActual << std::endl;

    for (size_t i = 0; i < it->second.partida.jugadores.size(); ++i) {
        if (it->second.partida.jugadores[i].socket) {
            sf::Packet notif;
            bool esSuTurno = (it->second.partida.jugadorActual == static_cast<int>(i));
            notif << "CAMBIAR_TURNO" << esSuTurno;

            std::cout << "[SERVER] Enviando a jugador " << i
                << " (Color: " << static_cast<int>(it->second.partida.jugadores[i].color)
                << "), Socket: " << it->second.partida.jugadores[i].socket
                << ", Es su turno: " << esSuTurno << std::endl;

            sf::Socket::Status status = it->second.partida.jugadores[i].socket->send(notif);

            if (status != sf::Socket::Status::Done) {
                std::cerr << "[SERVER ERROR] Fallo al enviar a jugador " << i
                    << ". Código de error: " << static_cast<int>(status) << std::endl;
            }
            else {
                std::cout << "[SERVER] Notificación enviada con éxito a jugador " << i << std::endl;
            }
        }
    }
}

void LobbyManager::broadcastToLobby(const std::string& lobbyCode, sf::Packet& packet) {
    auto it = lobbies.find(lobbyCode);
    if (it != lobbies.end()) {
        // Clonar el paquete para no afectar el original
        sf::Packet packetCopy;
        std::string packetType;
        packet >> packetType;
        packetCopy << packetType;

        // Copiar el resto del contenido
        while (!packet.endOfPacket()) {
            std::string data;
            packet >> data;
            packetCopy << data;
        }

        for (auto& jugador : it->second.partida.jugadores) {
            if (jugador.socket) {
                // Enviar copia del paquete
                if (jugador.socket->send(packetCopy) != sf::Socket::Status::Done) {
                    std::cerr << "[SERVER] Error enviando a jugador "
                        << static_cast<int>(jugador.color) << std::endl;
                }
            }
        }
    }
}

Lobby& LobbyManager::getLobby(const std::string& code) {
    auto it = lobbies.find(code);
    if (it == lobbies.end()) {
        throw std::runtime_error("Lobby no encontrado: " + code);
    }
    return it->second;
}

const Lobby& LobbyManager::getLobby(const std::string& code) const {
    auto it = lobbies.find(code);
    if (it == lobbies.end()) {
        throw std::runtime_error("Lobby no encontrado: " + code);
    }
    return it->second;
}
