#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// Retrieve public key from ca_data.txt for a valid user
string getPublicKeyForAadhar(const string& hashedAadhar) {
    ifstream file("ca/ca_data.txt");
    if (!file) {
        cerr << "Error opening ca_data.txt!\n";
        return "";
    }

    string line, serial, name, aadhar, publicKey;
    bool keyStarted = false;
    string tempKey = "";

    while (getline(file, line)) {
        stringstream ss(line);
        getline(ss, serial, '|');
        getline(ss, name, '|');
        getline(ss, aadhar, '|');
        getline(ss, publicKey); // Read the rest after the public key starts

        if (aadhar == hashedAadhar) {
            // Check if the key starts with -----BEGIN PUBLIC KEY-----
            if (publicKey.find("-----BEGIN PUBLIC KEY-----") != string::npos) {
                keyStarted = true;
                tempKey += publicKey + "\n";

                // Read remaining lines of the public key until -----END PUBLIC KEY-----
                while (getline(file, line)) {
                    tempKey += line + "\n";
                    if (line.find("-----END PUBLIC KEY-----") != string::npos) {
                        keyStarted = false;
                        break;
                    }
                }
            } else {
                tempKey = publicKey; // Single-line key (fallback)
            }

            file.close();
            return tempKey;
        }
    }

    file.close();
    return ""; // No matching Aadhar found
}


// Start CA server on port 8081
void startCA(int port) {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == -1) {
        cerr << "Error creating socket!\n";
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Bind the socket to port 8081
    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Bind failed!\n";
        return;
    }

    listen(serverSock, 3);
    cout << "CA Server listening on port " << port << "...\n";

    // Accept incoming connections
    int clientSock;
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSock < 0) {
        cerr << "Client connection failed!\n";
        return;
    }

    char buffer[1024] = {0};
    read(clientSock, buffer, sizeof(buffer));

    // Get the public key if user is verified
    string hashedAadhar(buffer);
    string publicKey = getPublicKeyForAadhar(hashedAadhar);

    if (!publicKey.empty()) {
        // Send public key if user is verified
        cout<<"publicKey requested is of "<<hashedAadhar<<" is :"<<endl;
        cout<<publicKey;
        send(clientSock, publicKey.c_str(), publicKey.size(), 0);
    } else {
        // Send "Not Verified" if user not found
        string response = "Not Verified";
        send(clientSock, response.c_str(), response.size(), 0);
    }

    close(clientSock);
    close(serverSock);
}

int main() {
    startCA(8081); // Use port 8081 as per the system design
    return 0;
}


/*#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// Verify user by checking hashed Aadhar in ca_data.txt
bool verifyUser(const string &hashedAadhar) {
    ifstream file("ca/ca_data.txt");
    if (!file) {
        cerr << "Error opening ca_data.txt!\n";
        return false;
    }

    string line, serial, name, aadhar, publicKey;
    while (getline(file, line)) {
        stringstream ss(line);
        getline(ss, serial, '|');
        getline(ss, name, '|');
        getline(ss, aadhar, '|');
        getline(ss, publicKey, '|');

        // Compare hashed Aadhar with the one from CA data
        if (aadhar == hashedAadhar) {
            cout << "User verified: " << name << " (" << aadhar << ")\n";
            file.close();
            return true;
        }
    }

    file.close();
    return false; // No match found
}

// Start CA server on port 8081
void startCA(int port) {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == -1) {
        cerr << "Error creating socket!\n";
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Bind the socket to port 8081
    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Bind failed!\n";
        return;
    }

    listen(serverSock, 3);
    cout << "CA Server listening on port " << port << "...\n";

    // Accept incoming connections
    int clientSock;
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSock < 0) {
        cerr << "Client connection failed!\n";
        return;
    }

    char buffer[1024] = {0};
    read(clientSock, buffer, sizeof(buffer));

    // Check if user is verified
    string hashedAadhar(buffer);
    string response = verifyUser(hashedAadhar) ? "Verified" : "Not Verified";

    // Send response to client
    send(clientSock, response.c_str(), response.size(), 0);

    close(clientSock);
    close(serverSock);
}

int main() {
    startCA(8081); // Use port 8081 as per the system design
    return 0;
}
*/