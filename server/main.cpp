#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../shared/protocol.h"
#include "../shared/cache.h"
#include "../shared/thread_pool.h"
#include "../shared/logger.h"

std::mutex serverMutex;
std::map<int, std::vector<int> > groupClients;

MessageCache messageCache;
Logger logger;

void sendText(int clientSocket, std::string text)
{
    send(clientSocket, text.c_str(), text.length(), 0);
}

void sendGroupHistory(int clientSocket, int groupID)
{
    std::vector<CachedMessage> history = messageCache.getMessages(groupID);

    if (history.size() == 0)
    {
        sendText(clientSocket, "No recent messages in this group yet.\n");
    }
    else
    {
        sendText(clientSocket, "\nRecent Messages:\n");

        for (int i = 0; i < (int)history.size(); i++)
        {
            std::string line = history[i].sender + ": " + history[i].message + "\n";
            sendText(clientSocket, line);
        }
    }
}

void broadcastToGroup(int groupID, int senderSocket, std::string message)
{
    std::lock_guard<std::mutex> lock(serverMutex);

    for (int i = 0; i < (int)groupClients[groupID].size(); i++)
    {
        int clientSocket = groupClients[groupID][i];

        if (clientSocket != senderSocket)
        {
            std::string text = "Group " + std::to_string(groupID) + " message: " + message + "\n";
            sendText(clientSocket, text);
        }
    }
}

void removeClient(int clientSocket)
{
    std::lock_guard<std::mutex> lock(serverMutex);

    for (auto &group : groupClients)
    {
        std::vector<int> updated;

        for (int i = 0; i < (int)group.second.size(); i++)
        {
            if (group.second[i] != clientSocket)
            {
                updated.push_back(group.second[i]);
            }
        }

        group.second = updated;
    }
}

void handleClient(int clientSocket)
{
    int currentGroup = 1;
    std::string senderName = "Client" + std::to_string(clientSocket);

    {
        std::lock_guard<std::mutex> lock(serverMutex);
        groupClients[currentGroup].push_back(clientSocket);
    }

    logger.writeLog(senderName + " connected and joined group 1");

    sendText(clientSocket, "Welcome to the C++ Group Chat System.\n");
    sendText(clientSocket, "You are in group 1.\n");
    sendText(clientSocket, "Commands:\n");
    sendText(clientSocket, "/join 1 or /join 2\n");
    sendText(clientSocket, "/list\n");
    sendText(clientSocket, "/history\n");
    sendText(clientSocket, "/quit\n");
    sendText(clientSocket, "Type a normal message to send it to your group.\n\n");

    sendGroupHistory(clientSocket, currentGroup);

    bool running = true;

    while (running)
    {
        ChatPacket packet;
        int bytesRead = recv(clientSocket, &packet, sizeof(packet), 0);

        if (bytesRead <= 0)
        {
            running = false;
        }
        else
        {
            std::string message = getPacketMessage(packet);

            if (message == "/quit")
            {
                sendText(clientSocket, "Goodbye.\n");
                running = false;
            }
            else if (message == "/list")
            {
                sendText(clientSocket, "Active groups: 1 and 2\n");
            }
            else if (message == "/history")
            {
                sendGroupHistory(clientSocket, currentGroup);
            }
            else if (message == "/join 1")
            {
                removeClient(clientSocket);

                currentGroup = 1;

                {
                    std::lock_guard<std::mutex> lock(serverMutex);
                    groupClients[currentGroup].push_back(clientSocket);
                }

                sendText(clientSocket, "You joined group 1.\n");
                sendGroupHistory(clientSocket, currentGroup);

                logger.writeLog(senderName + " switched to group 1");
            }
            else if (message == "/join 2")
            {
                removeClient(clientSocket);

                currentGroup = 2;

                {
                    std::lock_guard<std::mutex> lock(serverMutex);
                    groupClients[currentGroup].push_back(clientSocket);
                }

                sendText(clientSocket, "You joined group 2.\n");
                sendGroupHistory(clientSocket, currentGroup);

                logger.writeLog(senderName + " switched to group 2");
            }
            else
            {
                messageCache.addMessage(currentGroup, senderName, message);

                logger.writeLog(senderName + " sent message in group " + std::to_string(currentGroup) + ": " + message);

                sendText(clientSocket, "Message sent to group " + std::to_string(currentGroup) + ".\n");

                broadcastToGroup(currentGroup, clientSocket, message);
            }
        }
    }

    removeClient(clientSocket);
    close(clientSocket);

    logger.writeLog(senderName + " disconnected");
}

int main()
{
    int serverSocket;
    int clientSocket;
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;
    socklen_t clientSize;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0)
    {
        std::cout << "Socket failed." << std::endl;
        return 1;
    }

    int option = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cout << "Bind failed. Port may already be in use." << std::endl;
        return 1;
    }

    if (listen(serverSocket, 10) < 0)
    {
        std::cout << "Listen failed." << std::endl;
        return 1;
    }

    ThreadPool pool(4);

    std::cout << "Server listening on port 8080..." << std::endl;
    logger.writeLog("Server started on port 8080");

    clientSize = sizeof(clientAddress);

    while (true)
    {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientSize);

        if (clientSocket < 0)
        {
            std::cout << "Accept failed." << std::endl;
        }
        else
        {
            std::cout << "New client connected." << std::endl;

            pool.enqueue([clientSocket]()
            {
                handleClient(clientSocket);
            });
        }
    }

    close(serverSocket);

    return 0;
}