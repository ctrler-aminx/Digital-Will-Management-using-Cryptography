#include <iostream>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

bool verifyUser(const string &hashedAadhar) {
    return hashedAadhar == "hashed_aadhar_1" || hashedAadhar == "hashed_aadhar_2";
}

void startCA(int port) {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSock, 3);
    
    int clientSock = accept(serverSock, nullptr, nullptr);
    char buffer[1024] = {0};
    read(clientSock, buffer, sizeof(buffer));

    string response = verifyUser(buffer) ? "Verified" : "Not Verified";
    send(clientSock, response.c_str(), response.size(), 0);

    close(clientSock);
    close(serverSock);
}

int main() {
    startCA(8080);
    return 0;
}

