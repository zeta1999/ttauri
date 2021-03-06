// Copyright Take Vos 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "formula_binary_operator_node.hpp"
#include "formula_post_process_context.hpp"
#include "formula_name_node.hpp"

namespace tt {

struct formula_filter_node final : formula_binary_operator_node {
    mutable formula_post_process_context::filter_type filter;
    formula_name_node* rhs_name;

    formula_filter_node(parse_location location, std::unique_ptr<formula_node> lhs, std::unique_ptr<formula_node> rhs) :
        formula_binary_operator_node(std::move(location), std::move(lhs), std::move(rhs))
    {
        rhs_name = dynamic_cast<formula_name_node*>(this->rhs.get());
        if (rhs_name == nullptr) {
            tt_error_info().set<"parse_location">(location);
            throw parse_error(fmt::format("Expecting a name token on the right hand side of a filter operator. got {}.", rhs));
        }
    }

    void post_process(formula_post_process_context& context) override {
        formula_binary_operator_node::post_process(context);

        filter = context.get_filter(rhs_name->name);
        if (!filter) {
            tt_error_info().set<"parse_location">(location);
            throw parse_error("Could not find filter .{}().", rhs_name->name);
        }
    }

    datum evaluate(formula_evaluation_context& context) const override {
        auto lhs_ = lhs->evaluate(context);
        try {
            return {filter(static_cast<std::string>(lhs_))};
        } catch (...) {
            error_info(true).set<"parse_location">(location);
            throw;
        }
    }

    std::string string() const noexcept override {
        return fmt::format("({} ! {})", *lhs, *rhs);
    }
};

}
