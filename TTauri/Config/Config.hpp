// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "parser.hpp"

namespace TTauri::Config {

/*! Configuration
 * 
 */
struct Config {
    std::system::path path;
    ASTObject *ast;
    Value root;

    Location errorLocation;
    std::string errorMessage;

    /*! Load a configuration file.
     * See the README.md file is this directory for the file format of the configuration file.
     * \param path path to the configuration file.
     */
    Config(std::system::path const &path) : path(path) {
        try {
            ast = parseConfigFile(path);
            root = ast->execute();

        } catch (ConfigError &x) {
            if (auto const location=boost::get_error_info<errinfo_location>(e) )
                errorLocation = *location;
            }

            if (auto const message=boost::get_error_info<errinfo_message>(e) )
                errorMessage = *message;
            }
        }
    }

    ~Config() {
        delete ast;
    }

    /*! Parsing the configuration file was succesfull.
     */
    bool success() const {
        return !root.is_type<Undefined>();
    }

    /*! Retreive error message
     */
    std::string error() const {
        if (success()) {
            return ""s;
        } else {
            return (
                boost::format("%s:%i:%i: %s.") %
                errorLocation.file %
                errorLocation.line %
                errorLocation.column %
                errorMessage
            ).str();
        }
    }

    /*! string representation of the abstract-syntax-tree.
     */
    std::string astStr() const {
        if (ast) {
            return ast->str();
        } else {
            return "";
        }
    }

    /*! string representation of the configuration.
     */
    std::string str() const {
        if (success()) {
            return root.str();
        } else {
            return error();
        }
    }

    /*! Get a value from the configuration.
     * The key is a string; identifiers and integer indices
     * seperated by dots. To select items from nested objects
     * and arrays.
     *
     * The following types are supported:
     * - bool, int64_t, double, std::string, std::filesystem::path, Color_XYZ
     * - std::vector<std::any>, std::map<std::string, std::any>
     *
     * int64_t can be promoted to double.
     * std::string can be promoted to std::filesystem::path
     *
     * \param key A configuration key.
     */
    template<typename T>
    T value(std::string key) const {
        return root.get(key).value<T>();
    }

    /*! Get the root object.
     * \see value() for the different kinds of types that are supported.
     */
    std::map<std::string,std::any> rootObject() {
        return root.value<std::map<std::string,std::any>>();
    }

};

}
