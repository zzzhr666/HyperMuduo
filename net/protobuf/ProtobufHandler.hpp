#pragma once
#include <memory>

#include "Codec.hpp"
#include "Dispatcher.hpp"

namespace hyperMuduo::net {

        class ProtobufHandler {
        public:
            ProtobufHandler(const ProtobufHandler&) = delete;
            ProtobufHandler& operator=(const ProtobufHandler&) = delete;
            ProtobufHandler(ProtobufHandler&&) = default;
            ProtobufHandler& operator=(ProtobufHandler&&) = default;

            explicit ProtobufHandler(std::function<void(const TcpConnectionPtr&,const MessagePtr&,TimePoint)> default_cb);

            void OnMessage(const TcpConnectionPtr& connection, Buffer& buffer, TimePoint timestamp) const;

            template<typename T>
            void SetCallback(std::function<void(const TcpConnectionPtr&, const std::shared_ptr<T>&, TimePoint)> cb) {
                dispatcher_->RegisterMessageCallback<T>(cb);
            }

            void Send(const TcpConnectionPtr& connection,const google::protobuf::Message& msg);
        private:
            std::unique_ptr<Dispatcher> dispatcher_;
        };


}
