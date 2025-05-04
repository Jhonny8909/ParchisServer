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
#include "Lobby.h"

#define LISTENER_PORT 55000
/*#define SERVER "127.0.0.1"
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
        std::cout << "Conexion exitosa a la base de datos." << std::endl;
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
        std::cout << "Error al iniciar sesion: " << e.what() << std::endl;
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
        std::cout << "Conexion cerrada correctamente." << std::endl;
    }
}
*/
//std::map<std::string, sf::IpAddress> salas;

int main() {
    sf::TcpListener listener;

    if (listener.listen(LISTENER_PORT) != sf::Socket::Status::Done) {
        std::cerr << "No se pudo abrir el puerto " << LISTENER_PORT << std::endl;
        return -1;
    }

    std::cout << "Servidor escuchando en el puerto " << LISTENER_PORT << std::endl;

    while (true) {
        sf::TcpSocket* client = new sf::TcpSocket;

        if (listener.accept(*client) == sf::Socket::Status::Done) {
            std::cout << "Cliente conectado desde " << client->getRemoteAddress().value() << std::endl;
            std::thread(manejarCliente, client).detach();
        }
        else {
            std::cerr << "Fallo al aceptar cliente" << std::endl;
            delete client;
        }
    }

    return 0;
}
    
