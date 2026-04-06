#pragma once
#include <atomic>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Base/CurrentThread.hpp"

namespace hyperMuduo::net {
    class Channel;
    class Poller;
    class EventLoop {
    public:
        EventLoop();

        ~EventLoop();

        EventLoop(const EventLoop&) = delete;

        EventLoop& operator=(const EventLoop&) = delete;

        EventLoop(EventLoop&&) = delete;

        EventLoop& operator=(EventLoop&&) = delete;

        void loop();

        void quit();

        static EventLoop* getEventLoopOfCurrentThread();

        void assertInLoopThread();

        [[nodiscard]] bool isInLoopThread() const {
            return thread_id_ == CurrentThread::getTid();
        }

        void updateChannel(Channel& channel);

        void removeChannel(Channel& channel);

    private:
        void abortNotInLoopThread();

    private:
        static constexpr std::chrono::milliseconds kPollTime{10};
        using ChannelList = std::vector<Channel*>;
        using ChannelMap = std::unordered_map<int, std::unique_ptr<Channel>>;

        std::atomic<bool> looping_;

        std::atomic<bool> quit_;

        const int thread_id_;

        std::unique_ptr<Poller> poller_;

        ChannelMap channel_map_;

        ChannelList active_channels_;

    };
}
