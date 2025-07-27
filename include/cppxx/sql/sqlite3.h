#ifndef CPPXX_SQL_SQLITE3_H
#define CPPXX_SQL_SQLITE3_H

#include <cppxx/sql/sql.h>
#include <sqlite3.h>


namespace cppxx::sql::sqlite3 {
    class Connection : public cppxx::sql::Connection {
    public:
        Connection(const std::string &filename) {
            fmt::println(stderr, "[DEBUG] Opening database: {:?}", filename);
            int ret = sqlite3_open(filename.c_str(), &db);
            if (ret != SQLITE_OK) {
                std::string what = sqlite3_errmsg(db);
                sqlite3_close(db);
                throw std::runtime_error(fmt::format("Cannot open {:?}: {}", filename, what));
            }
        }

        using ::cppxx::sql::Connection::execute;
        void execute(const char *query) override {
            fmt::println(stderr, "[DEBUG] Executing query: {:?}", query);
            char *err;
            int ret = sqlite3_exec(db, query, nullptr, nullptr, &err);
            if (ret != SQLITE_OK) {
                std::string what = err;
                sqlite3_free(err);
                throw std::runtime_error(fmt::format("Failed to execute query {:?}: {}", query, what));
            }
        }

        virtual ~Connection() {
            fmt::println(stderr, "[DEBUG] Closing database");
            sqlite3_close(db);
        }

    protected:
        struct sqlite3 *db;
    };
} // namespace cppxx::sql::sqlite3

#endif
