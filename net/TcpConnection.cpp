#include "TcpConnection.hpp"

namespace hyperMuduo::net {


    Buffer& TcpConnection::outputBuffer() {
        if (!output_buffer_) {
            output_buffer_ = std::make_unique<Buffer>();
        }
        return *output_buffer_;
    }
    void TcpConnection::setContext(std::any context) {
        context_ = std::move(context);
    }
} // namespace hyper::net
