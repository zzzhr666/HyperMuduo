#pragma once
#include <map>
#include <memory>
#include <google/protobuf/message.h>
#include <net/TcpConnection.hpp>
#include <functional>

#include <chrono>

#include "Codec.hpp"


namespace hyperMuduo::net {
    using TimePoint = std::chrono::system_clock::time_point;
    class CallbackBase {
    public:
        virtual void OnMessage(const TcpConnectionPtr& connection, const MessagePtr& msg, TimePoint timestamp) = 0;

        virtual ~CallbackBase() = default;
    };

    template<typename T>
    class Callback final : public CallbackBase {
    public:
        using MessageCallback = std::function<void(const TcpConnectionPtr&, const std::shared_ptr<T>&, TimePoint)>;

        explicit Callback(MessageCallback cb) : callback_(std::move(cb)) {}

        void OnMessage(const TcpConnectionPtr& connection, const MessagePtr& msg, TimePoint timestamp) override {
            auto message = std::static_pointer_cast<T>(msg);
            callback_(connection, message, timestamp);
        }

    private:
        MessageCallback callback_;
    };

    class Dispatcher final {
    public:
        explicit Dispatcher(std::function<void(const TcpConnectionPtr&, const MessagePtr&, TimePoint)> default_cb);

        template<typename T>
        void RegisterMessageCallback(typename Callback<T>::MessageCallback cb) {
            dispatcher_map_[T::descriptor()] = std::make_shared<Callback<T>>(std::move(cb));
        }

        void Dispatch(const TcpConnectionPtr& connection, const MessagePtr& msg, TimePoint timestamp);

    private:
        std::map<const google::protobuf::Descriptor*, std::shared_ptr<CallbackBase>> dispatcher_map_;
        std::function<void(const TcpConnectionPtr&, const MessagePtr&, TimePoint)>default_cb_;
    };
}
