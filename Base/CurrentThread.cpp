#include "CurrentThread.hpp"

#include <unistd.h>
#include <sys/syscall.h>

void hyperMuduo::CurrentThread::cachedTid() {
    current_thread_tid_ = static_cast<int>(::syscall(SYS_gettid));
}


