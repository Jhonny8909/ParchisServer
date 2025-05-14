#pragma once
#include <mysql_connection.h>
#include <cppconn/driver.h>

class DatabaseManager {
public:
    static DatabaseManager& getInstance();

    sql::Connection* getConnection();
    void releaseConnection(sql::Connection* con);
    void testConnection();

private:
    DatabaseManager();
    ~DatabaseManager();

    sql::Driver* driver;
    std::string server = "127.0.0.1";
    std::string username = "root";
    std::string password = "";
    std::string database = "loginparchis";
};