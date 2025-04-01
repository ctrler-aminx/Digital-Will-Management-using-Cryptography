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
#include <limits>
#include <unistd.h>
#include "networking.h"
#include <cstring>
#include <sys/stat.h>
string loggedInAadhar;
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
    if (!requestPublicKeyFromCA(aadhar, publicKey)) {
        cerr << "CA connection failed. Could not retrieve public key.\n";
        return false;
    }
    return true;
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

        string hashedAadhar = sha256(relativeAadhar);
        string signedAadhar = signData(hashedAadhar, "keys/private.pem");
        string encodedSignedAadhar = base64Encode(signedAadhar);

        string publicKey;
        if (!getPublicKeyFromCA(relativeAadhar, publicKey)) {
            cout << "Failed to fetch public key from CA for " << relativeName << ". Skipping registration.\n";
            continue;
        }

        createUserDirectory(relativeDirPath);

        // Store hashed Aadhar
        ofstream hashFile(relativeDirPath + "/hashed_aadhar.txt");
        if (!hashFile) {
            cerr << "Error: Could not create hash file.\n";
            return;
        }
        hashFile << hashedAadhar;
        hashFile.close();

        // Store public key with relative name and Aadhar
        string encodedPublicKey = base64Encode(publicKey);
        ofstream pubFile(relativeDirPath + "/public_key.txt");
        if (!pubFile) {
            cerr << "Error: Could not create public key file.\n";
            return;
        }
        pubFile << relativeName << "|" << relativeAadhar << "|" << encodedPublicKey << "\n";
        pubFile.close();

        cout << "Beneficiary " << relativeName << " registered successfully. Public key stored securely.\n" << endl;
        cout << "The Public Key received from CA of " << relativeAadhar << " is " << endl;
        cout << publicKey << endl;
    }
}


void createWill() {
    registerBeneficiaries();
    cout << "Redirecting to will creation and encryption...\n";
    system(("./bin/will " + loggedInAadhar).c_str());
}

void viewWill(bool isRelative = false) {
    string testatorAadhar = loggedInAadhar;  // Default to logged-in user (testator)

    if (isRelative) {
        cout << "Enter the Aadhar number of the testator whose will you want to view: ";
        cin >> testatorAadhar;
        cin.ignore();
    }

    string willFile = "data/" + testatorAadhar + "/encrypted_will.txt";
    string keyFile = "data/" + testatorAadhar + "/aes_key.txt";
    string ivFile = "data/" + testatorAadhar + "/aes_iv.txt";  

/*
    string willfile = "data/" + testatorAadhar + "/will.txt";
ifstream willstream(willfile);
if (willstream) {
    cout << "\n======= Plaintext Will Content =======\n";
    cout << string((istreambuf_iterator<char>(willstream)), istreambuf_iterator<char>()) << endl;
} else {
    cout << "No will found at: " << willfile << endl;
}
*/

    ifstream willStream(willFile);
    ifstream keyStream(keyFile);
    ifstream ivStream(ivFile);  

    if (!willStream || !keyStream || !ivStream) {  
        cout << "No will found for Aadhar: " << testatorAadhar << ". Please check the Aadhar number.\n";
        return;
    }

    string encryptedWill, aesKey, aesIv;
    stringstream buffer;
buffer << willStream.rdbuf();
encryptedWill = buffer.str();
    getline(keyStream, aesKey);
    getline(ivStream, aesIv);  

    string decryptedWill = decryptAES(encryptedWill, (const unsigned char*)aesKey.c_str(), (const unsigned char*)aesIv.c_str());
    

    cout << "\n======= Decrypted Will Content =======\n";
    cout << decryptedWill << "\n";
}


void editWill() {
    string willFile = "data/" + loggedInAadhar + "/will.txt";
    string keyFile = "data/" + loggedInAadhar + "/aes_key.txt";
    string ivFile = "data/" + loggedInAadhar + "/aes_iv.txt";

    cout << "[DEBUG] Opening AES key and IV files...\n";
    
    ifstream keyStream(keyFile);
    ifstream ivStream(ivFile);
    
    if (!keyStream || !ivStream) {
        cout << "No will found. Please create a will first.\n";
        return;
    }

    string aesKey, aesIv;
    getline(keyStream, aesKey);
    getline(ivStream, aesIv);

    keyStream.close();
    ivStream.close();

    cout << "[DEBUG] AES key and IV successfully loaded.\n";

    // Read existing will content
    string existingWillContent = "";
    ifstream willStream(willFile);
    if (willStream) {
        string encryptedWill((istreambuf_iterator<char>(willStream)), istreambuf_iterator<char>());
        willStream.close();

        if (!encryptedWill.empty()) {
            existingWillContent = decryptAES(encryptedWill, (const unsigned char*)aesKey.c_str(), (const unsigned char*)aesIv.c_str());

            if (existingWillContent.empty()) {
                cout << "[ERROR] Decryption failed! Existing will content could not be retrieved.\n";
            }
        }
    }

    cout << "[DEBUG] Successfully decrypted existing will content:\n" << existingWillContent << "\n";

    // Get user input for additional content
    string newWillContent, line;
    cout << "\nEnter updated content for the will (type 'DONE' on a new line to finish):\n";

    cin.ignore(numeric_limits<streamsize>::max(), '\n');  // Ensure buffer is cleared

    while (true) {
        getline(cin, line);
        if (line == "DONE") break;
        newWillContent += line + "\n"; 
    }

    cout << "[DEBUG] New user input received:\n" << newWillContent << "\n";

    // Ensure new content is actually appended
    string finalWillContent = existingWillContent + "\n" + newWillContent;

    cout << "[DEBUG] Final combined will content:\n" << finalWillContent << "\n";

    // Encrypt the combined will
    string encryptedWill = encryptAES(finalWillContent, (const unsigned char*)aesKey.c_str(), (const unsigned char*)aesIv.c_str());

    if (encryptedWill.empty()) {
        cout << "[ERROR] Encryption failed! Will content is empty after encryption.\n";
        return;
    }

    // Overwrite the file with the new encrypted will
    ofstream outFile(willFile, ios::trunc);  // Ensure we overwrite the file
    if (!outFile) {
        cerr << "[ERROR] Unable to write to the will file.\n";
        return;
    }
    outFile << encryptedWill;
    outFile.close();

    cout << "[DEBUG] Will successfully encrypted.\n";
    cout << "Will updated and encrypted successfully!\n";
}



void addMoreBeneficiaries() {
    registerBeneficiaries(false); 
}

void showRelativeHomePage() {
    int choice;
    do {
        cout << "\n======= Digital Will Management System - Relative Home =======\n";
        cout << "1. View Will\n";
        cout << "2. Logout\n";
        cout << "Enter your choice: ";
        cin >> choice;
        cin.ignore(); 

        switch (choice) {
        case 1:
            viewWill(true);  // Only allow viewing the will
            break;
        case 2:
            cout << "Logging out...\n";
            return;  // Exit relative mode
        default:
            cout << "Invalid choice! Please try again.\n";
        }
    } while (choice != 2);
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
    if (argc < 2 || argc > 3) {
        cout << "Error: Invalid number of arguments.\n";
        cout << "Usage: ./home <AadharNumber> [relative]\n";
        return 1;
    }

    loggedInAadhar = argv[1];  // Aadhar of logged-in user

    // Check if the third argument is 'relative'
    bool isRelative = (argc == 3 && string(argv[2]) == "relative");

    if (isRelative) {
        showRelativeHomePage();  // Redirect to relative's home page
    } else {
        showHomePage();  // Default to testator's home page
    }
    return 0;
}
