// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "required.hpp"
#include <string>
#include <string_view>

namespace TTauri {

using string_tag = int64_t;

constexpr string_tag char_to_tag(char c) noexcept
{
    if (c == 0) {
        return 0;
    } else if (c >= 'a' && c <= 'z') {
        return (c - 'a') + 1;
    } else if (c >= 'A' && c <= 'Z') {
        return (c - 'A') + 1;
    } else if (c == '-' || c == '_' || c == ' ') {
        return 27;
    } else {
        no_default;
    }
}

constexpr char tag_to_char(string_tag tag) noexcept
{
    if (tag == 0) {
        return 0;
    } else if (tag < 27) {
        return static_cast<char>(tag - 1) + 'a';
    } else {
        return '-';
    }
}

/*! Creates a string_tag at compile time.
 * A string_tag can be used as a template parameter.
 *
 * A string_tag is an int64_t:
 *  * Zero is an empty string.
 *  * Positive values are a representation of a string up to 13 letters, lexical ordered.
 *  * Negative values may be used for custom purposes.
 *
 * Uppercase letters 'A'-'Z' are translated to lowercase letters 'a'-'z';
 * any other character is translated to a dash '-'
 */
constexpr string_tag string_to_tag(char const *str, size_t str_size) noexcept
{
    required_assert(str_size <= 13);

    string_tag r = 0;
    for (auto i = 0; i < 13; i++) {
        r *= 28;
        r += (i < str_size) ? char_to_tag(str[i]) : 0;
    }
    return r;
}

inline string_tag string_to_tag(std::string_view str) noexcept
{
    return string_to_tag(str.data(), str.size());
}

/*! Creates a string_tag at compile time.
 * \see string_to_tag(char const *str, size_t str_size)
 */
constexpr string_tag operator""_tag(char const *str, size_t str_size) noexcept
{
    return string_to_tag(str, str_size);
}

/*! Convert a string_tag to a string at runtime.
 * \see string_to_tag(char const *str, size_t str_size)
 */
inline std::string tag_to_string(string_tag tag) noexcept
{
    char tmp[14];
    tmp[13] = 0; // Add zero terminator when string is actually 13 letters long.

    for (auto i = 12; i >= 0; i--) {
        tmp[i] = tag_to_char(tag % 28);
        tag /= 28;
    }

    return {tmp};
}

};
