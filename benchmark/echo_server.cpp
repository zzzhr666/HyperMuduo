#include "net/EventLoop.hpp"
#include "net/TcpServer.hpp"
#include "spdlog/spdlog.h"

#include <iostream>
#include <sys/sysinfo.h>

using namespace hyperMuduo::net;

// 高性能 Echo Server 基准测试
// 特性：
// 1. 开启多线程 (EventLoopThreadPool)
// 2. 移除 MessageCallback 中的日志打印 (消除日志瓶颈)
void run_server(int port, int thread_num) {
    EventLoop loop;
    InetAddress addr(port);
    TcpServer server(loop, addr, "BenchmarkServer");

    // 设置多线程 Reactor 模式
    server.setThreadNum(thread_num);

    // 极简 Echo 逻辑：收到什么发回什么，绝不打印日志
    server.setMessageCallback([](const TcpConnectionPtr& conn, Buffer& buf, std::chrono::system_clock::time_point) {
        // 直接回发所有可用数据
        conn->send(buf.peek(), buf.readableBytes());
        buf.retrieveAll(); 
    });

    SPDLOG_INFO("Starting High-Performance EchoServer on port {}...", port);
    SPDLOG_INFO("Thread Num: {}", thread_num);
    SPDLOG_INFO("Server started. Waiting for connections...");
    
    server.start();
    loop.loop();
}

int main(int argc, char* argv[]) {
    int port = 8888;
    // 获取 CPU 核心数
    int num_threads = get_nprocs();
    if (num_threads < 2) num_threads = 2; // 至少 2 个线程

    if (argc > 1) {
        port = std::atoi(argv[1]);
    }
    if (argc > 2) {
        num_threads = std::atoi(argv[2]);
    }

    run_server(port, num_threads);
    return 0;
}
