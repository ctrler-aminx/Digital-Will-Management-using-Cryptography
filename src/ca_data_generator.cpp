#include <iostream>
#include <fstream>
#include <string>
#include "encryption.h"

using namespace std;

void generateCAData() {
    ofstream file("ca/ca_data.txt", ios::app); // Append mode to avoid overwriting existing data
    if (!file) {
        cerr << "Error creating CA data file!\n";
        return;
    }

    // Expanded list with diverse names
    string names[100] = {
        "Aarav", "Ananya", "Rahul", "Ishita", "Kavya", "Alice", "Ethan", "Liam", "Sophia", "Mason",
        "Hiro", "Yuna", "Fatima", "Omar", "Zara", "Miguel", "Elena", "Lucia", "Jamal", "Layla",
        "Chen", "Mei", "Yusuf", "Amira", "Santiago", "Gabriela", "Dimitri", "Natasha", "Aria", "Felix",
        "Javier", "Isla", "Diego", "Freya", "Abdul", "Hina", "Ali", "Nina", "Malik", "Kim",
        "Jonas", "Nadia", "Ravi", "Vikram", "Sita", "Rohan", "Fatima", "Hassan", "Alina", "Elijah",
        "Noah", "Valentina", "Mateo", "Aisha", "Hana", "Tariq", "Amina", "Rami", "Mariam", "Eli",
        "Omar", "Sami", "Zane", "Tiana", "Diana", "Theo", "Arjun", "Naveen", "Rey", "Priya",
        "Krishna", "Veer", "Sai", "Tanu", "Aditya", "Dev", "Kiran", "Vani", "Ira", "Rhea",
        "Kabir", "Ritu", "Nisha", "Aditi", "Anika", "Akash", "Avni", "Zubin", "Shanaya", "Parth",
        "Aryan", "Harini", "Bhavya", "Gauri", "Yash", "Shaurya", "Mahika", "Rudra", "Tanvi", "Ishaan"
    };

    // Unique Aadhar numbers for 100 entries
    string aadharNumbers[100];
    for (int i = 0; i < 100; ++i) {
        aadharNumbers[i] = std::string("1234567890") + (i < 10 ? "0" : "") + to_string(i + 1);

    }

    // Generate keys for entries starting from 11 (to avoid duplicates)
    for (int i = 10; i < 100; ++i) {
        string privateKeyPath = "keys/private_" + to_string(i + 1) + ".pem";
        string publicKeyPath = "keys/public_" + to_string(i + 1) + ".pem";

        generateRSAKeyPair(privateKeyPath, publicKeyPath);  // Create new keys only for entries 11 onward

        ifstream pubKeyFile(publicKeyPath);
        string publicKey((istreambuf_iterator<char>(pubKeyFile)), istreambuf_iterator<char>());
        pubKeyFile.close();

        file << (i + 1) << "|" << names[i] << "|" << aadharNumbers[i] << "|" << publicKey << "\n";
    }

    file.close();
    cout << "100 sample entries created successfully in ca_data.txt" << endl;
}

int main() {
    generateCAData();
    return 0;
}




/*#include <iostream>
#include <fstream>
#include <string>
#include "encryption.h"

using namespace std;

void generateCAData() {
    ofstream file("ca/ca_data.txt");
    if (!file) {
        cerr << "Error creating CA data file!\n";
        return;
    }

    // Updated list with Indian and Foreign names
    string names[10] = {
        "Aarav", "Ananya", "Rahul", "Ishita", "Kavya", // Indian Names
        "Alice", "Ethan", "Liam", "Sophia", "Mason"     // Foreign Names
    };

    string aadharNumbers[10] = {
        "123456789001", "123456789002", "123456789003", "123456789004", "123456789005",
        "123456789006", "123456789007", "123456789008", "123456789009", "123456789010"
    };

    // Generate keys for 10 users
    for (int i = 0; i < 10; ++i) {
        string privateKeyPath = "keys/private_" + to_string(i + 1) + ".pem";
        string publicKeyPath = "keys/public_" + to_string(i + 1) + ".pem";

        generateRSAKeyPair(privateKeyPath, publicKeyPath);

        ifstream pubKeyFile(publicKeyPath);
        string publicKey((istreambuf_iterator<char>(pubKeyFile)), istreambuf_iterator<char>());
        pubKeyFile.close();

        file << (i + 1) << "|" << names[i] << "|" << aadharNumbers[i] << "|" << publicKey << "\n";
    }

    file.close();
    cout << "10 sample entries created successfully in ca_data.txt" << endl;
}

int main() {
    generateCAData();
    return 0;
}
*/