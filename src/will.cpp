/*#include "encryption.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <dirent.h>  // For directory operations
#include <sys/types.h>
#include <sys/stat.h> // For checking file existence
#include <cstring>    // For memcpy

using namespace std;

const string AES_KEY_FILE = "keys/aes_key.txt";
const string AES_IV_FILE = "keys/aes_iv.txt";

// Check if a directory exists
bool directoryExists(const string &path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

// Read content from a file
string readFromFile(const string &filePath) {
    ifstream file(filePath);
    if (!file) {
        return "";
    }
    return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

// Function to retrieve beneficiaries based on Aadhaar number
vector<string> getBeneficiaries(const string &aadhaarNumber) {
    vector<string> beneficiaries;
    string relativesDir = "data/" + aadhaarNumber + "/relatives/";

    DIR *dir = opendir(relativesDir.c_str());
    if (!dir) {
        cerr << "No registered beneficiaries found for Aadhaar: " << aadhaarNumber << endl;
        return beneficiaries;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name != "." && name != "..") {  // Ignore hidden/system entries
            beneficiaries.push_back(name);
        }
    }

    closedir(dir);
    return beneficiaries;
}

// Display testator's details
void displayTestatorDetails(const string &aadhaarNumber) {
    string testatorDir = "data/" + aadhaarNumber + "/";

    // Check if user exists
    if (!directoryExists(testatorDir)) {
        cerr << "Testator with Aadhaar " << aadhaarNumber << " not found.\n";
        exit(1);
    }

    // Read user details
    string userInfo = readFromFile(testatorDir + "user.txt");
    string hashedAadhaar = readFromFile(testatorDir + "hashed_aadhar.txt");
    string publicKey = readFromFile(testatorDir + "public_key.txt");

    cout << "\n===== Testator Details =====\n";
    cout << "Aadhaar Number: " << aadhaarNumber << endl;
    if (!userInfo.empty()) cout << "User Info: " << userInfo << endl;
    if (!hashedAadhaar.empty()) cout << "Hashed Aadhaar: " << hashedAadhaar << endl;
    if (!publicKey.empty()) cout << "Public Key: " << publicKey << endl;

    // Display registered beneficiaries
    vector<string> beneficiaries = getBeneficiaries(aadhaarNumber);
    cout << "\n===== Beneficiaries =====\n";
    if (beneficiaries.empty()) {
        cout << "No beneficiaries found.\n";
    } else {
        for (size_t i = 0; i < beneficiaries.size(); i++) {
            cout << i + 1 << ". " << beneficiaries[i] << endl;
        }
    }
}

// Save data to a file
void saveToFile(const string &filename, const string &data) {
    ofstream outFile(filename, ios::binary);
    if (outFile.is_open()) {
        outFile << data;
        outFile.close();
    } else {
        cerr << "Error: Unable to open file " << filename << endl;
    }
}

/void createWill() {
    string aadhaarNumber;

    // Ask for Aadhaar Number
    cout << "Enter your Aadhaar number: ";
    cin >> aadhaarNumber;
    cin.ignore();

    // Display testator details
    displayTestatorDetails(aadhaarNumber);

    // Ask for number of properties
    int numProperties;
    cout << "\nEnter the number of properties you have: ";
    cin >> numProperties;
    cin.ignore();

    vector<pair<string, string>> propertyAllocations;
    vector<string> beneficiaries = getBeneficiaries(aadhaarNumber);

    for (int i = 0; i < numProperties; i++) {
        string property;
        cout << "\nEnter name of property " << (i + 1) << ": ";
        getline(cin, property);

        if (beneficiaries.empty()) {
            cerr << "No beneficiaries found. Property cannot be allocated.\n";
            continue;
        }

        // Display beneficiary options
        cout << "Select a beneficiary for this property:\n";
        for (size_t j = 0; j < beneficiaries.size(); j++) {
            cout << "(" << j + 1 << ") " << beneficiaries[j] << endl;
        }

        // Choose a beneficiary
        int choice;
        cout << "Enter choice (1 to " << beneficiaries.size() << "): ";
        cin >> choice;
        cin.ignore();

        if (choice >= 1 && choice <= static_cast<int>(beneficiaries.size())) {
            propertyAllocations.push_back({property, beneficiaries[choice - 1]});
        } else {
            cerr << "Invalid choice. Skipping property allocation.\n";
        }
    }

    // Construct the will content
    string formattedWill = "Aadhaar: " + aadhaarNumber + "\n\nProperties Distribution:\n";
    for (const auto &p : propertyAllocations) {
        formattedWill += "- " + p.first + " → " + p.second + "\n";
    }

    cout << "\nEnter additional will content: ";
    cin.ignore(); // Ensure buffer is cleared before taking input
    string willData;
    getline(cin, willData);
    formattedWill += "\nWill Content:\n" + willData;

    // Encryption process
    unsigned char aesKey[16], aesIv[16];
    if (!generateAESKeyAndIV(aesKey, aesIv)) {
        cerr << "Error: Failed to generate AES key and IV." << endl;
        return;
    }

    string encryptedWill = encryptAES(formattedWill, aesKey, aesIv);
    if (encryptedWill.empty()) {
        cerr << "Error: AES encryption failed." << endl;
        return;
    }

    string userDir = "data/" + aadhaarNumber + "/";
    string keysDir = userDir + "keys/";

    saveToFile(userDir + "will_encrypted.txt", encryptedWill);

    // Store AES key for testator (Encrypt it using testator's public key)
    string testatorKeyFile = userDir + "public_key.txt";
    string aesKeyStr(aesKey, aesKey + 16);
    string aesIvStr(aesIv, aesIv + 16);

    string encryptedTestatorKey = encryptWithPublicKey(aesKeyStr, testatorKeyFile);
    if (encryptedTestatorKey.empty()) {
        cerr << "Error: RSA encryption of AES key failed for testator." << endl;
    } else {
        saveToFile(keysDir + "aes_key.txt", encryptedTestatorKey);
    }

    string encryptedTestatorIv = encryptWithPublicKey(aesIvStr, testatorKeyFile);
    if (encryptedTestatorIv.empty()) {
        cerr << "Error: RSA encryption of AES IV failed for testator." << endl;
    } else {
        saveToFile(keysDir + "aes_iv.txt", encryptedTestatorIv);
    }

    // Encrypt AES key for each beneficiary
    for (const string &beneficiary : beneficiaries) {
        string beneficiaryKeyFile = userDir + "relatives/" + beneficiary + "/public_key.txt";

        string encryptedKey = encryptWithPublicKey(aesKeyStr, beneficiaryKeyFile);
        if (encryptedKey.empty()) {
            cerr << "Error: RSA encryption of AES key failed for " << beneficiary << endl;
            continue;
        }
        saveToFile(keysDir + "aes_key_" + beneficiary + ".txt", encryptedKey);

        string encryptedIv = encryptWithPublicKey(aesIvStr, beneficiaryKeyFile);
        if (encryptedIv.empty()) {
            cerr << "Error: RSA encryption of AES IV failed for " << beneficiary << endl;
            continue;
        }
        saveToFile(keysDir + "aes_iv_" + beneficiary + ".txt", encryptedIv);
    }

    cout << "Will created and encrypted successfully.\n";
}

int main() {
    createWill();
    return 0;
}


==========================*/
#include "encryption.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>  // For directory operations
#include <sys/types.h>
#include <sys/stat.h> // For checking file existence
#include <cstring>    // For memcpy

using namespace std;

const string AES_KEY_FILE = "keys/aes_key.txt";
const string AES_IV_FILE = "keys/aes_iv.txt";

// Check if a directory exists
bool directoryExists(const string &path) {
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

// Read content from a file
string readFromFile(const string &filePath) {
    ifstream file(filePath);
    if (!file) {
        return "";
    }
    return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

// Extract Public Key from File (Ignore Everything Before Second '|')
string extractPublicKey(const string &filePath) {
    string fileContent = readFromFile(filePath);
    if (fileContent.empty()) {
        cerr << "Error: Public key file is empty or not found." << endl;
        return "";
    }

    // Find the position of the second '|'
    size_t firstPipe = fileContent.find('|');
    if (firstPipe == string::npos) {
        cerr << "Error: Invalid public key format (missing '|')." << endl;
        return "";
    }

    size_t secondPipe = fileContent.find('|', firstPipe + 1);
    if (secondPipe == string::npos) {
        cerr << "Error: Invalid public key format (missing second '|')." << endl;
        return "";
    }

    // Extract only the Base64-encoded key (everything after the second '|')
    string base64Key = fileContent.substr(secondPipe + 1);

    // Trim extra spaces or newlines
    base64Key.erase(remove(base64Key.begin(), base64Key.end(), '\n'), base64Key.end());
    base64Key.erase(remove(base64Key.begin(), base64Key.end(), '\r'), base64Key.end());

    return base64Key;
}


// Function to retrieve beneficiaries based on Aadhaar number
vector<string> getBeneficiaries(const string &aadhaarNumber) {
    vector<string> beneficiaries;
    string relativesDir = "data/" + aadhaarNumber + "/relatives/";

    DIR *dir = opendir(relativesDir.c_str());
    if (!dir) {
        cerr << "No registered beneficiaries found for Aadhaar: " << aadhaarNumber << endl;
        return beneficiaries;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name != "." && name != "..") {  // Ignore hidden/system entries
            beneficiaries.push_back(name);
        }
    }

    closedir(dir);
    return beneficiaries;
}

// Display testator's details
void displayTestatorDetails(const string &aadhaarNumber) {
    string testatorDir = "data/" + aadhaarNumber + "/";

    // Check if user exists
    if (!directoryExists(testatorDir)) {
        cerr << "Testator with Aadhaar " << aadhaarNumber << " not found.\n";
        exit(1);
    }

    // Read user details
    string userInfo = readFromFile(testatorDir + "user.txt");
    string hashedAadhaar = readFromFile(testatorDir + "hashed_aadhar.txt");
    string publicKey = extractPublicKey(testatorDir + "public_key.txt");

    cout << "\n===== Testator Details =====\n";
    cout << "Aadhaar Number: " << aadhaarNumber << endl;
    if (!userInfo.empty()) cout << "User Info: " << userInfo << endl;
    if (!hashedAadhaar.empty()) cout << "Hashed Aadhaar: " << hashedAadhaar << endl;
    if (!publicKey.empty()) cout << "Public Key: " << publicKey << endl;

    // Display registered beneficiaries
    vector<string> beneficiaries = getBeneficiaries(aadhaarNumber);
    cout << "\n===== Beneficiaries =====\n";
    if (beneficiaries.empty()) {
        cout << "No beneficiaries found.\n";
    } else {
        for (size_t i = 0; i < beneficiaries.size(); i++) {
            cout << i + 1 << ". " << beneficiaries[i] << endl;
        }
    }
}

// Save data to a file
void saveToFile(const string &filename, const string &data) {
    ofstream outFile(filename, ios::binary);
    if (outFile.is_open()) {
        outFile << data;
        outFile.close();
    } else {
        cerr << "Error: Unable to open file " << filename << endl;
    }
}

// Base64 encode a string
string base64EncodeString(const string &data) {
    return base64Encode(data);
}

void createWill() {
    string aadhaarNumber;

    // Ask for Aadhaar Number
    cout << "Enter your Aadhaar number: ";
    cin >> aadhaarNumber;
    cin.ignore();

    // Display testator details
    displayTestatorDetails(aadhaarNumber);

    // Ask for number of properties
    int numProperties;
    cout << "\nEnter the number of properties you have: ";
    cin >> numProperties;
    cin.ignore();

    vector<pair<string, string>> propertyAllocations;
    vector<pair<string, string>> beneficiaries; // Stores {name, public_key}
    string relativesDir = "data/" + aadhaarNumber + "/relatives/";

    DIR *dir = opendir(relativesDir.c_str());
    if (!dir) {
        cerr << "No registered beneficiaries found for Aadhaar: " << aadhaarNumber << endl;
    } else {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name != "." && name != "..") {  
                string publicKeyPath = relativesDir + name + "/public_key.txt";
                string publicKey = extractPublicKey(publicKeyPath);
                if (!publicKey.empty()) {
                    beneficiaries.push_back({name, publicKey});
                } else {
                    cerr << "Warning: No valid public key found for " << name << endl;
                }
            }
        }
        closedir(dir);
    }

    for (int i = 0; i < numProperties; i++) {
        string property;
        cout << "\nEnter name of property " << (i + 1) << ": ";
        getline(cin, property);

        if (beneficiaries.empty()) {
            cerr << "No beneficiaries found. Property cannot be allocated.\n";
            continue;
        }

        // Display beneficiary options
        cout << "Select a beneficiary for this property:\n";
        for (size_t j = 0; j < beneficiaries.size(); j++) {
            cout << "(" << j + 1 << ") " << beneficiaries[j].first << endl;
        }

        // Choose a beneficiary
        int choice;
        cout << "Enter choice (1 to " << beneficiaries.size() << "): ";
        cin >> choice;
        cin.ignore();

        if (choice >= 1 && choice <= static_cast<int>(beneficiaries.size())) {
            propertyAllocations.push_back({property, beneficiaries[choice - 1].first});
        } else {
            cerr << "Invalid choice. Skipping property allocation.\n";
        }
    }

    // Construct the will content
    string formattedWill = "Aadhaar: " + aadhaarNumber + "\n\nProperties Distribution:\n";
    for (const auto &p : propertyAllocations) {
        formattedWill += "- " + p.first + " \u2192 " + p.second + "\n"; // Unicode arrow
    }

    cout << "\nEnter additional will content: ";
    cin.ignore(); // Ensure buffer is cleared before taking input
    string willData;
    getline(cin, willData);
    formattedWill += "\nWill Content:\n" + willData;

    string willPath = "data/" + aadhaarNumber + "/will.txt";
    saveToFile(willPath, formattedWill);
    cout << "Will saved at: " << willPath << endl;

    cout << "Will created successfully.\n";

    // AES Encryption
    unsigned char aesKey[32]; // 256-bit AES key
    unsigned char aesIv[16];  // 128-bit IV
    generateAESKeyAndIV(aesKey, aesIv); // Use the correct function
    string encryptedWill = encryptAES(formattedWill, aesKey, aesIv);

    // Save encrypted will, key, and IV
    saveToFile("data/" + aadhaarNumber + "/encrypted_will.txt", encryptedWill);
    saveToFile("data/" + aadhaarNumber + "/aes_key.txt", string(reinterpret_cast<char*>(aesKey), 32));
    saveToFile("data/" + aadhaarNumber + "/aes_iv.txt", string(reinterpret_cast<char*>(aesIv), 16));

    cout << "Encrypted will saved successfully.\n";
}
int main() {
    createWill();
    return 0;
}
