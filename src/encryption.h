#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>
#include <vector>
using namespace std;

// RSA Key Pair Generation
void generateRSAKeyPair(const std::string &privateKeyPath, const std::string &publicKeyPath);
bool fileExists(const std::string &filename);
std::string sha256(const std::string &data);
std::string signData(const std::string &data, const std::string &keyFile);

// Base64 Encoding/Decoding
std::string base64Encode(const std::string &data);
std::string base64Decode(const std::string &data);

// AES Encryption/Decryption
// encryption.h
string encryptAES(const string &data, const unsigned char *key, const unsigned char *iv);
string decryptAES(const string &data, const unsigned char *key, const unsigned char *iv);

// AES Key and IV Generation
bool generateAESKeyAndIV(unsigned char *aesKey, unsigned char *aesIv);

// RSA Encryption and Decryption
std::string encryptWithPublicKey(const std::string &data, const std::string &publicKeyFile);
std::string decryptWithPrivateKey(const std::string &data, const std::string &privateKeyFile);

#endif // ENCRYPTION_H

