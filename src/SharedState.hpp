#pragma once
#include <string>
#include <queue>
#include <mutex>

extern std::queue<std::string> messageQueue;
extern std::mutex messageMutex;