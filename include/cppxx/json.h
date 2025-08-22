#ifndef CPPXX_JSON_H
#define CPPXX_JSON_H

#include "literal.h"


namespace cppxx::json {
    template <typename T, literal K>
    struct Field {
        static constexpr literal Key = K;

        // must define
        // Any ref(const T &object) const { return object.field; }
    };

    template <typename JType, typename CType, typename... Objects>
    struct const_Ref {
        const CType &ref;
    };

    template <typename JType, typename CType, typename... Objects>
    struct Ref {
        CType &ref;
    };
} // namespace cppxx::json

#ifdef INCLUDE_NLOHMANN_JSON_HPP_
namespace cppxx::json {
    template <typename CType, typename... Objects>
    void to_json(nlohmann::json &j, const const_Ref<nlohmann::json, CType, Objects...> &v) {
        ((j[Objects::Key.value] = Objects{}.ref(v.ref)), ...);
    }

    template <typename CType, typename... Objects>
    void from_json(const nlohmann::json &j, Ref<nlohmann::json, CType, Objects...> &&v) {
        ((Objects{}.ref(v.ref) = j[Objects::Key.value]), ...);
    }
} // namespace cppxx::json
#endif
#endif
