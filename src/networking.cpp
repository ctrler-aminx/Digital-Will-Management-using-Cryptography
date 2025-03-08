#include "networking.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
using namespace std;

// Start server on given port
void startServer(int port) {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == -1) {
        cerr << "Error creating socket" << endl;
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Bind failed" << endl;
        return;
    }

    listen(serverSock, 3);
    cout << "Server listening on port " << port << endl;

    int clientSock;
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSock < 0) {
        cerr << "Client connection failed" << endl;
        return;
    }

    char buffer[1024] = {0};
    read(clientSock, buffer, sizeof(buffer));
    cout << "Received: " << buffer << endl;

    close(clientSock);
    close(serverSock);
}

// Connect to server
void connectToServer(const string &serverIP, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        cerr << "Error creating socket" << endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed!" << endl;
        return;
    }

    string message = "Requesting identity verification";
    send(sock, message.c_str(), message.size(), 0);
    cout << "Verification request sent to CA." << endl;
    close(sock);
}

// Request public key from CA
bool requestPublicKeyFromCA(const string& aadhar, string& publicKey) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        cerr << "Error creating socket." << endl;
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8081); // CA Server Port
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection to CA failed." << endl;
        close(sock);
        return false;
    }

    send(sock, aadhar.c_str(), aadhar.size(), 0);

    char buffer[1024] = {0};
    recv(sock, buffer, sizeof(buffer), 0);
    publicKey = string(buffer);

    close(sock);

    if (!publicKey.empty()) {
        cout << "Public Key received from CA: " << publicKey << "\n";
        return true;
    } else {
        cerr << "Failed to retrieve public key from CA." << endl;
        return false;
    }
}
