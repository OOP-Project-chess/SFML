#include "NetworkClient.hpp"
#include <iostream>
#include <array>

using boost::asio::ip::tcp;

NetworkClient::NetworkClient(const std::string& host, unsigned short port)
    : socket_(io_) {
    tcp::endpoint endpoint(boost::asio::ip::make_address(host), port);
    socket_.connect(endpoint);
}

NetworkClient::~NetworkClient() {
    running_ = false;
    if (receiveThread_.joinable())
        receiveThread_.join();
    socket_.close();
}

void NetworkClient::startReceiving(const std::function<void(const std::string&)>& onMessageReceived) {
    running_ = true;
    receiveThread_ = std::thread([this, onMessageReceived]() {
        try {
            while (running_) {
                std::array<char, 1024> buffer{};
                boost::system::error_code error;
                size_t len = socket_.read_some(boost::asio::buffer(buffer), error);

                if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
                    std::cout << "[서버 연결 종료]\n";
                    break;
                } else if (error) {
                    std::cerr << "[서버 수신 에러]: " << error.message() << "\n";
                    break;
                }

                if (len > 0) {
                    std::string msg(buffer.data(), len);
                    onMessageReceived(msg);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[서버 수신 예외]: " << e.what() << "\n";
        }
    });
}

tcp::socket& NetworkClient::getSocket() {
    return socket_;
}