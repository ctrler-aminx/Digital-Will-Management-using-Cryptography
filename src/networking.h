#ifndef NETWORKING_H
#define NETWORKING_H

#include <string>

void startServer(int port);
void connectToServer(const std::string &serverIP, int port);

// Added missing function declaration
bool requestPublicKeyFromCA(const std::string& aadhar, std::string& publicKey);

#endif // NETWORKING_H
