// Copyright 2020 Pokitec
// All rights reserved.

#pragma once

#include "required.hpp"
#include <atomic>
#include <memory>
#include <thread>

namespace tt {
struct unfair_lock_wrap;

class unfair_mutex {
#if TT_OPERATING_SYSTEM == TT_OS_WINDOWS
    std::atomic<int32_t> semaphore = 0;

    int32_t *semaphore_ptr() noexcept {
        return reinterpret_cast<int32_t *>(&semaphore);
    }

    tt_no_inline void lock_contented(int32_t first) noexcept;

#elif TT_OPERATING_SYSTEM == TT_OS_MACOS
    std::unique_ptr<unfair_lock_wrap> mutex;

#else
#error "Not implemented fast_mutex"
#endif

#if TT_BUILD_TYPE == TT_BT_DEBUG
    std::thread::id locking_thread;
#endif

public:
    unfair_mutex(unfair_mutex const &) = delete;
    unfair_mutex &operator=(unfair_mutex const &) = delete;

    unfair_mutex() noexcept;
    ~unfair_mutex();

    void lock() noexcept;

    /**
     * When try_lock() is called from a thread that already owns the lock it will
     * return false.
     *
     * Calling try_lock() in a loop will bypass the operating system's wait system,
     * meaning that no priority inversion will take place.
     */
    bool try_lock() noexcept;

    void unlock() noexcept;
};


};