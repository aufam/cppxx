#ifndef CPPXX_SQL_CONNECTION_H
#define CPPXX_SQL_CONNECTION_H

#include <cppxx/literal.h>


namespace cppxx::sql {
    class Connection {
    public:
        virtual ~Connection() = default;
    };
} // namespace cppxx::sql

#endif
