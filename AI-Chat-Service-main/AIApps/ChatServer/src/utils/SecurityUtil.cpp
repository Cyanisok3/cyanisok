#include "../../include/utils/SecurityUtil.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{

constexpr int kPasswordIterations = 150000;
constexpr size_t kSaltBytes = 16;
constexpr size_t kKeyBytes = 32;

std::string toHex(const std::vector<unsigned char>& bytes)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char byte : bytes)
    {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

int hexValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

std::vector<unsigned char> fromHex(const std::string& hex)
{
    if (hex.size() % 2 != 0)
    {
        throw std::invalid_argument("Invalid hex string");
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2)
    {
        int high = hexValue(hex[i]);
        int low = hexValue(hex[i + 1]);
        if (high < 0 || low < 0)
        {
            throw std::invalid_argument("Invalid hex string");
        }
        bytes.push_back(static_cast<unsigned char>((high << 4) | low));
    }
    return bytes;
}

std::vector<unsigned char> pbkdf2(const std::string& password,
                                  const std::vector<unsigned char>& salt,
                                  int iterations)
{
    std::vector<unsigned char> key(kKeyBytes);
    int ok = PKCS5_PBKDF2_HMAC(
        password.c_str(),
        static_cast<int>(password.size()),
        salt.data(),
        static_cast<int>(salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(key.size()),
        key.data());

    if (ok != 1)
    {
        throw std::runtime_error("Password hashing failed");
    }
    return key;
}

} // namespace

namespace security
{

bool isValidUsername(const std::string& username)
{
    if (username.size() < 3 || username.size() > 32)
    {
        return false;
    }

    return std::all_of(username.begin(), username.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}

bool isValidPassword(const std::string& password)
{
    return password.size() >= 8 && password.size() <= 128;
}

std::string hashPassword(const std::string& password)
{
    std::vector<unsigned char> salt(kSaltBytes);
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1)
    {
        throw std::runtime_error("Unable to generate password salt");
    }

    std::vector<unsigned char> key = pbkdf2(password, salt, kPasswordIterations);

    return "pbkdf2_sha256$" + std::to_string(kPasswordIterations) + "$" +
           toHex(salt) + "$" + toHex(key);
}

bool verifyPassword(const std::string& password, const std::string& storedHash)
{
    const std::string prefix = "pbkdf2_sha256$";
    if (storedHash.rfind(prefix, 0) != 0)
    {
        return false;
    }

    size_t iterationsStart = prefix.size();
    size_t saltStart = storedHash.find('$', iterationsStart);
    if (saltStart == std::string::npos) return false;

    size_t hashStart = storedHash.find('$', saltStart + 1);
    if (hashStart == std::string::npos) return false;

    try
    {
        int iterations = std::stoi(storedHash.substr(iterationsStart, saltStart - iterationsStart));
        std::vector<unsigned char> salt = fromHex(storedHash.substr(saltStart + 1, hashStart - saltStart - 1));
        std::string expectedHex = storedHash.substr(hashStart + 1);
        std::vector<unsigned char> actualKey = pbkdf2(password, salt, iterations);

        return toHex(actualKey) == expectedHex;
    }
    catch (...)
    {
        return false;
    }
}

} // namespace security
