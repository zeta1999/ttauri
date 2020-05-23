// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Foundation/os_detect.hpp"
#include "TTauri/Foundation/assert.hpp"
#include <type_traits>
#include <cstdint>

namespace TTauri {


using namespace std::literals;

/*! Invariant should be the default for variables.
* C++ does have an invariant but it requires you to enter the 'const' keyword which
* is easy to forget. Using a single keyword 'let' for an invariant makes it easier to notice
* when you have defined a variant.
*/
#ifndef let
#define let auto const
#endif

/*! Signed size/index into an array.
*/
using ssize_t = std::ptrdiff_t;

template<typename T>
force_inline std::remove_reference_t<T> rvalue_cast(T value)
{
    return value;
}


template<typename C>
struct nr_items {
    constexpr static ssize_t value = static_cast<ssize_t>(C::max) + 1;
};

template<typename C>
constexpr auto nr_items_v = nr_items<C>::value;

template <class C>
constexpr auto ssize(const C& c) -> std::common_type_t<ssize_t, std::make_signed_t<decltype(c.size())>> 
{
    using R = std::common_type_t<ssize_t, std::make_signed_t<decltype(c.size())>>;
    return static_cast<R>(c.size());
}

template <class T, ssize_t N>
constexpr ssize_t ssize(const T (&array)[N]) noexcept
{
    return N;
}

template <class C>
constexpr auto usize(const C& c) -> std::common_type_t<size_t, std::make_unsigned_t<decltype(c.size())>> 
{
    using R = std::common_type_t<size_t, std::make_unsigned_t<decltype(c.size())>>;
    return static_cast<R>(c.size());
}


#define ssizeof(x) (static_cast<ssize_t>(sizeof (x)))

}