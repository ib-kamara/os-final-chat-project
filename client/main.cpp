#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../shared/protocol.h"

void receiveMessages(int socketNumber)
{
    char buffer[1024];

    while (true)
    {
        for (int i = 0; i < 1024; i++)
        {
            buffer[i] = '\0';
        }

        int bytesRead = recv(socketNumber, buffer, 1024, 0);

        if (bytesRead <= 0)
        {
            std::cout << "Disconnected from server." << std::endl;
            return;
        }

        std::cout << buffer;
    }
}

int main()
{
    int socketNumber;
    struct sockaddr_in serverAddress;

    socketNumber = socket(AF_INET, SOCK_STREAM, 0);

    if (socketNumber < 0)
    {
        std::cout << "Socket creation failed." << std::endl;
        return 1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0)
    {
        std::cout << "Invalid address." << std::endl;
        return 1;
    }

    if (connect(socketNumber, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cout << "Connection failed. Make sure the server is running first." << std::endl;
        return 1;
    }

    std::thread receiver(receiveMessages, socketNumber);

    std::string message;
    bool running = true;

    while (running)
    {
        getline(std::cin, message);

        ChatPacket packet = createPacket(MSG_CHAT, 1, message);
        send(socketNumber, &packet, sizeof(packet), 0);

        if (message == "/quit")
        {
            running = false;
        }
    }

    close(socketNumber);
    receiver.detach();

    return 0;
}