// Copyright Take Vos 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "observable_base.hpp"

namespace tt::detail {

template<typename T, typename OT>
class observable_unary : public observable_base<T> {
public:
    using operand_type = observable_base<OT>;

    observable_unary(std::shared_ptr<operand_type> const &operand) noexcept :
        observable_base<T>(),
        _operand(operand),
        _operand_cache(operand->load())
    {
        _operand_callback = _operand->subscribe([this]() {
            ttlet old_value = this->load();
            {
                ttlet lock = std::scoped_lock(observable_base<T>::_mutex);
                _operand_cache = _operand->load();
            }
            ttlet new_value = this->load();
            observable_base<T>::notify(old_value, new_value);
        });
    }

protected:
    std::shared_ptr<operand_type> _operand;
    OT _operand_cache;
    typename operand_type::callback_ptr_type _operand_callback;
};

}
