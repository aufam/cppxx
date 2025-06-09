#include <openssl/sha.h>
#include <iomanip>
#include "workspace.h"


auto Workspace::encrypt(const std::string &input, size_t length) -> std::string {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.c_str()), input.size(), hash);

    std::ostringstream oss;
    for (size_t i = 0; i < length / 2 && i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}


