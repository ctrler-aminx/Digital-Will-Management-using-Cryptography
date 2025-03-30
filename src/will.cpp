#include "encryption.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring> // For memcpy

using namespace std;

const string AES_KEY_FILE = "keys/aes_key.txt";
const string AES_IV_FILE = "keys/aes_iv.txt";

// Function to retrieve beneficiaries from a file based on Aadhaar number
vector<string> getBeneficiaries(const string &aadhaarNumber) {
    vector<string> beneficiaries;
    string beneficiaryFile = "data/" + aadhaarNumber + "/beneficiaries.txt"; // Assuming a file stores this info
    
    ifstream file(beneficiaryFile);
    if (!file) {
        cerr << "Error: Could not open beneficiaries file for Aadhaar " << aadhaarNumber << endl;
        return beneficiaries; // Return empty vector if file is missing
    }

    string line;
    while (getline(file, line)) {
        beneficiaries.push_back(line);
    }

    file.close();
    return beneficiaries;
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

// Load data from a file
string loadFromFile(const string &filename) {
    ifstream inFile(filename, ios::binary);
    string data((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    
    if (!inFile.is_open()) {
        cerr << "Error: Unable to open file " << filename << endl;
    }

    return data;
}

// Create and encrypt the will
void createWill(const string &willData, const string &publicKeyFile) {
    string aadhaarNumber, userName;
    
    // Ask for Aadhaar Number and Name
    cout << "Enter your Aadhaar number: ";
    cin >> aadhaarNumber;
    cin.ignore();  // To clear newline character from input buffer
    cout << "Enter your full name: ";
    getline(cin, userName);

    // Display Beneficiaries (Assume getBeneficiaries() fetches a list)
    vector<string> beneficiaries = getBeneficiaries(aadhaarNumber);
    cout << "\n===== Beneficiaries =====\n";
    for (const string &b : beneficiaries) {
        cout << "- " << b << endl;
    }
    
    // Ask for Properties and their Assigned Beneficiaries
    cout << "\nEnter the list of properties and assign each to a beneficiary.\n";
    vector<pair<string, string>> propertyAllocations;
    while (true) {
        string property, beneficiary;
        cout << "Enter property name (or type 'done' to finish): ";
        getline(cin, property);
        if (property == "done") break;

        cout << "Assign this property to which beneficiary? ";
        getline(cin, beneficiary);

        propertyAllocations.push_back({property, beneficiary});
    }

    // Construct the will content
    string formattedWill = "Aadhaar: " + aadhaarNumber + "\n";
    formattedWill += "Name: " + userName + "\n\n";
    formattedWill += "Properties Distribution:\n";
    for (const auto &p : propertyAllocations) {
        formattedWill += "- " + p.first + " → " + p.second + "\n";
    }
    formattedWill += "\nWill Content:\n" + willData;  // Append original will content

    // Encryption process remains unchanged
    unsigned char aesKey[16], aesIv[16];
    if (!generateAESKeyAndIV(aesKey, aesIv)) {
        cerr << "Error: Failed to generate AES key and IV." << endl;
        return;
    }

    string encryptedWill = encryptAES(formattedWill, aesKey, aesIv);
    if (encryptedWill.empty()) {
        cerr << "Error: AES encryption failed or produced empty output." << endl;
        return;
    }
    saveToFile("will_encrypted.txt", encryptedWill);

    // Convert AES key & IV to strings for RSA encryption
    string aesKeyStr(aesKey, aesKey + 16);
    string aesIvStr(aesIv, aesIv + 16);

    // Encrypt and save AES key & IV using the public key
    string encryptedKey = encryptWithPublicKey(aesKeyStr, publicKeyFile);
    if (encryptedKey.empty()) {
        cerr << "Error: RSA encryption of AES key failed." << endl;
        return;
    }
    saveToFile(AES_KEY_FILE, encryptedKey);

    string encryptedIv = encryptWithPublicKey(aesIvStr, publicKeyFile);
    if (encryptedIv.empty()) {
        cerr << "Error: RSA encryption of AES IV failed." << endl;
        return;
    }
    saveToFile(AES_IV_FILE, encryptedIv);

    cout << "Will created and encrypted successfully." << endl;
}


// Read and decrypt the will
string readWill(const string &privateKeyFile) {
    // Load and decrypt AES key
    string encryptedKey = loadFromFile(AES_KEY_FILE);
    string aesKeyStr = decryptWithPrivateKey(encryptedKey, privateKeyFile);
    if (aesKeyStr.empty()) {
        cerr << "Error: Failed to decrypt AES key." << endl;
        return "";
    }

    // Load and decrypt AES IV
    string encryptedIv = loadFromFile(AES_IV_FILE);
    string aesIvStr = decryptWithPrivateKey(encryptedIv, privateKeyFile);
    if (aesIvStr.empty()) {
        cerr << "Error: Failed to decrypt AES IV." << endl;
        return "";
    }

    if (aesKeyStr.size() < 16 || aesIvStr.size() < 16) {
        cerr << "Error: Invalid AES key or IV size." << endl;
        return "";
    }

    unsigned char aesKey[16];
    unsigned char aesIv[16];
    memcpy(aesKey, aesKeyStr.c_str(), 16);
    memcpy(aesIv, aesIvStr.c_str(), 16);

    // Load and decrypt will
    string encryptedWill = loadFromFile("will_encrypted.txt");
    string decryptedWill = decryptAES(encryptedWill, aesKey, aesIv);
    if (decryptedWill.empty()) {
        cerr << "Error: Failed to decrypt the will." << endl;
        return "";
    }

    return decryptedWill;
}

int main() {
    string choice;
    cout << "Do you want to create or read the will? (create/read): ";
    cin >> choice;
    cin.ignore();

    if (choice == "create") {
        string willData, publicKeyFile;
        cout << "Enter the will data: ";
        getline(cin, willData);
        cout << "Enter public key file: ";
        cin >> publicKeyFile;
        createWill(willData, publicKeyFile);
    } else if (choice == "read") {
        string privateKeyFile;
        cout << "Enter private key file: ";
        cin >> privateKeyFile;
        string willContent = readWill(privateKeyFile);
        if (!willContent.empty()) {
            cout << "Decrypted Will Content: \n" << willContent << endl;
        }
    } else {
        cout << "Invalid choice. Please enter 'create' or 'read'." << endl;
    }

    return 0;
}
