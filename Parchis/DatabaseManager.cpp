#include "DatabaseManager.h"
#include <iostream>

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {
    driver = get_driver_instance();
}

DatabaseManager::~DatabaseManager() {
    // Limpieza de conexiones pendientes
}

sql::Connection* DatabaseManager::getConnection() {
    try {
        sql::Connection* con = driver->connect(server, username, password);
        con->setSchema(database);
        return con;
    }
    catch (sql::SQLException& e) {
        std::cerr << "Error DB: " << e.what() << std::endl;
        return nullptr;
    }
}

void DatabaseManager::releaseConnection(sql::Connection* con) {
    if (con) {
        delete con;
    }
}

void DatabaseManager::testConnection() {
    std::unique_ptr<sql::Connection> con(getConnection());
    if (!con) {
        throw std::runtime_error("No se pudo conectar a la base de datos");
    }
    std::cout << "Conexión a BD verificada" << std::endl;
}