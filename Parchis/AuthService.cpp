#include "AuthService.h"
#include <iomanip>
#include <sstream>
#include <memory>
// Incluir los headers específicos de MySQL Connector
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

using namespace std;

std::string AuthService::hashSHA256(const std::string& password) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) {
        throw runtime_error("Error al crear contexto EVP");
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if (!EVP_DigestInit_ex(context, EVP_sha256(), nullptr)) {
        EVP_MD_CTX_free(context);
        throw runtime_error("Error al inicializar digest");
    }

    if (!EVP_DigestUpdate(context, password.c_str(), password.length())) {
        EVP_MD_CTX_free(context);
        throw runtime_error("Error al actualizar digest");
    }

    if (!EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
        EVP_MD_CTX_free(context);
        throw runtime_error("Error al finalizar digest");
    }

    EVP_MD_CTX_free(context);

    stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool AuthService::loginUser(sql::Connection* con, const std::string& username, const std::string& password) {
    if (!con) return false;

    try {
        unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement(
            "SELECT contrasena FROM usuarios WHERE nombre = ?"
        ));
        stmt->setString(1, username);

        unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if (!res->next()) return false;

        string storedHash = res->getString("contrasena");
        string inputHash = hashSHA256(password);

        return (storedHash == inputHash);
    }
    catch (const sql::SQLException& e) {
        cerr << "SQL Error en login: " << e.what() << endl;
        return false;
    }
    catch (...) {
        cerr << "Error desconocido en login" << endl;
        return false;
    }
}

bool AuthService::registerUser(sql::Connection* con, const std::string& username, const std::string& password) {
    if (!con) return false;

    try {
        // Verificar si el usuario ya existe
        unique_ptr<sql::PreparedStatement> checkStmt(con->prepareStatement(
            "SELECT id FROM usuarios WHERE nombre = ?"
        ));
        checkStmt->setString(1, username);
        unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        if (res->next()) return false;

        // Registrar nuevo usuario
        unique_ptr<sql::PreparedStatement> insertStmt(con->prepareStatement(
            "INSERT INTO usuarios (nombre, contrasena) VALUES (?, ?)"
        ));
        insertStmt->setString(1, username);
        insertStmt->setString(2, hashSHA256(password));

        return (insertStmt->executeUpdate() > 0);
    }
    catch (const sql::SQLException& e) {
        cerr << "SQL Error en registro: " << e.what() << endl;
        return false;
    }
    catch (...) {
        cerr << "Error desconocido en registro" << endl;
        return false;
    }
}