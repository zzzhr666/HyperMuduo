#pragma once

#include <string>
#include <arpa/inet.h>

namespace hyperMuduo::net {
    class InetAddress {
    public:
        explicit InetAddress(uint16_t port);

        InetAddress(const std::string& ip, uint16_t port);

        explicit InetAddress(const sockaddr_in& addr);

        std::string toIp() const;

        uint16_t toPort() const;

        std::string toIpPort() const;

        const sockaddr_in* getSockAddrIn() const;

        sockaddr_in* getSockAddrInMutable();

        void setSockAddrIn(const sockaddr_in& addr) {
            addr_ = addr;
        }

    private:
        sockaddr_in addr_;
    };
}
