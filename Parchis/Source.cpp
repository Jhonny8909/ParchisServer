#include <SFML/Network.hpp>
#include <iostream>
#include <string>

#define LISTENER_PORT 55000


void main ()
{
	sf::TcpListener listener;
	sf::TcpSocket Client;
	sf::Packet receivedPacket;
	std::string codigoSala;

	bool closeServer = false;


	if (listener.listen(LISTENER_PORT) != sf::Socket::Status::Done) {
		std::cerr << "Error al escuchar el puerto" << LISTENER_PORT << std::endl;
		return;
	}

		std::cout << "Esperando conexion..." << LISTENER_PORT << std::endl;

		if (listener.accept(Client) != sf::Socket::Status::Done) {
			std::cerr << "Error al aceptar conexión del cliente" << std::endl;
			return;
		}
		
		std::cout << "Cliente conectado desde: " << Client.getRemoteAddress().value() << std::endl;

		while (true)
		{
			sf::Packet packet;
			sf::Socket::Status status = Client.receive(packet);

			if (status == sf::Socket::Status::Disconnected)
			{
				std::cout << "Cliente desconectado" << std::endl;
				break;
			}
			else if (status != sf::Socket::Status::Done)
			{
				std::cerr << "Error al recibir el paquete" << std::endl;
				break;
			}

			std::string codigoSala;
			if (!(packet >> codigoSala))
			{
				std::cerr << "Error al extraer el codigo de sala del paquete" << std::endl;
				break;
			}
			else {
				std::cout << "Codigo de sala recibido: " << codigoSala << std::endl;
			}
		}

}