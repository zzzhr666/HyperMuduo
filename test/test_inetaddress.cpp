#include <gtest/gtest.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "net/InetAddress.hpp"

using namespace hyperMuduo::net;

TEST(InetAddressTest, DefaultConstructor) {
    InetAddress addr(8080);
    EXPECT_EQ(addr.toIp(), "0.0.0.0");
    EXPECT_EQ(addr.toPort(), 8080);
    EXPECT_EQ(addr.toIpPort(), "0.0.0.0:8080");
}

TEST(InetAddressTest, IpAndPortConstructor) {
    InetAddress addr("192.168.1.100", 9090);
    EXPECT_EQ(addr.toIp(), "192.168.1.100");
    EXPECT_EQ(addr.toPort(), 9090);
    EXPECT_EQ(addr.toIpPort(), "192.168.1.100:9090");
}

TEST(InetAddressTest, LocalhostConstruction) {
    InetAddress addr("127.0.0.1", 3000);
    EXPECT_EQ(addr.toIp(), "127.0.0.1");
    EXPECT_EQ(addr.toPort(), 3000);
    EXPECT_EQ(addr.toIpPort(), "127.0.0.1:3000");
}

TEST(InetAddressTest, SockaddrInConstructor) {
    sockaddr_in addr_in{};
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(7070);
    inet_pton(AF_INET, "10.0.0.1", &addr_in.sin_addr);

    InetAddress addr(addr_in);
    EXPECT_EQ(addr.toIp(), "10.0.0.1");
    EXPECT_EQ(addr.toPort(), 7070);
    EXPECT_EQ(addr.toIpPort(), "10.0.0.1:7070");
}

TEST(InetAddressTest, GetSockAddrIn) {
    InetAddress addr("172.16.0.1", 5000);
    const sockaddr_in* addr_ptr = addr.getSockAddrIn();
    
    EXPECT_NE(addr_ptr, nullptr);
    EXPECT_EQ(addr_ptr->sin_family, AF_INET);
    EXPECT_EQ(ntohs(addr_ptr->sin_port), 5000);
}

TEST(InetAddressTest, GetSockAddrInMutable) {
    InetAddress addr(6000);
    sockaddr_in* addr_ptr = addr.getSockAddrInMutable();
    
    EXPECT_NE(addr_ptr, nullptr);
    
    // Modify the address
    sockaddr_in new_addr{};
    new_addr.sin_family = AF_INET;
    new_addr.sin_port = htons(7777);
    inet_pton(AF_INET, "192.168.0.1", &new_addr.sin_addr);
    
    addr.setSockAddrIn(new_addr);
    
    EXPECT_EQ(addr.toIp(), "192.168.0.1");
    EXPECT_EQ(addr.toPort(), 7777);
}

TEST(InetAddressTest, PortRange) {
    // Test with port 0
    InetAddress addr0(0);
    EXPECT_EQ(addr0.toPort(), 0);
    
    // Test with max port
    InetAddress addr_max(65535);
    EXPECT_EQ(addr_max.toPort(), 65535);
}

TEST(InetAddressTest, InvalidIpHandling) {
    // This should log an error but still construct the object
    InetAddress addr("invalid_ip", 8080);
    // The address will be unset/zeroed due to invalid IP
    EXPECT_EQ(addr.toPort(), 8080);
}
