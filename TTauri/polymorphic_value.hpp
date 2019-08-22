// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "required.hpp"
#include <array>
#include <memory>

namespace TTauri {

template<typename T, size_t S>
class polymorphic_value {
private:
    using base_type = T;
    static constexpr size_t capacity = S;

    std::array<std::byte,capacity> _data;

    /*! Size of the object stored in the item.
    * Special values:
    *  * 0 - Empty
    */
    size_t _size;

public:
    polymorphic_value() : _size(0) {
    }

    ~polymorphic_value()
    {
        if (_size > 0) {
            std::destroy_at(this->operator->());
        }
    }

    polymorphic_value(polymorphic_value const &other) = delete;
    polymorphic_value(polymorphic_value &&other) = delete;
    polymorphic_value &operator=(polymorphic_value const &other) = delete;
    polymorphic_value &operator=(polymorphic_value &&other) = delete;

    template<typename O>
    std::enable_if_t<std::is_base_of_v<base_type, O>, polymorphic_value> &operator=(O const &other) {
        static_assert(sizeof(O) <= capacity, "Assignment of a type larger than capacity of polymorphic_value");
        reset();
        new(data()) O(other);
        _size = sizeof(O);
        return *this;
    }

    template<typename O>
    std::enable_if_t<std::is_base_of_v<base_type, O>, polymorphic_value> &operator=(O &&other) {
        static_assert(sizeof(O) <= capacity, "Assignment of a type larger than capacity of polymorphic_value");
        reset();
        new(data()) O(std::forward<O>(other));
        _size = sizeof(O);
        return *this;
    }

    template<typename O, typename... Args>
    std::enable_if_t<std::is_base_of_v<base_type, O>, void> emplace(Args &&... args) {
        static_assert(sizeof(O) <= capacity, "Assignment of a type larger than capacity of polymorphic_value");
        reset();
        new(data()) O(std::forward<Args>(args)...);
        _size = sizeof(O);
    }

    void reset() noexcept {
        if (_size > 0) {
            std::destroy_at(this->operator->());
        }
        _size = 0;
    }

    void *data() noexcept {
        return reinterpret_cast<void *>(_data.data());
    }

    void const *data() const noexcept {
        return reinterpret_cast<void const *>(_data.data());
    }

    size_t size() const noexcept {
        return _size;
    }

    T const &operator*() const noexcept {
        required_assert(size() > 0);
        return *reinterpret_cast<T const *>(data());
    }

    T &operator*() noexcept {
        required_assert(size() > 0);
        return *reinterpret_cast<T *>(data());
    }

    T const *operator->() const noexcept {
        required_assert(size() > 0);
        return reinterpret_cast<T const *>(data());
    }

    T *operator->() noexcept {
        required_assert(size() > 0);
        return reinterpret_cast<T *>(data());
    }
};

}