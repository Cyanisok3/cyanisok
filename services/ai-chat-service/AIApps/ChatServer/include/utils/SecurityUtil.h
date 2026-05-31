#pragma once

#include <string>

namespace security
{

bool isValidUsername(const std::string& username);
bool isValidPassword(const std::string& password);
std::string hashPassword(const std::string& password);
bool verifyPassword(const std::string& password, const std::string& storedHash);

} // namespace security
