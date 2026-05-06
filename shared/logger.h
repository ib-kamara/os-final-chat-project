#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>

class Logger
{
private:
    std::mutex logMutex;

public:
    void writeLog(std::string text)
    {
        std::lock_guard<std::mutex> lock(logMutex);

        std::ofstream file;
        file.open("logs/chat_log.txt", std::ios::app);

        if (file.is_open())
        {
            file << "[" << time(nullptr) << "] " << text << std::endl;
            file.close();
        }
    }
};

#endif