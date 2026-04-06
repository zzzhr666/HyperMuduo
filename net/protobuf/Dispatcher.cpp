#include "Dispatcher.hpp"


hyperMuduo::net::Dispatcher::Dispatcher(std::function<void(const TcpConnectionPtr&, const MessagePtr&, TimePoint)> default_cb)
    : default_cb_{std::move(default_cb)} {
}

void hyperMuduo::net::Dispatcher::Dispatch(const TcpConnectionPtr& connection,
                                      const MessagePtr& msg,
                                      TimePoint timestamp) {
    if (const auto it = dispatcher_map_.find(msg->GetDescriptor()); it != dispatcher_map_.end()) {
        it->second->OnMessage(connection, msg, timestamp);
    } else {
        std::cerr << "dispatcher not found" << std::endl;
        default_cb_(connection, msg, timestamp);
    }
}
