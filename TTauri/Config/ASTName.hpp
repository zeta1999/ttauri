// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "ASTExpression.hpp"
#include "ASTObject.hpp"
#include "parser.hpp"

namespace TTauri::Config {

struct ASTName : ASTExpression {
    std::string name;

    ASTName(Location location, char *name) : ASTExpression(location), name(name) {
        free(name);
    }

    std::string str() const override {
        return name;
    }

    Value &executeLValue(ExecutionContext *context) const override {
        return context->currentObject()[name];
    } 

    Value &executeAssignment(ExecutionContext *context, Value other) const override {
        auto &lv = context->currentObject()[name];
        lv = std::move(other);
        return lv;
    }

    template<typename T>
    T getArgument(std::vector<Value> const &arguments, size_t i, bool lastArgument=false) const {
        if (i >= arguments.size()) {
            BOOST_THROW_EXCEPTION(InvalidOperationError((boost::format("syntax error, not enough arguments to function '%', expecting argument number %i of type %s")
                % name % (i + 1) % typeid(T).name()).str())
                << errinfo_location(location)
            );
        }

        auto const argument = arguments.at(0);
        if (!argument.is_promotable_to<T>()) {
            BOOST_THROW_EXCEPTION(InvalidOperationError((boost::format("syntax error, invalid argument to function '%s', expecting argument number %i of type %s got %s")
                % name % (i + 1) % typeid(T).name() % argument.type().name()).str())
                << errinfo_location(location)
            );
        }

        if (lastArgument && i != (arguments.size() - 1)) {
            BOOST_THROW_EXCEPTION(InvalidOperationError((boost::format("syntax error, too many arguments to function '%', expecting %i arguments got %i")
                % name % (i + 1) % arguments.size()).str())
                << errinfo_location(location)
            );
        }

        return argument.value<T>();
    }

    /*! Include a configuration file.
     */
    Value executeIncludeCall(ExecutionContext *context, std::vector<Value> const &arguments) const {
        auto path = getArgument<std::filesystem::path>(arguments, 0, true);

        // The included file is relative to the directory of this configuration file.
        if (path.is_relative()) {
            path = location.file->parent_path() / path;
        }

        try {
            auto const ast = std::unique_ptr<ASTObject>{parseConfigFile(path)};
            return ast->execute();

        } catch (ConfigError &e) {
            std::string errorMessage;
            if (auto const previousErrorMessage = boost::get_error_info<errinfo_previous_error_message>(e)) {
                errorMessage += *previousErrorMessage + "\n";
            }

            if (auto const location = boost::get_error_info<errinfo_location>(e)) {
                errorMessage += location->str() + ": ";
            }

            errorMessage += e.what();
            errorMessage += ".";

            BOOST_THROW_EXCEPTION(InvalidOperationError((boost::format("Could not include file '%s'") % path.generic_string()).str())
                << errinfo_location(location)
                << errinfo_previous_error_message(errorMessage)
            );
        }
    }

    /*! Return a absolute path relative to the directory where this configuration file is located.
    */
    Value executePathCall(ExecutionContext *context, std::vector<Value> const &arguments) const {
        if (arguments.size() == 0) {
            // Without arguments return the directory where this configuration file is located.
            return location.file->parent_path();
        } else {
            // Suffix the given argument with the directory where this configuration file is located.
            auto const path = getArgument<std::filesystem::path>(arguments, 0, true);

            if (path.is_relative()) {
                return location.file->parent_path() / path;
            } else {
                return path;
            }
        }
    }

    /*! Return a absolute path relative to the current working directory.
     */
    Value executeCwdCall(ExecutionContext *context, std::vector<Value> const &arguments) const {
        if (arguments.size() == 0) {
            // Without argument return the current working directory.
            return std::filesystem::current_path();

        } else {
            // Suffix the given argument with the current working directory.
            auto const path = getArgument<std::filesystem::path>(arguments, 0, true);

            if (path.is_relative()) {
                return std::filesystem::current_path() / path;
            } else {
                BOOST_THROW_EXCEPTION(InvalidOperationError((boost::format("Expecting relative path argument to function '%s' got '%s'")
                    % name % path.string()).str())
                    << errinfo_location(location)
                );
            }
        }
    }

    /*! A function call.
     * The expression is a identifier followed by a call; therefor this is a normal function call.
     */
    Value executeCall(ExecutionContext *context, std::vector<Value> const &arguments) const override {
        if (name == "include") {
            return executeIncludeCall(context, arguments);

        } else if (name == "path") {
            return executePathCall(context, arguments);

        } else if (name == "cwd") {
            return executeCwdCall(context, arguments);

        } else {
            BOOST_THROW_EXCEPTION(InvalidOperationError((boost::format("Unknown function '%'") % name).str())
                << errinfo_location(location)
            );
        }
    }

};

}
