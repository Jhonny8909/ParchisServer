#pragma once
#include <SFML/Network.hpp>
#include <map>
#include <string>
#include <array>

// Movemos la constante antes de la estructura Lobby
constexpr int MAX_PLAYERS_LOBBY = 4;  // Definimos la constante aquí

enum class ColorJugador {
    ROJO = 0,
    AMARILLO = 1,
    VERDE = 2,
    AZUL = 3,
    NINGUNO = 4
};

enum class EstadoJuego {
    ESPERANDO_TIRADA,
    DADO_LANZADO,
    ESPERANDO_SELECCION_FICHA,
    MOVIENDO_FICHA,
    TURNO_TERMINADO
};

struct JugadorInfo {
    sf::TcpSocket* socket;
    int fichasEnMeta;
    bool listo;
    ColorJugador color;
};


struct Partida {
    std::array<JugadorInfo, MAX_PLAYERS_LOBBY> jugadores; // Usamos la constante aquí
    int jugadorActual;
    EstadoJuego estado;
    int dadoValue;
    int fichaSeleccionada;
    std::array<int, 16> posicionesFichas; // 4 fichas x 4 jugadores
    bool partidaTerminada;
};

struct Lobby {
    std::string code;
    std::vector<sf::TcpSocket*> players;
    Partida partida;
    bool gameStarted = false;

    Lobby() : players(MAX_PLAYERS_LOBBY, nullptr) {
        // Inicializar colores disponibles en orden FIJO
        coloresDisponibles = {
            ColorJugador::ROJO,    // Slot 0 - Host
            ColorJugador::AMARILLO, // Slot 1
            ColorJugador::VERDE,   // Slot 2
            ColorJugador::AZUL     // Slot 3
        };
    }

    std::vector<ColorJugador> coloresDisponibles;

    ColorJugador asignarColor(size_t slot) {
        switch (slot) {
        case 0: return ColorJugador::ROJO;
        case 1: return ColorJugador::AMARILLO;
        case 2: return ColorJugador::VERDE;
        case 3: return ColorJugador::AZUL;  // Asignación explícita para slot 3
        default: return ColorJugador::NINGUNO;
        }
    }
};

class LobbyManager {
public:
    bool createLobby(const std::string& code, sf::TcpSocket* host);
    bool joinLobby(const std::string& code, sf::TcpSocket* player);
    void startGame(const std::string& code);
    void manejarPaqueteJuego(sf::TcpSocket* client, sf::Packet& packet, const std::string& lobbyCode);
    void manejarConsultaTurno(sf::TcpSocket* client, const std::string& lobbyCode);
    void notificarCambioTurno(const std::string& lobbyCode);

private:
    void cambiarTurno(const std::string& lobbyCode);
    void enviarEstadoPartida(const std::string& lobbyCode);
    void procesarMovimiento(const std::string& lobbyCode, int jugadorId, int fichaId, int pasos);
    void actualizarEstadoJuego(const std::string& code);

    std::map<std::string, Lobby> lobbies;
    static const int MAX_PLAYERS = MAX_PLAYERS_LOBBY; // Mantenemos por compatibilidad
};