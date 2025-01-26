#include "common.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <cstring>
#include <cstdlib> 

void receiveMessages(int socket) {
    char buffer[1024] = {0};
    while (true) {
        int valread = read(socket, buffer, sizeof(buffer) - 1);
        if (valread > 0) {
            buffer[valread] = '\0';
            std::cout << buffer << std::endl;
        } else if (valread == 0) {
            std::cerr << "Server disconnected." << std::endl;
            break;
        } else {
            perror("Error reading from server");
            break;
        }
    }
}

int main(int argc, char* argv[]) {
 
    if (argc != 2) {
        std::cerr << "Usage: ./client <server_ip>" << std::endl;
        return 1;
    }

    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[1024] = {0};

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address format" << std::endl;
        close(clientSocket);
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        perror("Connection to server failed");
        close(clientSocket);
        exit(1);
    }

    std::cout << "Connected to the server!" << std::endl;

    std::thread recvThread(receiveMessages, clientSocket);
    recvThread.detach();

    while (true) {
        std::string message;
        std::getline(std::cin, message);

        if (message == "exit") {
            std::cout << "Disconnecting..." << std::endl;
            break;
        }

        if (send(clientSocket, message.c_str(), message.size(), 0) == -1) {
            perror("Error sending message");
            break;
        }
    }

    close(clientSocket);
    return 0;
}
