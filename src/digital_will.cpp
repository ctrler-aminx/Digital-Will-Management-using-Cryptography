#include "encryption.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h> // For mkdir() and checking directories
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include "networking.h"

using namespace std;

// ======== Helper function to check if directory exists ========
bool directoryExists(const string& path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

// ======== Helper function to create user directory ========
void createUserDirectory(const string& path) {
    string command = "mkdir -p " + path;
    system(command.c_str());
}

// ======== Function to connect to CA and fetch public key ========
bool getPublicKeyFromCA(const string& aadhar, string& publicKey) {
    if (!requestPublicKeyFromCA(aadhar, publicKey)) {
        cout << "CA connection failed. Could not retrieve public key.\n";
        return false;
    }
    return true;
}

// ======== Function to register a new user ========
void registerUser() {
    string name, aadhar, relativeAadhar, relativeName;
    cout << "\n======= Digital Will Registration =======\n";
    cout << "Enter Name: ";
    getline(cin, name);
    cout << "Enter Aadhar Number (12 digits): ";
    getline(cin, aadhar);

    string userDir = "data/" + aadhar;

    // Check if user directory already exists
    if (directoryExists(userDir)) {
        cout << "User with Aadhar number " << aadhar << " is already registered!\n";
        return;
    }

    // Hash and sign Aadhar number for secure verification
    string hashedAadhar = sha256(aadhar);
    string signedAadhar = signData(hashedAadhar, "keys/private.pem");

    // Encode signed data using Base64 before storing
    string encodedSignedAadhar = base64Encode(signedAadhar);

    string publicKey;
    if (!getPublicKeyFromCA(aadhar, publicKey)) {
        cout << "Registration failed: Could not fetch public key from CA.\n";
        return; // Don't proceed if CA connection fails
    }

    // ===== Move directory creation and file writing here =====
    createUserDirectory(userDir);

    // Encode the public key using Base64 before storing
    string encodedPublicKey = base64Encode(publicKey);

    // Store user data securely in the user's directory
    ofstream userFile(userDir + "/user.txt");
    userFile << name << "|" << aadhar << "|" << encodedSignedAadhar << "\n";
    userFile.close();

    // Store hashed Aadhar for future login verification
    ofstream hashFile(userDir + "/hashed_aadhar.txt");
    hashFile << hashedAadhar;
    hashFile.close();

    // Store public key for future use
    ofstream pubFile(userDir + "/public_key.txt");
    pubFile << aadhar << "|" << encodedPublicKey << "\n";
    pubFile.close();

    cout << name << ", you are registered successfully!\nYour Aadhar number is: " << aadhar << endl;
    cout << "Public key retrieved and stored successfully. Your Public key: \n";
    cout << publicKey << endl;
}

// ======== Function to log in a user ========
void loginUser() {
    string aadhar;
    cout << "\n======= Digital Will Login =======\n";
    cout << "Enter Aadhar Number (12 digits): ";
    getline(cin, aadhar);

    string userDir = "data/" + aadhar;

    // Check if user directory exists
    if (!directoryExists(userDir)) {
        cout << "Login failed! User with Aadhar number " << aadhar << " is not registered.\n";
        return;
    }

    // Hash the Aadhar number for verification
    string hashedAadhar = sha256(aadhar);

    // Read stored hashed Aadhar from file
    ifstream hashFile(userDir + "/hashed_aadhar.txt");
    string storedHashedAadhar;
    getline(hashFile, storedHashedAadhar);

    if (storedHashedAadhar == hashedAadhar) {
        ifstream userFile(userDir + "/user.txt");
        string line, storedName, storedAadhar;
        getline(userFile, line);
        size_t pos1 = line.find("|");
        storedName = line.substr(0, pos1);
        cout << "Login successful! Welcome, " << storedName << "\n";

        // ===== Launch home.cpp after successful login =====
        cout << "Redirecting to Home Page...\n";
        string command = "./bin/home " + aadhar;
system(command.c_str());

    } else {
        cout << "Stored Hash: " << storedHashedAadhar << endl;
        cout << "Computed Hash: " << hashedAadhar << endl;
        cout << "Login failed! Incorrect Aadhar number.\n";
    }
}

// ======== Function to log in a relative ========
void loginRelative() {
    string testatorAadhar, relativeAadhar;
    cout << "\n======= Relative Login =======\n";
    cout << "Enter Testator's Aadhar Number: ";
    getline(cin, testatorAadhar);
    cout << "Enter Your Aadhar Number: ";
    getline(cin, relativeAadhar);

    string relativeDir = "data/" + testatorAadhar + "/relatives/" + relativeAadhar + ".txt";

    ifstream relativeFile(relativeDir);
    if (!relativeFile) {
        cout << "Relative login failed! No such relative registered for this testator.\n";
        return;
    }

    string line, storedName, storedAadhar;
    getline(relativeFile, line);
    size_t pos = line.find("|");
    storedName = line.substr(0, pos);
    storedAadhar = line.substr(pos + 1);

    // Hash and verify relative's Aadhar
    string hashedRelativeAadhar = sha256(relativeAadhar);
    string storedHashedAadhar = sha256(storedAadhar);

    if (hashedRelativeAadhar == storedHashedAadhar) {
        cout << "Login successful! Welcome, " << storedName << "\n";

        // ===== Launch home.cpp after successful login =====
        cout << "Redirecting to Home Page...\n";
        string command = "./bin/home " + relativeAadhar;
       system(command.c_str());

    } else {
        cout << "Relative login failed! Incorrect credentials.\n";
    }
}

// ======== Main Function ========
int main() {
    generateRSAKeyPair("keys/private.pem", "keys/public.pem");

    int choice;
    cout << "\n======= Digital Will Management System (DWMS) =======\n";
    cout << "1. Register User\n2. Login User\n3. Login Relative\n4. Exit\nEnter choice: ";
    cin >> choice;
    cin.ignore(); // To handle newlines after input

    if (choice == 1)
        registerUser();
    else if (choice == 2)
        loginUser();
    else if (choice == 3)
        loginRelative();
    else
        cout << "Exiting...\n";

    return 0;
}
