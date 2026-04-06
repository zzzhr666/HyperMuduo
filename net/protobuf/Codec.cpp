#include "Codec.hpp"
#include <string_view>
#include <algorithm>
#include <zlib.h>
#include <string>
#include <endian.h>

namespace {
    hyperMuduo::net::MessagePtr createMessage(std::string_view type_name) {
        if (const auto descriptor = google::protobuf::DescriptorPool::generated_pool()->
                FindMessageTypeByName(type_name)) {
            if (const auto prototype = google::protobuf::MessageFactory::generated_factory()->
                    GetPrototype(descriptor)) {
                return std::shared_ptr<google::protobuf::Message>(prototype->New());
            }
        }
        return nullptr;
    }
}

std::tuple<hyperMuduo::net::Codec::DecodeState, hyperMuduo::net::MessagePtr> hyperMuduo::net::Codec::Decode(Buffer& buffer) {
    if (buffer.readableBytes() < HEADER_LEN) {
        return {DecodeState::INCOMPLETE, nullptr};
    }

    int32_t total_len;
    memcpy(&total_len, buffer.peek(), HEADER_LEN);
    total_len = be32toh(total_len);

    // [Safety] Sanity check total_len IMMEDIATELY to prevent OOM/Malicious frames
    if (total_len < static_cast<int32_t>(MIN_LEN) || total_len > static_cast<int32_t>(MAX_LEN)) {
        // FATAL ERROR: Protocol synchronization is lost. 
        // We must discard all data and signal the caller to CLOSE the connection.
        buffer.retrieveAll(); 
        return {DecodeState::DECODE_ERROR, nullptr};
    }

    if (buffer.readableBytes() < HEADER_LEN + total_len) {
        return {DecodeState::INCOMPLETE, nullptr};
    }

    const char* data = buffer.peek();
    
    // Checksum verification
    int32_t expected_checksum;
    memcpy(&expected_checksum, data + HEADER_LEN + total_len - CHECKSUM_LEN, CHECKSUM_LEN);
    expected_checksum = be32toh(expected_checksum);

    const int32_t actual_checksum = CalculateAdler32(data + HEADER_LEN, static_cast<int>(total_len - CHECKSUM_LEN));
    if (actual_checksum != expected_checksum) {
        buffer.retrieve(HEADER_LEN + total_len);
        return {DecodeState::CHECKSUM_ERROR, nullptr};
    }

    int32_t name_len;
    memcpy(&name_len, data + HEADER_LEN, NAME_LEN_SIZE);
    name_len = be32toh(name_len);

    if (name_len <= 0 || name_len > static_cast<int32_t>(total_len - NAME_LEN_SIZE - CHECKSUM_LEN)) {
        buffer.retrieve(HEADER_LEN + total_len);
        return {DecodeState::DECODE_ERROR, nullptr};
    }

    std::string_view name{data + HEADER_LEN + NAME_LEN_SIZE, static_cast<size_t>(name_len)};
    auto msg = createMessage(name);
    if (!msg) {
        buffer.retrieve(HEADER_LEN + total_len);
        return {DecodeState::INVALID_NAME, nullptr};
    }

    size_t serialized_data_offset = HEADER_LEN + NAME_LEN_SIZE + name_len;
    size_t serialized_data_len = total_len - NAME_LEN_SIZE - name_len - CHECKSUM_LEN;
    
    if (!msg->ParseFromArray(data + serialized_data_offset, static_cast<int>(serialized_data_len))) {
        buffer.retrieve(HEADER_LEN + total_len);
        return {DecodeState::DECODE_ERROR, nullptr};
    }

    buffer.retrieve(HEADER_LEN + total_len);
    return {DecodeState::SUCCESS, msg};
}

void hyperMuduo::net::Codec::Encode(const google::protobuf::Message& msg, Buffer& out) {
    std::string_view type_name = msg.GetTypeName();
    const auto payload_size = static_cast<int32_t>(msg.ByteSizeLong());
    const auto name_len = static_cast<int32_t>(type_name.size());
    const auto total_size = static_cast<int32_t>(NAME_LEN_SIZE + name_len + payload_size + CHECKSUM_LEN);
    int32_t be_total_size = htobe32(total_size);
    out.ensureWritableBytes(total_size + HEADER_LEN);
    char* writer_ptr = out.beginWrite();
    int pos = 0;
    memcpy(writer_ptr + pos, &be_total_size, HEADER_LEN);
    pos += HEADER_LEN;
    int32_t be_name_len = htobe32(name_len);
    memcpy(writer_ptr + pos, &be_name_len, NAME_LEN_SIZE);
    pos += NAME_LEN_SIZE;
    std::ranges::copy(type_name, writer_ptr + pos);
    pos += name_len;
    if (!msg.SerializeToArray(writer_ptr + pos, payload_size)) {
        std::cerr << "Failed to serialize message" << std::endl;
        return;
    }
    pos += payload_size;
    int32_t secret = CalculateAdler32(writer_ptr + HEADER_LEN, static_cast<int>(total_size - CHECKSUM_LEN));
    secret = htobe32(secret);
    memcpy(writer_ptr + pos, &secret, CHECKSUM_LEN);
    out.hasWritten(total_size + HEADER_LEN);
}

int32_t hyperMuduo::net::Codec::CalculateAdler32(const char* buffer, const int len) {
    return static_cast<int32_t>(::adler32(1L, reinterpret_cast<const unsigned char*>(buffer), len));
}
