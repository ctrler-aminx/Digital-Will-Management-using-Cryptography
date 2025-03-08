#include "networking.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

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
