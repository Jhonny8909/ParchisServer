#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#define LISTENER_PORT 55000
#define SERVER "127.0.0.1"
#define USERNAME "root"
#define PASSWORD ""
#define DATABASE "loginparchis"

std::string hashSHA256(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();

    if (mdctx == nullptr) {
        std::cerr << "Error al crear contexto EVP" << std::endl;
        return "";
    }

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr)) {
        std::cerr << "Error al inicializar digest" << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    if (1 != EVP_DigestUpdate(mdctx, password.c_str(), password.size())) {
        std::cerr << "Error al actualizar digest" << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    unsigned int lengthOfHash = 0;
    if (1 != EVP_DigestFinal_ex(mdctx, hash, &lengthOfHash)) {
        std::cerr << "Error al finalizar digest" << std::endl;
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

void connectDatabase(sql::Driver*& driver, sql::Connection*& con) {
    try {
        driver = get_driver_instance();
        con = driver->connect(SERVER, USERNAME, PASSWORD);
        std::cout << "Conexión exitosa a la base de datos." << std::endl;
        con->setSchema(DATABASE);
    }
    catch (sql::SQLException& e) {
        std::cout << "Error al conectar a la base de datos: " << e.what() << std::endl;
    }
}

bool loginUser(sql::Connection* con, const std::string& username, const std::string& password) {
    try {
        sql::PreparedStatement* stmt = con->prepareStatement(
            "SELECT contrasena FROM usuarios WHERE nombre = ?"
        );
        stmt->setString(1, username);
        sql::ResultSet* res = stmt->executeQuery();

        if (!res->next()) {
            std::cout << "Error: Usuario no encontrado." << std::endl;
            delete res;
            delete stmt;
            return false;
        }

        std::string storedHashedPassword = res->getString("contrasena");
        std::string inputHashedPassword = hashSHA256(password);

        if (inputHashedPassword.empty()) {
            std::cout << "Error al hashear la contraseña ingresada." << std::endl;
            delete res;
            delete stmt;
            return false;
        }

        delete res;
        delete stmt;

        if (inputHashedPassword == storedHashedPassword) {
            std::cout << "Inicio de sesión exitoso. ¡Bienvenido, " << username << "!" << std::endl;
            return true;
        }
        else {
            std::cout << "Error: Contraseña incorrecta." << std::endl;
            return false;
        }
    }
    catch (sql::SQLException& e) {
        std::cout << "Error al iniciar sesión: " << e.what() << std::endl;
        return false;
    }
}

bool registerUser(sql::Connection* con, const std::string& username, const std::string& password) {
    try {
        sql::PreparedStatement* checkStmt = con->prepareStatement(
            "SELECT nombre FROM usuarios WHERE nombre = ?"
        );
        checkStmt->setString(1, username);
        sql::ResultSet* res = checkStmt->executeQuery();

        if (res->next()) {
            std::cout << "Error: El usuario ya existe." << std::endl;
            delete res;
            delete checkStmt;
            return false;
        }
        delete res;
        delete checkStmt;

        std::string hashedPassword = hashSHA256(password);
        if (hashedPassword.empty()) {
            std::cout << "Error al hashear la contraseña." << std::endl;
            return false;
        }

        sql::PreparedStatement* insertStmt = con->prepareStatement(
            "INSERT INTO usuarios (nombre, contrasena) VALUES (?, ?)"
        );
        insertStmt->setString(1, username);
        insertStmt->setString(2, hashedPassword);
        insertStmt->executeUpdate();

        std::cout << "Usuario registrado exitosamente." << std::endl;
        delete insertStmt;
        return true;
    }
    catch (sql::SQLException& e) {
        std::cout << "Error al registrar usuario: " << e.what() << std::endl;
        return false;
    }
}

void disconnectDatabase(sql::Connection* con) {
    if (con != nullptr) {
        con->close();
        delete con;
        std::cout << "Conexión cerrada correctamente." << std::endl;
    }
}

int main() {
    sf::TcpListener listener;
    sql::Driver* driver;
    sql::Connection* con;

    connectDatabase(driver, con);

    if (con == nullptr || con->isClosed()) {
        std::cout << "No se pudo establecer conexión con la base de datos." << std::endl;
        return 1;
    }

    if (listener.listen(LISTENER_PORT) != sf::Socket::Status::Done) {
        std::cerr << "Error al escuchar el puerto " << LISTENER_PORT << std::endl;
        return 1;
    }

    std::cout << "Esperando conexión en el puerto " << LISTENER_PORT << std::endl;

    sf::TcpSocket client;
    if (listener.accept(client) != sf::Socket::Status::Done) {
        std::cerr << "Error al aceptar conexión del cliente" << std::endl;
        return 1;
    }

    std::cout << "Cliente conectado desde: " << client.getRemoteAddress().value() << std::endl;

    while (true) {
        sf::Packet packet;
        sf::Socket::Status status = client.receive(packet);

        if (status == sf::Socket::Status::Disconnected) {
            std::cout << "Cliente desconectado" << std::endl;
            break;
        }
        else if (status != sf::Socket::Status::Done) {
            std::cerr << "Error al recibir el paquete" << std::endl;
            break;
        }

        // Debug: Print raw packet size
        std::cout << "Packet size received: " << packet.getDataSize() << " bytes" << std::endl;

        // Extract data with proper error checking
        bool isLogin;
        std::string username;
        std::string password;

        if (!(packet >> isLogin)) {
            std::cerr << "Failed to extract isLogin flag" << std::endl;
            continue;
        }

        if (!(packet >> username)) {
            std::cerr << "Failed to extract username" << std::endl;
            continue;
        }

        if (!(packet >> password)) {
            std::cerr << "Failed to extract password" << std::endl;
            continue;
        }

        std::cout << "Received data - isLogin: " << isLogin
            << ", username: " << username
            << ", password: " << password << std::endl;

        // Process the request
        bool success;
        if (isLogin) {
            success = loginUser(con, username, password);
        }
        else {
            success = registerUser(con, username, password);
        }

        // Send response
        sf::Packet responsePacket;
        responsePacket << success;
        if (client.send(responsePacket) != sf::Socket::Status::Done) {
            std::cerr << "Error al enviar respuesta al cliente" << std::endl;
        }
    }

    disconnectDatabase(con);
    return 0;
}