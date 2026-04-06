#pragma once
namespace hyperMuduo{
    class CurrentThread {
    public:
        CurrentThread() = delete;

        ~CurrentThread() = delete;

        static int getTid() {
            if (current_thread_tid_ == 0) [[unlikely]]{
            cachedTid();
            }
            return current_thread_tid_;
        }

    private:
        static void cachedTid();

        inline static thread_local int current_thread_tid_ = 0;
    };
}
