#include "net/EventLoop.hpp"
#include "net/TcpClient.hpp"
#include "spdlog/spdlog.h"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>
#include <thread>
#include <sys/sysinfo.h>

using namespace hyperMuduo::net;

// 测试参数
static const size_t kMessageSize = 1024;      // 消息大小 1KB
static const int kConnectionsPerThread = 100; // 每个线程的连接数
static const int kDurationSeconds = 10;       // 测试时长

// 统计数据
static std::atomic<int64_t> g_bytes_read{0};
static std::atomic<int64_t> g_messages_read{0};
static std::atomic<int> g_clients_connected{0};

// 打印结果
void print_stats() {
    double duration_sec = kDurationSeconds;
    double mbs = static_cast<double>(g_bytes_read.load()) / 1024.0 / 1024.0 / duration_sec;
    double qps = static_cast<double>(g_messages_read.load()) / duration_sec;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n============================================\n";
    std::cout << "Benchmark Results (" << kDurationSeconds << "s)\n";
    std::cout << "============================================\n";
    std::cout << "Total Connections: " << g_clients_connected.load() << "\n";
    std::cout << "Total Throughput : " << mbs << " MB/s\n";
    std::cout << "Total QPS        : " << qps << " msgs/s\n";
    std::cout << "Total Messages   : " << g_messages_read.load() << "\n";
    std::cout << "Total Bytes      : " << g_bytes_read.load() << "\n";
    std::cout << "============================================\n";
}

// 每个工作线程运行一个 EventLoop 和一组 TcpClient
void worker_thread(int thread_idx, int num_clients, const std::string& server_ip, int port) {
    EventLoop loop;
    std::vector<std::unique_ptr<TcpClient>> clients;
    clients.reserve(num_clients);

    std::string payload(kMessageSize, 'H'); 

    for (int i = 0; i < num_clients; ++i) {
        int conn_id = thread_idx * num_clients + i;
        std::string name = "Worker-" + std::to_string(thread_idx) + "-Client-" + std::to_string(i);
        auto client = std::make_unique<TcpClient>(loop, InetAddress(server_ip, port), name);

        client->setConnectionCallback([&](const TcpConnectionPtr& conn) {
            if (conn->connected()) {
                g_clients_connected.fetch_add(1);
                conn->send(payload); 
            }
        });

        client->setMessageCallback([&](const TcpConnectionPtr& conn, Buffer& buf, std::chrono::system_clock::time_point) {
            g_bytes_read += buf.readableBytes();
            g_messages_read += 1;
            
            conn->send(buf.peek(), buf.readableBytes()); 
            buf.retrieveAll();
        });

        client->connect();
        clients.push_back(std::move(client));
    }

    // 定时退出
    loop.runAfter(std::chrono::seconds(kDurationSeconds + 2), [&]() { // 加 2s 缓冲
        loop.quit();
    });

    loop.loop();
}

int main(int argc, char* argv[]) {
    std::string ip = "127.0.0.1";
    int port = 8888;
    
    // 获取 CPU 核心数，用于决定客户端线程数，尽量占满带宽
    int cpu_cores = get_nprocs();
    // 使用一半的核心数作为客户端线程数，避免客户端自身调度开销过大
    int client_threads_count = std::max(2, cpu_cores / 2); 
    
    if (argc > 2) {
        ip = argv[1];
        port = std::atoi(argv[2]);
    }

    int total_connections = client_threads_count * kConnectionsPerThread;

    SPDLOG_INFO("========================================");
    SPDLOG_INFO(" Starting Multithreaded Benchmark Client");
    SPDLOG_INFO("========================================");
    SPDLOG_INFO("Target: {}:{}", ip, port);
    SPDLOG_INFO("Client Threads: {}", client_threads_count);
    SPDLOG_INFO("Connections/Thread: {}", kConnectionsPerThread);
    SPDLOG_INFO("Total Connections: {}", total_connections);
    SPDLOG_INFO("Message Size: {} bytes", kMessageSize);
    SPDLOG_INFO("Duration: {}s", kDurationSeconds);
    SPDLOG_INFO("========================================");

    std::vector<std::thread> workers;
    workers.reserve(client_threads_count);

    for (int i = 0; i < client_threads_count; ++i) {
        workers.emplace_back(worker_thread, i, kConnectionsPerThread, ip, port);
    }

    // 等待所有工作线程结束
    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    print_stats();
    return 0;
}
