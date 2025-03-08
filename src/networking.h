#ifndef NETWORKING_H
#define NETWORKING_H

#include <string>

void startServer(int port);
void connectToServer(const std::string &serverIP, int port);

#endif
