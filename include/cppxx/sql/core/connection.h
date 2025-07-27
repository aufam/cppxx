#ifndef CPPXX_SQL_CONNECTION_H
#define CPPXX_SQL_CONNECTION_H

#include <cppxx/literal.h>


namespace cppxx::sql {
    class Connection {
    public:
        virtual void execute(const char *query) = 0;

        template <size_t N>
        void execute(const literal<N> &query) {
            execute(query.value);
        }

        void execute(const std::string &query) { execute(query.c_str()); }

        virtual ~Connection() = default;
    };
} // namespace cppxx::sql

#endif
