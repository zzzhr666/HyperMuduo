#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "net/Acceptor.hpp"
#include "net/EventLoop.hpp"
#include "net/InetAddress.hpp"
#include "net/Socket.hpp"

using namespace hyperMuduo::net;

TEST(AcceptorTest, ConstructionAndListening) {
    EventLoop loop;
    InetAddress addr(19000);
    Acceptor acceptor(loop, addr);
    
    EXPECT_FALSE(acceptor.listening());
    
    // Start listening
    acceptor.listen();
    EXPECT_TRUE(acceptor.listening());
}

TEST(AcceptorTest, AcceptIncomingConnection) {
    EventLoop loop;
    InetAddress listen_addr(19001);
    Acceptor acceptor(loop, listen_addr);
    
    std::atomic<bool> connection_received{false};
    InetAddress received_addr(0);
    
    acceptor.setNewConnectionCallback([&](Socket conn_socket, const InetAddress& peer_addr) {
        connection_received = true;
        received_addr = peer_addr;
        // Connection socket should be valid
        EXPECT_TRUE(conn_socket.valid());
        loop.quit();
    });
    
    acceptor.listen();
    EXPECT_TRUE(acceptor.listening());
    
    // Create a client connection from another thread
    std::thread client([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Create a client socket and connect
        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_GE(client_fd, 0);
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(19001);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
        
        int ret = connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        EXPECT_EQ(ret, 0);
        
        close(client_fd);
    });
    
    loop.loop();
    client.join();
    
    EXPECT_TRUE(connection_received.load());
}

TEST(AcceptorTest, MultipleConnections) {
    EventLoop loop;
    InetAddress listen_addr(19002);
    Acceptor acceptor(loop, listen_addr);
    
    std::atomic<int> connection_count{0};
    constexpr int num_connections = 3;
    
    acceptor.setNewConnectionCallback([&](Socket conn_socket, const InetAddress&) {
        connection_count++;
        EXPECT_TRUE(conn_socket.valid());
        if (connection_count == num_connections) {
            loop.quit();
        }
    });
    
    acceptor.listen();
    
    // Create multiple client connections
    std::thread clients([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        for (int i = 0; i < num_connections; i++) {
            int client_fd = socket(AF_INET, SOCK_STREAM, 0);
            EXPECT_GE(client_fd, 0);
            
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(19002);
            inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
            
            int ret = connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
            EXPECT_EQ(ret, 0);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            close(client_fd);
        }
    });
    
    loop.loop();
    clients.join();
    
    EXPECT_EQ(connection_count.load(), num_connections);
}

TEST(AcceptorTest, CallbackNotInvokedBeforeListen) {
    EventLoop loop;
    InetAddress listen_addr(19003);
    Acceptor acceptor(loop, listen_addr);
    
    std::atomic<bool> callback_called{false};
    
    acceptor.setNewConnectionCallback([&](Socket, const InetAddress&) {
        callback_called = true;
    });
    
    // Don't call listen() yet
    EXPECT_FALSE(acceptor.listening());
    
    // Try to connect anyway
    std::thread client([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_GE(client_fd, 0);
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(19003);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
        
        // This should fail since server is not listening
        int ret = connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        // Connection should fail or timeout
        close(client_fd);
        
        loop.quit();
    });
    
    // Run loop briefly
    std::thread quiter([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        loop.quit();
    });
    
    loop.loop();
    client.join();
    quiter.join();
    
    // Callback should not have been called
    EXPECT_FALSE(callback_called.load());
}

TEST(AcceptorTest, DifferentIpBinding) {
    EventLoop loop;
    // Bind to localhost specifically
    InetAddress listen_addr("127.0.0.1", 19004);
    Acceptor acceptor(loop, listen_addr);
    
    acceptor.listen();
    EXPECT_TRUE(acceptor.listening());
    
    // Quit after a brief period
    std::thread quiter([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        loop.quit();
    });
    
    loop.loop();
    quiter.join();
}

TEST(AcceptorTest, ConnectionCallbackReceivesCorrectPeerAddress) {
    EventLoop loop;
    InetAddress listen_addr(19005);
    Acceptor acceptor(loop, listen_addr);
    
    InetAddress received_peer_addr(0);
    std::atomic<bool> connection_received{false};
    
    acceptor.setNewConnectionCallback([&](Socket, const InetAddress& peer_addr) {
        received_peer_addr = peer_addr;
        connection_received = true;
        loop.quit();
    });
    
    acceptor.listen();
    
    std::thread client([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        EXPECT_GE(client_fd, 0);
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(19005);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
        
        int ret = connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        EXPECT_EQ(ret, 0);
        
        close(client_fd);
    });
    
    loop.loop();
    client.join();
    
    EXPECT_TRUE(connection_received.load());
    // The peer address should be 127.0.0.1
    EXPECT_EQ(received_peer_addr.toIp(), "127.0.0.1");
    EXPECT_GT(received_peer_addr.toPort(), 0); // Ephemeral port
}
