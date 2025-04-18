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


bool directoryExists(const string& path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}


void createUserDirectory(const string& path) {
    if (!directoryExists(path)) {
        string command = "mkdir -p " + path;
        system(command.c_str());
    }
}


bool getPublicKeyFromCA(const string& aadhar, string& publicKey) {
    if (!requestPublicKeyFromCA(aadhar, publicKey)) {
        cerr << "CA connection failed. Could not retrieve public key.\n";
        return false;
    }
    return true;
}


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

    string hashedAadhar = sha256(aadhar);
    string signedAadhar = signData(hashedAadhar, "keys/private.pem");

    
    string encodedSignedAadhar = base64Encode(signedAadhar);

    string publicKey;
    if (!getPublicKeyFromCA(aadhar, publicKey)) {
        cout << "Registration failed: Could not fetch public key from CA.\n";
        return; // Don't proceed if CA connection fails
    }

    
    createUserDirectory(userDir);

    
    string encodedPublicKey = base64Encode(publicKey);


    ofstream userFile(userDir + "/user.txt");
    if (!userFile) {
        cerr << "Error: Could not create user file.\n";
        return;
    }
    userFile << name << "|" << aadhar << "|" << encodedSignedAadhar << "\n";
    userFile.close();

    
    ofstream hashFile(userDir + "/hashed_aadhar.txt");
    if (!hashFile) {
        cerr << "Error: Could not create hash file.\n";
        return;
    }
    hashFile << hashedAadhar;
    hashFile.close();

    // Store public key for future use
    ofstream pubFile(userDir + "/public_key.txt");
    if (!pubFile) {
        cerr << "Error: Could not create public key file.\n";
        return;
    }
    pubFile << aadhar << "|" << encodedPublicKey << "\n";
    pubFile.close();

    cout << name << ", you are registered successfully!\nYour Aadhar number is: " << aadhar << endl;
    cout << "Public key retrieved and stored successfully. Your Public key: \n";
    cout << publicKey << endl;

string statusFile = "data/" + aadhar + "/status.txt";
ofstream statusStream(statusFile);
if (statusStream) {
    statusStream << "ALIVE";
    statusStream.close();
} else {
    cout << "[ERROR] Could not create status file for testator!\n";
}



}


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

    string statusFile = userDir + "/status.txt";
    ifstream statusStream(statusFile);
    if (!statusStream) {
        cout << "[ERROR] Status file missing! Cannot verify user status.\n";
        return;
    }

    string status;
    getline(statusStream, status);
    statusStream.close();

    if (status == "DECEASED") {
        cout << "Permission Denied! The testator is deceased. You cannot log in.\n";
        return;
    }

    
    string hashedAadhar = sha256(aadhar);

    // Read stored hashed Aadhar from file
    ifstream hashFile(userDir + "/hashed_aadhar.txt");
    if (!hashFile) {
        cerr << "Error: Could not read hash file.\n";
        return;
    }

    string storedHashedAadhar;
    getline(hashFile, storedHashedAadhar);
    hashFile.close();

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

    // ======= Check if the testator is deceased before proceeding =======
    string statusFile = "data/" + testatorAadhar + "/status.txt";
    ifstream statusStream(statusFile);

    if (!statusStream) {
        cout << "[ERROR] Testator's status file not found! Cannot proceed.\n";
        return;
    }

    string status;
    getline(statusStream, status);
    statusStream.close();

    if (status != "DECEASED") {
        cout << "Access Denied: The testator is still alive. You cannot access the will yet.\n";
        return;
    }

    // ======= Check if relative is registered =======
    string relativeDir = "data/" + testatorAadhar + "/relatives/" + relativeAadhar;
    string hashedRelativePath = relativeDir + "/hashed_aadhar.txt";

    ifstream relativeFile(hashedRelativePath);
    if (!relativeFile) {
        cout << "Relative login failed! No such relative registered for this testator.\n";
        return;
    }

    string storedHashedAadhar;
    getline(relativeFile, storedHashedAadhar);
    relativeFile.close();

    // Hash the entered relative Aadhar
    string hashedRelativeAadhar = sha256(relativeAadhar);

    if (hashedRelativeAadhar == storedHashedAadhar) {
        cout << "Login successful! Welcome, Relative " << relativeAadhar << "\n";

        // ===== Launch home.cpp after successful relative login =====
        cout << "Redirecting to Relative Home Page...\n";
        string command = "./bin/home " + relativeAadhar + " relative";
        system(command.c_str());
    } else {
        cout << "Relative login failed! Incorrect credentials.\n";
    }
}


// ======== Main Function ========
int main() {
    // Check if keys already exist before regenerating them
    if (!directoryExists("keys")) {
        createUserDirectory("keys");
    }
    if (!directoryExists("keys/private.pem") || !directoryExists("keys/public.pem")) {
        generateRSAKeyPair("keys/private.pem", "keys/public.pem");
    }

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
