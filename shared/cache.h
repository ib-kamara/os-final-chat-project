#ifndef CACHE_H
#define CACHE_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>

struct CachedMessage
{
    int groupID;
    std::string sender;
    std::string message;
    time_t timestamp;
};

class MessageCache
{
private:
    std::map<int, std::vector<CachedMessage> > cache;
    int maxMessages;
    int ttlSeconds;

public:
    MessageCache()
    {
        maxMessages = 5;
        ttlSeconds = 300;
    }

    void addMessage(int groupID, std::string sender, std::string message)
    {
        CachedMessage item;

        item.groupID = groupID;
        item.sender = sender;
        item.message = message;
        item.timestamp = time(nullptr);

        cache[groupID].push_back(item);

        if ((int)cache[groupID].size() > maxMessages)
        {
            cache[groupID].erase(cache[groupID].begin());
        }
    }

    std::vector<CachedMessage> getMessages(int groupID)
    {
        std::vector<CachedMessage> validMessages;
        time_t now = time(nullptr);

        for (int i = 0; i < (int)cache[groupID].size(); i++)
        {
            int age = (int)(now - cache[groupID][i].timestamp);

            if (age <= ttlSeconds)
            {
                validMessages.push_back(cache[groupID][i]);
            }
        }

        return validMessages;
    }
};

#endif