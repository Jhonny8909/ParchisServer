#pragma once
#include <string>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <mysql_connection.h>

class AuthService {
public:
    static bool loginUser(sql::Connection* con, const std::string& username, const std::string& password);
    static bool registerUser(sql::Connection* con, const std::string& username, const std::string& password);

private:
    static std::string hashSHA256(const std::string& password);
};