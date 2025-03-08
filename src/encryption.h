
#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>

void generateRSAKeyPair();
bool fileExists(const std::string &filename);
std::string sha256(const std::string &data);
std::string signData(const std::string &data, const std::string &keyFile);

#endif // ENCRYPTION_H
