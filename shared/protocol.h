#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <arpa/inet.h>

const int PAYLOAD_SIZE = 256;

enum MessageType
{
    MSG_JOIN = 1,
    MSG_CHAT = 2,
    MSG_LIST = 3,
    MSG_QUIT = 4
};

struct ChatPacket
{
    uint8_t type;
    uint16_t groupID;
    uint32_t timestamp;
    uint16_t payloadSize;
    char payload[PAYLOAD_SIZE];
};

ChatPacket createPacket(uint8_t type, uint16_t groupID, const std::string& message)
{
    ChatPacket packet;

    packet.type = type;
    packet.groupID = htons(groupID);
    packet.timestamp = htonl((uint32_t)time(nullptr));

    std::string safeMessage = message;

    if (safeMessage.length() >= PAYLOAD_SIZE)
    {
        safeMessage = safeMessage.substr(0, PAYLOAD_SIZE - 1);
    }

    packet.payloadSize = htons((uint16_t)safeMessage.length());

    memset(packet.payload, 0, PAYLOAD_SIZE);
    strcpy(packet.payload, safeMessage.c_str());

    return packet;
}

std::string getPacketMessage(ChatPacket packet)
{
    char buffer[PAYLOAD_SIZE + 1];

    memset(buffer, 0, PAYLOAD_SIZE + 1);
    memcpy(buffer, packet.payload, PAYLOAD_SIZE);

    return std::string(buffer);
}

uint16_t getPacketGroup(ChatPacket packet)
{
    return ntohs(packet.groupID);
}

uint32_t getPacketTime(ChatPacket packet)
{
    return ntohl(packet.timestamp);
}

#endif