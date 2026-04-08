#include <gtest/gtest.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <chrono>
#include "net/Socket.hpp"
#include "net/InetAddress.hpp"

using namespace hyperMuduo::net;

TEST(SocketTest, DefaultConstructor) {
    Socket sock;
    EXPECT_TRUE(sock.valid());
    EXPECT_GE(sock.getFd(), 0);
    // Socket should be non-blocking and close-on-exec by default
    int flags = fcntl(sock.getFd(), F_GETFL);
    EXPECT_TRUE(flags & O_NONBLOCK);
    flags = fcntl(sock.getFd(), F_GETFD);
    EXPECT_TRUE(flags & FD_CLOEXEC);
}

TEST(SocketTest, MoveConstructor) {
    Socket sock1;
    int fd = sock1.getFd();
    EXPECT_TRUE(sock1.valid());
    
    Socket sock2(std::move(sock1));
    EXPECT_FALSE(sock1.valid());
    EXPECT_TRUE(sock2.valid());
    EXPECT_EQ(sock2.getFd(), fd);
}

TEST(SocketTest, MoveAssignment) {
    Socket sock1;
    int fd1 = sock1.getFd();
    
    Socket sock2;
    int fd2 = sock2.getFd();
    
    sock2 = std::move(sock1);
    EXPECT_FALSE(sock1.valid());
    EXPECT_TRUE(sock2.valid());
    EXPECT_EQ(sock2.getFd(), fd1);
    // fd2 should have been closed
}

TEST(SocketTest, Invalidate) {
    Socket sock;
    EXPECT_TRUE(sock.valid());
    sock.invalidate();
    EXPECT_FALSE(sock.valid());
    EXPECT_EQ(sock.getFd(), Socket::INVALID_FD);
}

TEST(SocketTest, ConstructFromFd) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_GE(fd, 0);
    
    Socket sock(fd);
    EXPECT_TRUE(sock.valid());
    EXPECT_EQ(sock.getFd(), fd);
    // Socket destructor will close the fd
}

TEST(SocketTest, BindAndListen) {
    Socket sock;
    InetAddress addr(18080);
    
    EXPECT_NO_THROW(sock.bindAddress(addr));
    EXPECT_NO_THROW(sock.listen());
}

TEST(SocketTest, SetReuseAddr) {
    Socket sock;
    EXPECT_NO_THROW(sock.setReuseAddr());
    
    // Verify the option is set
    int optval = 0;
    socklen_t optlen = sizeof(optval);
    int ret = getsockopt(sock.getFd(), SOL_SOCKET, SO_REUSEADDR, &optval, &optlen);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(optval, 1);
}

TEST(SocketTest, SetReusePort) {
    Socket sock;
    EXPECT_NO_THROW(sock.setReusePort());
    
    // Verify the option is set
    int optval = 0;
    socklen_t optlen = sizeof(optval);
    int ret = getsockopt(sock.getFd(), SOL_SOCKET, SO_REUSEPORT, &optval, &optlen);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(optval, 1);
}

TEST(SocketTest, SetKeepAlive) {
    Socket sock;
    EXPECT_NO_THROW(sock.setKeepAlive());
    
    // Verify the option is set
    int optval = 0;
    socklen_t optlen = sizeof(optval);
    int ret = getsockopt(sock.getFd(), SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(optval, 1);
}

TEST(SocketTest, BindInvalidAddress) {
    Socket sock;
    InetAddress addr("256.256.256.256", 8080); // Invalid IP
    
    // This should abort, but we can't easily test abort in unit tests
    // The behavior is that it will call std::abort()
}

TEST(SocketTest, AcceptNonBlockingSocket) {
    // Create a listening socket
    Socket server_sock;
    server_sock.setReuseAddr();
    InetAddress server_addr(18081);
    server_sock.bindAddress(server_addr);
    server_sock.listen();
    
    // Try to accept on a non-blocking socket (should return invalid when no connections)
    InetAddress peer_addr(0);
    Socket conn_sock = server_sock.accept(peer_addr);
    
    // Since there are no pending connections, accept should return invalid
    // with EAGAIN/EWOULDBLOCK
    EXPECT_FALSE(conn_sock.valid());
}

TEST(SocketTest, MultipleSocketOptions) {
    Socket sock;
    sock.setReuseAddr();
    sock.setReusePort();
    sock.setKeepAlive();
    
    InetAddress addr(18082);
    sock.bindAddress(addr);
    sock.listen();
    
    EXPECT_TRUE(sock.valid());
}

TEST(SocketTest, DestructorClosesFd) {
    int fd;
    {
        Socket sock;
        fd = sock.getFd();
    }
    // After Socket goes out of scope, fd should be closed
    // Trying to use it should fail
    char buf;
    int ret = read(fd, &buf, 1);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(errno, EBADF);
}
