#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <algorithm>

using namespace std;

// Retrieve public key from ca_data.txt for a valid user
string getPublicKeyForAadhar(const string &hashedAadhar) {
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

// Start CA server on port 8081 with support for multiple clients
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
    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Bind failed!\n";
        close(serverSock);
        return;
    }

    // Set socket to listen for connections
    listen(serverSock, 5);
    cout << "CA Server listening on port " << port << "...\n";

    // Set server socket to non-blocking mode
    fcntl(serverSock, F_SETFL, O_NONBLOCK);

    fd_set readfds;
    vector<int> clientSockets; // Store multiple client sockets

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(serverSock, &readfds);
        int maxSock = serverSock;

        // Add existing client sockets to set
        for (int clientSock : clientSockets) {
            FD_SET(clientSock, &readfds);
            if (clientSock > maxSock) {
                maxSock = clientSock;
            }
        }

        // Wait for activity on sockets
        int activity = select(maxSock + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            cerr << "Select error!\n";
            break;
        }

        // Handle new connection
        if (FD_ISSET(serverSock, &readfds)) {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int newSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
            if (newSock >= 0) {
                cout << "New client connected!\n";
                clientSockets.push_back(newSock);
            }
        }

        // Handle data from clients
        vector<int> disconnectedSockets;
        for (int clientSock : clientSockets) {
            if (FD_ISSET(clientSock, &readfds)) {
                char buffer[1024] = {0};
                int bytesRead = read(clientSock, buffer, sizeof(buffer));

                if (bytesRead == 0) {
                    // Client disconnected
                    cout << "Client disconnected.\n";
                    disconnectedSockets.push_back(clientSock);
                    close(clientSock);
                } else {
                    // Get the public key if user is verified
                    string hashedAadhar(buffer);
                    string publicKey = getPublicKeyForAadhar(hashedAadhar);

                    if (!publicKey.empty()) {
                        // Send public key if user is verified
                        cout << "Public key requested for " << hashedAadhar << ":\n";
                        cout << publicKey;
                        send(clientSock, publicKey.c_str(), publicKey.size(), 0);
                    } else {
                        // Send "Not Verified" if user not found
                        string response = "Not Verified";
                        send(clientSock, response.c_str(), response.size(), 0);
                    }
                }
            }
        }

        // Remove disconnected clients from the list
        for (int sock : disconnectedSockets) {
            clientSockets.erase(remove(clientSockets.begin(), clientSockets.end(), sock), clientSockets.end());
        }
    }

    // Close all sockets before exiting
    for (int clientSock : clientSockets) {
        close(clientSock);
    }
    close(serverSock);
}

int main() {
    startCA(8081); // Use port 8081 as per the system design
    return 0;
}
