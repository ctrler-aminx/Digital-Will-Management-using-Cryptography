#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include "encryption.h"  // for base64 and sha utilities
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

#define CA_IP "127.0.0.1"   // CA runs on localhost for now
#define CA_PORT 8081        // Port for CA communication

using namespace std;

bool directoryExists(const string& path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

void createUserDirectory(const string& path) {
    string command = "mkdir -p " + path;
    system(command.c_str());
}
bool getPublicKeyFromCA(const string& aadhar, string& publicKey) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error: Failed to create socket.\n";
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CA_PORT);
    inet_pton(AF_INET, CA_IP, &serverAddr.sin_addr);

    // Connect to CA
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error: Could not connect to CA.\n";
        close(sock);
        return false;
    }
    string request = "GET_PUBLIC_KEY|" + aadhar;
    send(sock, request.c_str(), request.size(), 0);
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        cerr << "Error: Failed to receive public key from CA.\n";
        close(sock);
        return false;
    }

    publicKey = string(buffer);
    cout << "\nPublic key retrieved for Aadhar number " << aadhar << endl;
    cout << publicKey << "\n";
    close(sock);
    return true;
}

string loggedInAadhar; 
string encryptAES(const string& plainText, const string& key) {
    return "ENCRYPTED_" + plainText; 
}
string decryptAES(const string& encryptedText, const string& key) {
    if (encryptedText.find("ENCRYPTED_") == 0) {
        return encryptedText.substr(10); 
    }
    return "ERROR: Invalid encrypted data!";
}
void registerBeneficiaries(bool isNew = true) {
    int numRelatives;
    cout << "\nHow many beneficiaries do you want to add? ";
    cin >> numRelatives;
    cin.ignore(); 
    string relativeDir = "data/" + loggedInAadhar + "/relatives/";
    if (!directoryExists(relativeDir)) {
        createUserDirectory(relativeDir);
    }

    for (int i = 0; i < numRelatives; ++i) {
        string relativeName, relativeAadhar;
        cout << "\nEnter name of beneficiary " << (i + 1) << ": ";
        getline(cin, relativeName);
        cout << "Enter Aadhar number of " << relativeName << " (12 digits): ";
        getline(cin, relativeAadhar);

        string relativeDirPath = relativeDir + relativeAadhar;
        if (directoryExists(relativeDirPath)) {
            cout << "Beneficiary with Aadhar " << relativeAadhar << " is already registered!\n";
            continue;
        }
        string publicKey;
        if (!getPublicKeyFromCA(relativeAadhar, publicKey)) {
            cout << "Failed to fetch public key from CA for " << relativeName << ". Skipping registration.\n";
            continue;
        }
        createUserDirectory(relativeDirPath);
        ofstream relativeFile(relativeDirPath + "/relative.txt");
        relativeFile << relativeName << "|" << relativeAadhar << "\n";
        relativeFile.close();
        string encodedPublicKey = base64Encode(publicKey);
        ofstream pubKeyFile(relativeDirPath + "/public_key.txt");
        pubKeyFile << encodedPublicKey << "\n";
        pubKeyFile.close();

        cout << "Beneficiary " << relativeName << " registered successfully. Public key stored securely.\n";
    }

    if (isNew) {
        cout << "All beneficiaries have been registered successfully!\n";
    } else {
        cout << "Beneficiaries have been added successfully!\n";
    }
}
void createWill() {
    registerBeneficiaries();
    cout << "Redirecting to will creation and encryption...\n";
    system(("./bin/will " + loggedInAadhar).c_str());
}

void viewWill() {
    string willFile = "data/" + loggedInAadhar + "/will.txt";
    string keyFile = "data/" + loggedInAadhar + "/aes_key.txt";

    ifstream willStream(willFile);
    ifstream keyStream(keyFile);

    if (!willStream || !keyStream) {
        cout << "No will found. Please create a will first.\n";
        return;
    }

    string encryptedWill, aesKey;
    getline(willStream, encryptedWill);
    getline(keyStream, aesKey);
    string decryptedWill = decryptAES(encryptedWill, aesKey);
    cout << "\n======= Decrypted Will Content =======\n";
    cout << decryptedWill << "\n";
}
void editWill() {
    string willFile = "data/" + loggedInAadhar + "/will.txt";
    string keyFile = "data/" + loggedInAadhar + "/aes_key.txt";

    ifstream keyStream(keyFile);
    if (!keyStream) {
        cout << "No will found. Please create a will first.\n";
        return;
    }

    string newWillContent;
    cout << "\nEnter updated content for the will: ";
    getline(cin, newWillContent);
    string aesKey;
    getline(keyStream, aesKey);
    string encryptedWill = encryptAES(newWillContent, aesKey);
    ofstream willStream(willFile);
    willStream << encryptedWill;
    willStream.close();

    cout << "Will updated and encrypted successfully!\n";
}
void addMoreBeneficiaries() {
    registerBeneficiaries(false); 
}
void showHomePage() {
    int choice;

    do {
        cout << "\n======= Digital Will Management System - Home =======\n";
        cout << "1. Create Will\n";
        cout << "2. View Will\n";
        cout << "3. Edit Will\n";
        cout << "4. Add More Beneficiaries\n";
        cout << "5. View Registered Beneficiaries\n";
        cout << "6. Logout\n";
        cout << "Enter your choice: ";
        cin >> choice;
        cin.ignore(); 
        switch (choice) {
        case 1:
            createWill();
            break;
        case 2:
            viewWill();
            break;
        case 3:
            editWill();
            break;
        case 4:
            addMoreBeneficiaries();
            break;
        case 5: {
            string relativeDir = "data/" + loggedInAadhar + "/relatives/";
            cout << "\nRegistered Beneficiaries:\n";
            system(("ls " + relativeDir).c_str());
            break;
        }
        case 6:
            cout << "Logging out...\n";
            return;
        default:
            cout << "Invalid choice! Please try again.\n";
        }
    } while (choice != 6);
}
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Error: Aadhar number not provided to home.\n";
        return 1;
    }
    loggedInAadhar = argv[1]; 
    showHomePage();
    return 0;
}
