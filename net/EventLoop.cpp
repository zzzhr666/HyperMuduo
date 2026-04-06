#include "EventLoop.hpp"

#include <cassert>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <sys/poll.h>

#include "Channel.hpp"
#include "Poller.hpp"
namespace {
    thread_local hyperMuduo::net::EventLoop* t_current_thread_loop;
}


hyperMuduo::net::EventLoop::EventLoop()
    : looping_{false},quit_{true},thread_id_{CurrentThread::getTid()},poller_{std::make_unique<Poller>(*this)} {
    SPDLOG_INFO("Eventloop created {} in thread {}", fmt::ptr(this), thread_id_);
    if (t_current_thread_loop == nullptr) {
        t_current_thread_loop = this;
    } else {
        SPDLOG_CRITICAL("Another EventLoop {} exists in this thread {}", fmt::ptr(t_current_thread_loop), thread_id_);
        std::abort();
    }
}

hyperMuduo::net::EventLoop::~EventLoop() {
    assert(!looping_);
    t_current_thread_loop = nullptr;
}

void hyperMuduo::net::EventLoop::loop() {
    if (!looping_) {
        assertInLoopThread();
        looping_ = true;
        quit_ = false;
        while (!quit_) {
            active_channels_.clear();
            poller_->poll(kPollTime, active_channels_);
            for (auto* channel : active_channels_) {
                channel->handleEvent();
            }
        }
    }
    SPDLOG_TRACE("EventLoop {} stop looping.");
    looping_ = false;
}

void hyperMuduo::net::EventLoop::quit() {
    quit_ = true;
    //wakeup
}

hyperMuduo::net::EventLoop* hyperMuduo::net::EventLoop::getEventLoopOfCurrentThread() {
    return t_current_thread_loop;
}

void hyperMuduo::net::EventLoop::assertInLoopThread() {
    if (!isInLoopThread()) {
        abortNotInLoopThread();
    }
}

void hyperMuduo::net::EventLoop::updateChannel(Channel& channel) {
    if (&channel.getOwnerLoop() == this) {
        assertInLoopThread();
        poller_->updateChannel(channel);
    }
}

void hyperMuduo::net::EventLoop::removeChannel(Channel& channel) {
    if (&channel.getOwnerLoop() == this) {
        assertInLoopThread();
    }
}

void hyperMuduo::net::EventLoop::abortNotInLoopThread() {
    SPDLOG_CRITICAL("EventLoop::abortNotInLoopThread - EventLoop {} was created in thread {},current thread id = {}",fmt::ptr(this),thread_id_,CurrentThread::getTid());
    std::abort();
}
