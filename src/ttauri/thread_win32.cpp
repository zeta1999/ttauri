// Copyright 2019 Pokitec
// All rights reserved.

#include "thread.hpp"
#include "strings.hpp"
#include "Application.hpp"

#include <Windows.h>

namespace tt {

void set_thread_name(std::string_view name)
{
    current_thread_id = thread_id_count.fetch_add(1, std::memory_order::memory_order_relaxed) + 1;

    ttlet wname = to_wstring(name);
    SetThreadDescription(GetCurrentThread(), wname.data());
#elif  TT_OPERATING_SYSTEM == TT_OS_MACOS
    pthread_setname_np(name.cstr());
#elif  TT_OPERATING_SYSTEM == TT_OS_LINUX
    pthread_setname_np(pthread_self(), name.cstr());
#endif
}

bool is_main_thread()
{
    tt_assume(application);
    return std::this_thread::get_id() == application->mainThreadID;
}

void run_on_main_thread(std::function<void()> f)
{
    if (is_main_thread()) {
        return f();

    } else {
        tt_assume(application);
        application->runOnMainThread(f);
    }
}

}