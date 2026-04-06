#include "net/Buffer.hpp"
#include "net/Channel.hpp"
#include "net/EventLoop.hpp"
#include "net/protobuf/Codec.hpp"
#include "net/protobuf/ProtobufHandler.hpp"
#include "test.pb.h"

#include <chrono>
#include <cstring>
#include <functional>
#include <atomic>
#include <spdlog/spdlog.h>
#include <thread>
#include <tuple>
#include <unistd.h>

namespace {
    bool testBufferBasic() {
        hyperMuduo::net::Buffer buffer;
        buffer.append("abc", 3);
        if (buffer.readableBytes() != 3) {
            return false;
        }
        auto msg = buffer.retrieveAllAsString();
        return msg == "abc" && buffer.readableBytes() == 0;
    }

    bool testCodecRoundTrip() {
        hyperMuduo::net::Buffer wire;
        hyper::Query query;
        query.set_id(42);
        query.set_questioner("zhr");
        query.set_content("hello muduo");

        hyperMuduo::net::Codec::Encode(query, wire);
        auto [state, decoded] = hyperMuduo::net::Codec::Decode(wire);
        if (state != hyperMuduo::net::Codec::DecodeState::SUCCESS || !decoded) {
            return false;
        }
        auto out = std::dynamic_pointer_cast<hyper::Query>(decoded);
        return out && out->id() == 42 && out->questioner() == "zhr" && out->content() == "hello muduo";
    }

    bool testProtobufHandlerDispatch() {
        int query_called = 0;
        int default_called = 0;
        hyperMuduo::net::ProtobufHandler handler(
            [&](const hyperMuduo::net::TcpConnectionPtr&, const hyperMuduo::net::MessagePtr&, hyperMuduo::net::TimePoint) {
                ++default_called;
            });
        handler.SetCallback<hyper::Query>(
            [&](const hyperMuduo::net::TcpConnectionPtr&, const std::shared_ptr<hyper::Query>& msg, hyperMuduo::net::TimePoint) {
                if (msg && msg->id() == 7) {
                    ++query_called;
                }
            });

        hyperMuduo::net::Buffer in;
        hyper::Query query;
        query.set_id(7);
        query.set_questioner("reader");
        query.set_content("dispatch");
        hyperMuduo::net::Codec::Encode(query, in);

        handler.OnMessage({}, in, std::chrono::system_clock::now());
        return query_called == 1 && default_called == 0;
    }

    bool testEventLoopChannelPoller() {
        int fds[2];
        if (::pipe(fds) != 0) {
            return false;
        }

        hyperMuduo::net::EventLoop loop;
        hyperMuduo::net::Channel channel(loop, fds[0]);
        bool handled = false;
        channel.setReadCallback([&]() {
            char c = 0;
            ::read(fds[0], &c, 1);
            handled = true;
            loop.quit();
        });
        channel.listenTillReadable();

        std::thread writer([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            const char c = 'x';
            ::write(fds[1], &c, 1);
        });

        loop.loop();
        writer.join();
        ::close(fds[0]);
        ::close(fds[1]);
        return handled;
    }

    bool testEventLoopMultiChannelDispatch() {
        int p1[2];
        int p2[2];
        if (::pipe(p1) != 0 || ::pipe(p2) != 0) {
            return false;
        }

        hyperMuduo::net::EventLoop loop;
        hyperMuduo::net::Channel ch1(loop, p1[0]);
        hyperMuduo::net::Channel ch2(loop, p2[0]);
        int handled_count = 0;

        ch1.setReadCallback([&]() {
            char c = 0;
            ::read(p1[0], &c, 1);
            ++handled_count;
            if (handled_count == 2) {
                loop.quit();
            }
        });
        ch2.setReadCallback([&]() {
            char c = 0;
            ::read(p2[0], &c, 1);
            ++handled_count;
            if (handled_count == 2) {
                loop.quit();
            }
        });
        ch1.listenTillReadable();
        ch2.listenTillReadable();

        std::thread writer([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            const char a = 'a';
            const char b = 'b';
            ::write(p1[1], &a, 1);
            ::write(p2[1], &b, 1);
        });

        loop.loop();
        writer.join();
        ::close(p1[0]);
        ::close(p1[1]);
        ::close(p2[0]);
        ::close(p2[1]);
        return handled_count == 2;
    }

    bool testEventLoopQuitFromOtherThread() {
        hyperMuduo::net::EventLoop loop;
        auto begin = std::chrono::steady_clock::now();

        std::thread stopper([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            loop.quit();
        });

        loop.loop();
        stopper.join();
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - begin).count();
        return elapsed_ms >= 20 && elapsed_ms < 500;
    }

    bool testEventLoopThreadAffinityFlags() {
        hyperMuduo::net::EventLoop loop;
        const bool in_main_thread = loop.isInLoopThread();
        std::atomic<bool> in_worker_thread{true};

        std::thread worker([&]() {
            in_worker_thread = loop.isInLoopThread();
        });
        worker.join();
        return in_main_thread && !in_worker_thread.load();
    }
}

int main() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] [%s:%#] %v");
    SPDLOG_INFO("Run smoke tests...");

    const std::vector<std::pair<std::string, bool (*)()>> tests = {
        {"Buffer basic", testBufferBasic},
        {"Codec round trip", testCodecRoundTrip},
        {"Protobuf handler dispatch", testProtobufHandlerDispatch},
        {"EventLoop single channel dispatch", testEventLoopChannelPoller},
        {"EventLoop multi-channel dispatch", testEventLoopMultiChannelDispatch},
        {"EventLoop cross-thread quit", testEventLoopQuitFromOtherThread},
        {"EventLoop thread affinity flags", testEventLoopThreadAffinityFlags},
    };

    int passed = 0;
    for (const auto& [name, fn] : tests) {
        const bool ok = fn();
        if (ok) {
            ++passed;
            SPDLOG_INFO("[PASS] {}", name);
        } else {
            SPDLOG_ERROR("[FAIL] {}", name);
        }
    }

    SPDLOG_INFO("Smoke tests: {}/{} passed", passed, tests.size());
    return passed == static_cast<int>(tests.size()) ? 0 : 1;
}