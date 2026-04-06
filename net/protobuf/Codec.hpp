#pragma once

#include <memory>
#include <google/protobuf/message.h>


#include "net/Buffer.hpp"

namespace hyperMuduo::net {
    using MessagePtr = std::shared_ptr<google::protobuf::Message>;

    class Codec {
    public:
        static constexpr size_t INT32_SIZE = sizeof(int32_t);
        static constexpr size_t HEADER_LEN = INT32_SIZE;
        static constexpr size_t NAME_LEN_SIZE = INT32_SIZE;
        static constexpr size_t CHECKSUM_LEN = INT32_SIZE;

        static constexpr size_t MIN_LEN = 9; // name_len + 1(name.size()) + checksum
        static constexpr size_t MAX_LEN = 64 * 1024 * 1024; //64 MB
        enum class DecodeState {
            SUCCESS,
            INCOMPLETE,
            CHECKSUM_ERROR,
            INVALID_NAME,
            DECODE_ERROR
        };

        static std::tuple<DecodeState, MessagePtr> Decode(Buffer& buffer);

        static void Encode(const google::protobuf::Message& msg, Buffer& out);

    private:
        static int32_t CalculateAdler32(const char* buffer, int len);
    };

}
