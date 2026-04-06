#include <spdlog/spdlog.h>
#include "Channel.hpp"

#include "EventLoop.hpp"

hyperMuduo::net::Channel::Channel(EventLoop& loop, int fd)
    : loop_{loop}, fd_{fd}, events_{kNoneEvent}, return_events_{0}, index_{NOT_FILL} {}

void hyperMuduo::net::Channel::handleEvent() {
    if (return_events_ & POLLNVAL) {
        SPDLOG_WARN("Channel::handleEvent() POLLNVAL");
    }

    if (return_events_ & (POLLNVAL | POLLERR)) {
        if (errorCallback_) {
            errorCallback_();
        }
    }
    if (return_events_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) {
            readCallback_();
        }
    }

    if (return_events_ & POLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}

void hyperMuduo::net::Channel::notifyLoop() {
    loop_.updateChannel(*this);
}

