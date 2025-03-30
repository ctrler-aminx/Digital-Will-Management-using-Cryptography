#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace std;

// Function to create a directory for the user
void createUserDirectory(const string &username) {
    string dirPath = "./data/" + username;
    mkdir(dirPath.c_str(), 0777);
}

// Function to generate a random AES key
void generateAESKey(unsigned char *key, int keySize) {
    RAND_bytes(key, keySize);
}

// Function to encrypt data using AES EVP API
bool encryptAES(const unsigned char *key, const unsigned char *iv, const string &plainText, vector<unsigned char> &cipherText) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int len;
    vector<unsigned char> encryptedData(plainText.size() + 16);
    if (EVP_EncryptUpdate(ctx, encryptedData.data(), &len, (unsigned char *)plainText.c_str(), plainText.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    int ciphertextLen = len;

    if (EVP_EncryptFinal_ex(ctx, encryptedData.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertextLen += len;
    encryptedData.resize(ciphertextLen);
    cipherText.insert(cipherText.end(), encryptedData.begin(), encryptedData.end());

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

// Function to encrypt AES key using RSA EVP API
vector<unsigned char> encryptRSA(const unsigned char *aesKey, int keySize, const string &pubKeyFile) {
    FILE *pubFile = fopen(pubKeyFile.c_str(), "r");
    EVP_PKEY *pkey = PEM_read_PUBKEY(pubFile, NULL, NULL, NULL);
    fclose(pubFile);

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pkey, NULL);
    EVP_PKEY_encrypt_init(ctx);

    size_t outLen;
    EVP_PKEY_encrypt(ctx, NULL, &outLen, aesKey, keySize);

    vector<unsigned char> encryptedKey(outLen);
    EVP_PKEY_encrypt(ctx, encryptedKey.data(), &outLen, aesKey, keySize);
    encryptedKey.resize(outLen);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return encryptedKey;
}

// Function to get list of beneficiaries from database
void getBeneficiaries(const string &userAadhar, vector<string> &beneficiaries, vector<string> &aadhars, vector<string> &publicKeys) {
    string path = "./data/" + userAadhar + "/relatives/";
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path.c_str())) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            string relativeDir = entry->d_name;
            if (relativeDir != "." && relativeDir != "..") {
                string relativePath = path + relativeDir;
                string relInfoPath = relativePath + "/relative.txt";
                ifstream relFile(relInfoPath);
                string name, aadhar, pubKeyPath = relativePath + "/public_key.txt";
                getline(relFile, name);
                getline(relFile, aadhar);
                beneficiaries.push_back(name);
                aadhars.push_back(aadhar);
                publicKeys.push_back(pubKeyPath);
                relFile.close();
            }
        }
        closedir(dir);
    } else {
        cerr << "Error: Unable to open relatives directory." << endl;
    }
}

int main() {
    string username, userAadhar, userName;
    cout << "Enter your name: ";
    getline(cin, userName);
    cout << "Enter your Aadhar number: ";
    getline(cin, userAadhar);

    vector<string> beneficiaries, aadhars, publicKeys;
    getBeneficiaries(userAadhar, beneficiaries, aadhars, publicKeys);

    if (beneficiaries.empty()) {
        cerr << "No beneficiaries found for this Aadhar." << endl;
        return 1;
    }

    cout << "\nUser Details:\n";
    cout << "Name: " << userName << "\n";
    cout << "Aadhar: " << userAadhar << "\n\n";

    cout << "Beneficiaries for this will:\n";
    for (size_t i = 0; i < beneficiaries.size(); i++) {
        cout << i + 1 << ". " << beneficiaries[i] << " (Aadhar: " << aadhars[i] << ")\n";
    }

    int numProperties;
    cout << "\nEnter number of properties: ";
    cin >> numProperties;
    cin.ignore();

    vector<string> properties(numProperties);
    vector<string> assignedBeneficiaries(numProperties);
    for (int i = 0; i < numProperties; i++) {
        cout << "Enter property " << i + 1 << " details: ";
        getline(cin, properties[i]);
        cout << "Assign to beneficiary (1-" << beneficiaries.size() << "): ";
        int choice;
        cin >> choice;
        cin.ignore();
        assignedBeneficiaries[i] = beneficiaries[choice - 1];
    }

    string willContent = "Will of Aadhar: " + userAadhar + "\n";
    for (int i = 0; i < numProperties; i++) {
        willContent += "Property: " + properties[i] + " assigned to " + assignedBeneficiaries[i] + "\n";
    }

    // Save will in will.txt
    string willPath = "./data/" + userAadhar + "/will.txt";
    ofstream willFile(willPath);
    willFile << willContent;
    willFile.close();

    // Generate AES key and IV
    unsigned char aesKey[16];
    unsigned char iv[16];
    generateAESKey(aesKey, 16);
    generateAESKey(iv, 16);

    vector<unsigned char> encryptedWill;
    if (!encryptAES(aesKey, iv, willContent, encryptedWill)) {
        cerr << "Error encrypting will." << endl;
        return 1;
    }

    // Save encrypted will
    string encWillPath = "./data/" + userAadhar + "/encrypted_will.txt";
    ofstream encWillFile(encWillPath, ios::binary);
    encWillFile.write((char *)encryptedWill.data(), encryptedWill.size());
    encWillFile.close();

    // Encrypt AES key with each beneficiary's RSA key and save
    for (size_t i = 0; i < beneficiaries.size(); i++) {
        vector<unsigned char> encryptedAESKey = encryptRSA(aesKey, 16, publicKeys[i]);
        string keyFilePath = "./data/" + userAadhar + "/relatives/" + aadhars[i] + "/encrypted_key.txt";
        ofstream keyFile(keyFilePath, ios::binary);
        keyFile.write((char *)encryptedAESKey.data(), encryptedAESKey.size());
        keyFile.close();
    }

    cout << "\nWill successfully created, stored, and encrypted." << endl;
    return 0;
}
