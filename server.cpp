#include "crow_all.h"
#include <sqlite3.h>

void initializeDatabase() {
    sqlite3 *db;
    int rc = sqlite3_open("user_database.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    const char *sql = "CREATE TABLE IF NOT EXISTS users ("
                      "username TEXT PRIMARY KEY, "
                      "password TEXT NOT NULL, "
                      "permission TEXT NOT NULL);";
    char *errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    sqlite3_close(db);
}

int main() {
    crow::SimpleApp app;

    initializeDatabase();

    CROW_ROUTE(app, "/create_user").methods("POST"_method)([](const crow::request& req) {
        sqlite3 *db;
        sqlite3_open("user_database.db", &db);

        auto json = crow::json::load(req.body);
        std::string username = json["username"].s();
        std::string password = json["password"].s();
        std::string permission = json["permission"].s();

        sqlite3_stmt *stmt;
        const char *sql = "INSERT INTO users (username, password, permission) VALUES (?, ?, ?)";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, permission.c_str(), -1, SQLITE_STATIC);

        int rc = sqlite3_step(stmt);
        crow::json::wvalue response;
        if (rc == SQLITE_DONE) {
            response["message"] = "User created successfully";
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return crow::response(201, response);
        } else {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return crow::response(500, "{\"message\":\"Error creating user\"}");
        }
    });

    CROW_ROUTE(app, "/login").methods("POST"_method)([](const crow::request& req) {
        sqlite3 *db;
        sqlite3_open("user_database.db", &db);

        auto json = crow::json::load(req.body);
        std::string username = json["username"].s();
        std::string password = json["password"].s();

        sqlite3_stmt *stmt;
        const char *sql = "SELECT password, permission FROM users WHERE username = ?";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        int rc = sqlite3_step(stmt);
        crow::json::wvalue response;
        if (rc == SQLITE_ROW) {
            std::string storedPassword = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string permission = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (storedPassword == password) {
                response["message"] = "Login successful";
                response["permission"] = permission;
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                return crow::response(200, response);
            } else {
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                return crow::response(401, "{\"message\":\"Incorrect password\"}");
            }
        } else {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return crow::response(401, "{\"message\":\"Invalid credentials\"}");
        }
    });

    app.port(8080).multithreaded().run();
}
