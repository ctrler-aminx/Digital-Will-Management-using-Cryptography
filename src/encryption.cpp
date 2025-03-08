#include "encryption.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/bn.h>

using namespace std;

bool fileExists(const string &filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

// Generate RSA Key Pair using OpenSSL EVP API
void generateRSAKeyPair(const string& privateKeyPath = "keys/private.pem",
    const string& publicKeyPath = "keys/public.pem") {
if (fileExists(privateKeyPath) && fileExists(publicKeyPath)) {
cout << "Keys already exist. Skipping generation.\n";
return;
}

system("mkdir -p keys");

EVP_PKEY* pkey = EVP_PKEY_new();
RSA* rsa = RSA_new();
BIGNUM* e = BN_new();

if (!pkey || !rsa || !e) {
cerr << "Error allocating memory for RSA key pair!\n";
return;
}

BN_set_word(e, RSA_F4);
if (RSA_generate_key_ex(rsa, 2048, e, nullptr) != 1) {
cerr << "Error generating RSA key!\n";
return;
}
EVP_PKEY_assign_RSA(pkey, rsa);

FILE* privFile = fopen(privateKeyPath.c_str(), "wb");
if (!privFile) {
cerr << "Error creating private key file!\n";
return;
}
PEM_write_PrivateKey(privFile, pkey, nullptr, nullptr, 0, nullptr, nullptr);
fclose(privFile);

FILE* pubFile = fopen(publicKeyPath.c_str(), "wb");
if (!pubFile) {
cerr << "Error creating public key file!\n";
return;
}
PEM_write_PUBKEY(pubFile, pkey);
fclose(pubFile);

EVP_PKEY_free(pkey);
BN_free(e);

cout << "RSA Key Pair generated successfully.\n";
}

// Compute SHA-256 Hash
string sha256(const string &data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    return string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

// Load RSA Private Key using EVP API
EVP_PKEY* loadPrivateKey(const string &filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        cerr << "Error opening private key file!" << endl;
        return nullptr;
    }
    EVP_PKEY* pkey = PEM_read_PrivateKey(file, nullptr, nullptr, nullptr);
    fclose(file);
    return pkey;
}

// Sign Data with RSA Private Key
string signData(const string &data, const string &keyFile) {
    EVP_PKEY* pkey = loadPrivateKey(keyFile);
    if (!pkey) return "";

    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
    if (!mdCtx) {
        cerr << "Error creating message digest context!" << endl;
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_DigestSignInit(mdCtx, nullptr, EVP_sha256(), nullptr, pkey) != 1) {
        cerr << "Error initializing signing operation!" << endl;
        EVP_MD_CTX_free(mdCtx);
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_DigestSignUpdate(mdCtx, data.c_str(), data.length()) != 1) {
        cerr << "Error updating signing operation!" << endl;
        EVP_MD_CTX_free(mdCtx);
        EVP_PKEY_free(pkey);
        return "";
    }

    size_t sigLen = 0;
    EVP_DigestSignFinal(mdCtx, nullptr, &sigLen);
    unsigned char* signature = new unsigned char[sigLen];

    if (EVP_DigestSignFinal(mdCtx, signature, &sigLen) != 1) {
        cerr << "Error finalizing signing operation!" << endl;
        delete[] signature;
        EVP_MD_CTX_free(mdCtx);
        EVP_PKEY_free(pkey);
        return "";
    }

    string signedData(reinterpret_cast<char*>(signature), sigLen);
    delete[] signature;
    EVP_MD_CTX_free(mdCtx);
    EVP_PKEY_free(pkey);
    return signedData;
}

// Encrypt data with RSA private key
string encryptWithPrivateKey(const string &data, const string &keyFile) {
    RSA* rsa = RSA_new();
    FILE* file = fopen(keyFile.c_str(), "r");
    if (!file) {
        cerr << "Error opening private key file!" << endl;
        return "";
    }
    rsa = PEM_read_RSAPrivateKey(file, nullptr, nullptr, nullptr);
    fclose(file);

    unsigned char encrypted[256];
    int encryptedLength = RSA_private_encrypt(
        data.length(),
        reinterpret_cast<const unsigned char*>(data.c_str()),
        encrypted, rsa, RSA_PKCS1_PADDING
    );

    RSA_free(rsa);
    return string(reinterpret_cast<char*>(encrypted), encryptedLength);
}
