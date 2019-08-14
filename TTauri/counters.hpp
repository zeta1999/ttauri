// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "required.hpp"
#include "string_tag.hpp"
#include "wfree_unordered_map.hpp"
#include <gsl/gsl>

namespace TTauri {

constexpr int MAX_NR_COUNTERS = 1000;

inline auto &get_counter_map() noexcept
{
    static wfree_unordered_map<MAX_NR_COUNTERS, string_tag, std::atomic<int64_t> *> counter_map = {};
    return counter_map;
}

//inline std::vector<string_tag> get_counter_tags()
//{
//    auto counter_map = get_counter_map();
//    return counter_map.keys();
//}

template<string_tag TAG>
struct counter_functor {
    // Make sure non of the counters are false sharing cache-lines.
    alignas(std::hardware_destructive_interference_size)
    inline static std::atomic<int64_t> counter = 0;

    int64_t increment() const noexcept {
        let value = counter.fetch_add(1, std::memory_order_relaxed);

        if (value == 0) {
            auto &counter_map = get_counter_map();
            counter_map.insert(TAG, &counter);
        }

        return value + 1;
    }

    int64_t read() const noexcept {
        return counter.load(std::memory_order_relaxed);
    }

    // Don't implement readAndSet, a set to zero would cause the counters to be reinserted.
};

template<string_tag TAG>
inline int64_t increment_counter() noexcept 
{
    return counter_functor<TAG>{}.increment();
}

template<string_tag TAG>
inline int64_t read_counter() noexcept
{
    return counter_functor<TAG>{}.read();
}

inline int64_t read_counter(string_tag tag) noexcept
{
    auto &counter_map = get_counter_map();
    if (let value = counter_map.get(tag)) {
        return (*value)->load(std::memory_order_relaxed);
    } else {
        // The counter may have not been incremented yet,
        // thus it was not yet added to the map.
        // which means the counter value is still zero.
        return 0;    
    }
}

inline int64_t read_counter(std::string_view name) noexcept
{
    return read_counter(string_to_tag(name));
}

}
