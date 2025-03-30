#include "encryption.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/buffer.h>

using namespace std;

bool fileExists(const string &filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

// ======== Base64 Encoding Function ========
string base64Encode(const string &data) {
    BIO* bio = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    bio = BIO_push(bio, bmem);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data.c_str(), data.length());
    BIO_flush(bio);

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);

    string encodedData(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    return encodedData;
}

// ======== Base64 Decoding Function ========
string base64Decode(const string &data) {
    BIO* bio = BIO_new_mem_buf(data.c_str(), data.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    char buffer[512];
    int length = BIO_read(bio, buffer, sizeof(buffer));
    BIO_free_all(bio);

    return string(buffer, length);
}

// ======== Generate RSA Key Pair using OpenSSL EVP API ========
void generateRSAKeyPair(const string& privateKeyPath, const string& publicKeyPath) {
    if (fileExists(privateKeyPath) && fileExists(publicKeyPath)) {
        cout << "Keys already exist. Skipping generation.\n";
        return;
    }

    system("mkdir -p keys");

    EVP_PKEY* pkey = EVP_PKEY_new();
    RSA* rsa = RSA_new();
    BIGNUM* e = BN_new();

    BN_set_word(e, RSA_F4);
    if (RSA_generate_key_ex(rsa, 2048, e, nullptr) != 1) {
        cerr << "Error generating RSA key!\n";
        return;
    }
    EVP_PKEY_assign_RSA(pkey, rsa);

    FILE* privFile = fopen(privateKeyPath.c_str(), "wb");
    PEM_write_PrivateKey(privFile, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    fclose(privFile);

    FILE* pubFile = fopen(publicKeyPath.c_str(), "wb");
    PEM_write_PUBKEY(pubFile, pkey);
    fclose(pubFile);

    EVP_PKEY_free(pkey);
    BN_free(e);

    cout << "RSA Key Pair generated successfully.\n";
}

// ======== Compute SHA-256 Hash ========
string sha256(const string &data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    return base64Encode(string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH));
}

// ======== Load RSA Private Key ========
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

// ======== Load RSA Public Key ========
EVP_PKEY* loadPublicKey(const string &filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        cerr << "Error opening public key file!" << endl;
        return nullptr;
    }
    EVP_PKEY* pkey = PEM_read_PUBKEY(file, nullptr, nullptr, nullptr);
    fclose(file);
    return pkey;
}

// ======== Sign Data with RSA Private Key ========
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
    string encodedSignature = base64Encode(signedData);

    delete[] signature;
    EVP_MD_CTX_free(mdCtx);
    EVP_PKEY_free(pkey);

    return encodedSignature;
}

// ======== Encrypt with Public Key ========
string encryptWithPublicKey(const string &data, const string &keyFile) {
    EVP_PKEY* pkey = loadPublicKey(keyFile);
    if (!pkey) return "";

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        cerr << "Error creating context for encryption." << endl;
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        cerr << "Error initializing encryption." << endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    size_t outLen;
    EVP_PKEY_encrypt(ctx, nullptr, &outLen, reinterpret_cast<const unsigned char*>(data.c_str()), data.size());

    unsigned char* outData = new unsigned char[outLen];

    if (EVP_PKEY_encrypt(ctx, outData, &outLen, reinterpret_cast<const unsigned char*>(data.c_str()), data.size()) <= 0) {
        cerr << "Error encrypting data." << endl;
        delete[] outData;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    string encryptedData(reinterpret_cast<char*>(outData), outLen);
    string encodedData = base64Encode(encryptedData);

    delete[] outData;
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return encodedData;
}

// ======== AES Key and IV Generation ========
bool generateAESKeyAndIV(unsigned char *key, unsigned char *iv) {
    if (!RAND_bytes(key, AES_BLOCK_SIZE) || !RAND_bytes(iv, AES_BLOCK_SIZE)) {
        cerr << "Error generating AES key or IV." << endl;
        return false;
    }
    return true;
}

// ======== Encrypt Data with AES ========
string encryptAES(const string &data, const unsigned char *key, const unsigned char *iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

    int len;
    int ciphertextLen;
    unsigned char ciphertext[data.size() + AES_BLOCK_SIZE];

    EVP_EncryptUpdate(ctx, ciphertext, &len, reinterpret_cast<const unsigned char*>(data.c_str()), data.size());
    ciphertextLen = len;

    EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
    ciphertextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    return base64Encode(string(reinterpret_cast<char*>(ciphertext), ciphertextLen));
}

// ======== Decrypt Data with AES ========
string decryptAES(const string &data, const unsigned char *key, const unsigned char *iv) {
    string decodedData = base64Decode(data);
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

    int len;
    int plaintextLen;
    unsigned char plaintext[decodedData.size() + AES_BLOCK_SIZE];

    EVP_DecryptUpdate(ctx, plaintext, &len, reinterpret_cast<const unsigned char*>(decodedData.c_str()), decodedData.size());
    plaintextLen = len;

    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintextLen += len;

    EVP_CIPHER_CTX_free(ctx);

    return string(reinterpret_cast<char*>(plaintext), plaintextLen);
}

// ======== Decrypt with Private Key ========
string decryptWithPrivateKey(const string &data, const string &keyFile) {
    EVP_PKEY* pkey = loadPrivateKey(keyFile);
    if (!pkey) return "";

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        cerr << "Error creating context for decryption." << endl;
        EVP_PKEY_free(pkey);
        return "";
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        cerr << "Error initializing decryption." << endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    string decodedData = base64Decode(data);
    size_t outLen;
    
    // Determine buffer length
    if (EVP_PKEY_decrypt(ctx, nullptr, &outLen, reinterpret_cast<const unsigned char*>(decodedData.c_str()), decodedData.size()) <= 0) {
        cerr << "Error determining buffer length for decryption." << endl;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    unsigned char* outData = new unsigned char[outLen];

    // Perform decryption
    if (EVP_PKEY_decrypt(ctx, outData, &outLen, reinterpret_cast<const unsigned char*>(decodedData.c_str()), decodedData.size()) <= 0) {
        cerr << "Error decrypting data." << endl;
        delete[] outData;
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return "";
    }

    string decryptedData(reinterpret_cast<char*>(outData), outLen);
    delete[] outData;

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return decryptedData;
}
