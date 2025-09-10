#include <sha256.h>
#include "workspace.h"


auto Workspace::encrypt(const std::string &input, size_t length) -> std::string {
    std::string hash = SHA256::hashString(input);
    return hash.substr(0, length);
}
