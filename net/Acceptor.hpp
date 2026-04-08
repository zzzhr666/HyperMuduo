#pragma once
#include <functional>


#include "Socket.hpp"
#include "Channel.hpp"
namespace hyperMuduo::net {

    class InetAddress;
    class EventLoop;
    class Acceptor {
    public:
        using NewConnectionCallback = std::function<void(Socket, const InetAddress&)>;

        Acceptor(EventLoop& loop,const InetAddress& listen_address);

        Acceptor(const Acceptor&) = delete;

        Acceptor(Acceptor&&) = delete;

        Acceptor& operator=(const Acceptor&) = delete;

        Acceptor& operator=(Acceptor&&) = delete;


        void setNewConnectionCallback(NewConnectionCallback cb) {
            new_connection_callback_ = std::move(cb);
        }

        bool listening()const {
            return is_listening_;
        }

        void listen();

    private:
        EventLoop& loop_;
        Socket accept_socket_;
        Channel accept_channel_;
        NewConnectionCallback new_connection_callback_;
        bool is_listening_;
    };
}
