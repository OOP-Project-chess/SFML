#pragma once

#include <boost/asio.hpp>
#include <thread>
#include <functional>

class NetworkClient {
public:
    NetworkClient(const std::string& host, unsigned short port);
    ~NetworkClient();

    void startReceiving(const std::function<void(const std::string&)>& onMessageReceived);
    boost::asio::ip::tcp::socket& getSocket();

private:
    boost::asio::io_context io_;
    boost::asio::ip::tcp::socket socket_;
    std::thread receiveThread_;
    bool running_ = false;
};