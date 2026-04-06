#pragma once
#include <any>
#include <memory>
#include "Buffer.hpp"

namespace hyperMuduo::net {
    class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
    public:
        TcpConnection(const TcpConnection&) = delete;
        TcpConnection(TcpConnection&&) = delete;
        TcpConnection& operator=(const TcpConnection&) = delete;
        TcpConnection& operator=(TcpConnection&&) = delete;

        void Shutdown();

        void Send(Buffer& buffer);

        Buffer& outputBuffer();

        template<typename T>
        bool hasContext() const {
            return context_.has_value() && context_.type() == typeid(T);
        }

        template<typename T>
        const T& getContextAs() const {
            return std::any_cast<T&>(context_);
        }

        /**
         * judge hasContext<T> before getContextAs
         **/
        template<typename T>
        T& getContextAs() {
            return std::any_cast<T&>(context_);
        }

        void setContext(std::any context);

    private:
        std::unique_ptr<Buffer> output_buffer_;
        std::any context_;
    };

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
} // namespace hyper::net
