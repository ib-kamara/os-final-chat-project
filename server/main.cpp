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
            std::string text = "Group " + std::to_string(groupID) + ": " + message + "\n";
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

    logger.writeLog(senderName + " connected");

    sendText(clientSocket, "Welcome.\nCommands: /join 1, /join 2, /list, /history, /quit\n");

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
                running = false;
            }
            else if (message == "/list")
            {
                sendText(clientSocket, "Groups: 1, 2\n");
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

                sendText(clientSocket, "Joined group 1\n");
            }
            else if (message == "/join 2")
            {
                removeClient(clientSocket);
                currentGroup = 2;

                {
                    std::lock_guard<std::mutex> lock(serverMutex);
                    groupClients[currentGroup].push_back(clientSocket);
                }

                sendText(clientSocket, "Joined group 2\n");
            }
            else
            {
                // STORE MESSAGE
                messageCache.addMessage(currentGroup, senderName, message);

                // PERFORMANCE METRIC
                logger.writeMetric("Message processed in group " + std::to_string(currentGroup));

                // LOG
                logger.writeLog(senderName + ": " + message);

                sendText(clientSocket, "Sent.\n");

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
    struct sockaddr_in serverAddress;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    int option = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 10);

    ThreadPool pool(4);

    std::cout << "Server running on port 8080\n";

    while (true)
    {
        int clientSocket = accept(serverSocket, nullptr, nullptr);

        pool.enqueue([clientSocket]()
        {
            handleClient(clientSocket);
        });
    }

    return 0;
}