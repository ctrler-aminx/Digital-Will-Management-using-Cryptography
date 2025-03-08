#include <iostream>
#include <fstream>
#include <string>
#include "encryption.h"

using namespace std;

void registerUser() {
    string name, aadhar, property, beneficiary1, beneficiary2;
    cout << "\n======= Digital Will Registration =======\n";
    cout << "Enter Name: "; getline(cin, name);
    cout << "Enter Aadhar Number: "; getline(cin, aadhar);
    cout << "Enter Property Details: "; getline(cin, property);
    cout << "Enter Beneficiary 1: "; getline(cin, beneficiary1);
    cout << "Enter Beneficiary 2: "; getline(cin, beneficiary2);
    
    // Hash and sign Aadhar number
    string hashedAadhar = sha256(aadhar);
    string signedAadhar = signData(hashedAadhar, "keys/private.pem");
    
    // Store encrypted details in file
    ofstream file("users.txt", ios::app);
    file << name << "|" << signedAadhar << "|" << property << "|" << beneficiary1 << "|" << beneficiary2 << endl;
    file.close();
    
    cout << "User registered successfully!\n";
}

int main() {
    generateRSAKeyPair();  // Ensure keys exist before proceeding

    int choice;
    cout << "\n======= Digital Will Management System (DWMS) =======\n";
    cout << "1. Register User\n2. Exit\nEnter choice: ";
    cin >> choice;
    cin.ignore(); // Clear input buffer
    
    if (choice == 1)
        registerUser();
    else
        cout << "Exiting...\n";
    
    return 0;
}
/*// digital_will.cpp - Main file for Digital Will Management System (DWMS)
#include <iostream>
#include <fstream>
#include <string>
#include <openssl/aes.h>   // OpenSSL for AES encryption
#include <openssl/sha.h>   // OpenSSL for SHA-256 hashing
#include <openssl/rsa.h>   // OpenSSL for RSA encryption
#include <openssl/pem.h>   // OpenSSL for reading/writing keys
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// AES Key (Hardcoded for now, should be securely generated later)
const unsigned char AES_USER_KEY[16] = "0123456789abcdef"; 

// Function Prototypes
void generateRSAKeyPair();
bool fileExists(const string &filename);
string encryptAES(const string &data);
string sha256(const string &data);
RSA* loadPrivateKey(const string &filename);
RSA* loadPublicKey(const string &filename);
string encryptWithPrivateKey(const string &data, const string &keyFile);
string encryptWithPublicKey(const string &data, const string &keyFile);

// Generate RSA key pair if it doesn't exist
void generateRSAKeyPair() {
    if (fileExists("keys/private.pem") && fileExists("keys/public.pem")) {
        cout << "Keys already exist. Skipping generation.\n";
        return;
    }

    system("mkdir -p keys");

    RSA* rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);
    if (!rsa) {
        cerr << "Error generating RSA key pair!" << endl;
        return;
    }

    // Save Private Key
    FILE* privFile = fopen("keys/private.pem", "wb");
    PEM_write_RSAPrivateKey(privFile, rsa, nullptr, nullptr, 0, nullptr, nullptr);
    fclose(privFile);

    // Save Public Key
    FILE* pubFile = fopen("keys/public.pem", "wb");
    PEM_write_RSA_PUBKEY(pubFile, rsa);
    fclose(pubFile);

    RSA_free(rsa);
    cout << "RSA Key Pair generated successfully.\n";
}

// Check if a file exists
bool fileExists(const string &filename) {
    ifstream file(filename);
    return file.good();
}

// Hash a string using SHA-256
string sha256(const string &data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    return string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

// Load RSA Private Key from File
RSA* loadPrivateKey(const string &filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        cerr << "Error opening private key file!" << endl;
        return nullptr;
    }
    RSA* rsa = PEM_read_RSAPrivateKey(file, nullptr, nullptr, nullptr);
    fclose(file);
    return rsa;
}

// Load RSA Public Key from File
RSA* loadPublicKey(const string &filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        cerr << "Error opening public key file!" << endl;
        return nullptr;
    }
    RSA* rsa = PEM_read_RSA_PUBKEY(file, nullptr, nullptr, nullptr);
    fclose(file);
    return rsa;
}

// Encrypt Data with RSA Private Key
string encryptWithPrivateKey(const string &data, const string &keyFile) {
    RSA* rsa = loadPrivateKey(keyFile);
    if (!rsa) return "";

    unsigned char encrypted[256];
    int encryptedLength = RSA_private_encrypt(data.length(), (unsigned char*)data.c_str(), encrypted, rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    return string(reinterpret_cast<char*>(encrypted), encryptedLength);
}

// Encrypt Data with RSA Public Key
string encryptWithPublicKey(const string &data, const string &keyFile) {
    RSA* rsa = loadPublicKey(keyFile);
    if (!rsa) return "";

    unsigned char encrypted[256];
    int encryptedLength = RSA_public_encrypt(data.length(), (unsigned char*)data.c_str(), encrypted, rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    return string(reinterpret_cast<char*>(encrypted), encryptedLength);
}

// AES Encryption for Property Details (Basic Implementation)
string encryptAES(const string &data) {
    unsigned char encrypted[256] = {0};
    AES_USER_KEY encryptKey;
    AES_set_encrypt_key(AES_USER_KEY, 128, &encryptKey);
    AES_encrypt(reinterpret_cast<const unsigned char*>(data.c_str()), encrypted, &encryptKey);
    return string(reinterpret_cast<char*>(encrypted), 16);
}

// Register a new user
void registerUser() {
    string name, aadhar, property, beneficiary1, beneficiary2;
    cout << "\n======= Digital Will Registration =======\n";
    cout << "Enter Name: "; getline(cin, name);
    cout << "Enter Aadhar Number: "; getline(cin, aadhar);
    cout << "Enter Property Details: "; getline(cin, property);
    cout << "Enter Beneficiary 1: "; getline(cin, beneficiary1);
    cout << "Enter Beneficiary 2: "; getline(cin, beneficiary2);
    
    // Hash and encrypt Aadhar number
    string hashedAadhar = sha256(aadhar);
    string encryptedAadhar = encryptWithPrivateKey(hashedAadhar, "keys/private.pem");
    
    // Encrypt property details using AES
    string encryptedProperty = encryptAES(property);
    
    // Store encrypted details in file
    ofstream file("users.txt", ios::app);
    file << name << "|" << encryptedAadhar << "|" << encryptedProperty << "|" << beneficiary1 << "|" << beneficiary2 << endl;
    file.close();
    
    cout << "User registered successfully!\n";
}

// Connect to CA for verification
void connectToCA() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        cerr << "Error creating socket" << endl;
        return;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed!" << endl;
        return;
    }
    
    string message = "Requesting identity verification";
    send(sock, message.c_str(), message.size(), 0);
    cout << "Verification request sent to CA." << endl;
    close(sock);
}

int main() {
    generateRSAKeyPair();  // Ensure keys exist before proceeding

    int choice;
    cout << "\n======= Digital Will Management System (DWMS) =======\n";
    cout << "1. Register User\n2. Verify with CA\nEnter choice: ";
    cin >> choice;
    cin.ignore(); // Clear input buffer
    
    if (choice == 1)
        registerUser();
    else if (choice == 2)
        connectToCA();
    else
        cout << "Invalid choice!" << endl;
    
    return 0;
}
*/