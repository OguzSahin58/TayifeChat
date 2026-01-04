#ifndef DATABASE_CPP
#define DATABASE_CPP

#include <string>
#include <iostream>

// Use extern "C" to properly link with SQLite C code
extern "C" {
#include "sqlite3.h"
}

class DatabaseManager {
private:
    sqlite3* db;
    std::string db_path;

public:
    DatabaseManager(const std::string& path = "chatserver.db") : db(nullptr), db_path(path) {}

    ~DatabaseManager() {
        if (db) {
            sqlite3_close(db);
        }
    }

    bool connect() {
        int rc = sqlite3_open(db_path.c_str(), &db);

        if (rc != SQLITE_OK) {
            std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        std::cout << "Database connected successfully!" << std::endl;
        return true;
    }

    bool authenticate_user(const std::string& username, const std::string& password) {
        std::string query = "SELECT COUNT(*) FROM users WHERE username = ? AND password = ? AND is_active = 1;";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        int count = 0;

        if (rc == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);

        return count > 0;
    }

    bool username_exists(const std::string& username) {
        std::string query = "SELECT COUNT(*) FROM users WHERE username = ?;";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        int count = 0;

        if (rc == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);

        return count > 0;
    }

    bool update_last_login(const std::string& username) {
        std::string query = "UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE username = ?;";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return rc == SQLITE_DONE;
    }

    int get_user_id(const std::string& username) {
        std::string query = "SELECT id FROM users WHERE username = ?;";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return -1;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        int user_id = -1;

        if (rc == SQLITE_ROW) {
            user_id = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        return user_id;
    }

    bool log_session(int user_id) {
        std::string query = "INSERT INTO sessions (user_id, login_time) VALUES (?, CURRENT_TIMESTAMP);";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_int(stmt, 1, user_id);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return rc == SQLITE_DONE;
    }

    bool add_user(const std::string& username, const std::string& password) {
        std::string query = "INSERT INTO users (username, password) VALUES (?, ?);";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to add user: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        return true;
    }
};

#endif