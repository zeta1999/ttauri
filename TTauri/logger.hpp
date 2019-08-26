// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "required.hpp"
#include "polymorphic_value.hpp"
#include "wfree_mpsc_message_queue.hpp"
#include "singleton.hpp"
#include "url_parser.hpp"
#include "atomic.hpp"
#include "counters.hpp"
#include "cpu_counter_clock.hpp"
#include "sync_clock.hpp"
#include <date/tz.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <string>
#include <string_view>
#include <tuple>

namespace TTauri {

#ifdef _WIN32
std::string getLastErrorMessage();
#endif

enum class log_level {
    //! Messages that are used for debugging during developmet.
    Debug,
    //! Informational messages used debugging problems in production by users of the application.
    Info,
    //! An exception was throw, probably isn't a problem.
    Exception,
    //! Messages for auditing purposes.
    Audit,
    //! An error was detected which is recoverable by the application.
    Warning,
    //! An error was detected and is recoverable by the user.
    Error,
    //! An error has caused data to be corrupted.
    Critical,
    //! Unrecoverable error, need to terminate the application to reduce impact.
    Fatal,
};

constexpr char const *to_const_string(log_level level) noexcept
{
    switch (level) {
    case log_level::Debug:     return "DEBUG";
    case log_level::Info:      return "INFO";
    case log_level::Exception: return "THROW";
    case log_level::Audit:     return "AUDIT";
    case log_level::Warning:   return "WARN";
    case log_level::Error:     return "ERROR";
    case log_level::Critical:  return "CRIT";
    case log_level::Fatal:     return "FATAL";
    default: no_default;
    }
}

constexpr bool operator<(log_level lhs, log_level rhs) noexcept { return static_cast<int>(lhs) < static_cast<int>(rhs); }
constexpr bool operator>(log_level lhs, log_level rhs) noexcept { return rhs < lhs; }
constexpr bool operator==(log_level lhs, log_level rhs) noexcept { return static_cast<int>(lhs) == static_cast<int>(rhs); }
constexpr bool operator!=(log_level lhs, log_level rhs) noexcept { return !(lhs == rhs); }
constexpr bool operator<=(log_level lhs, log_level rhs) noexcept { return !(lhs > rhs); }
constexpr bool operator>=(log_level lhs, log_level rhs) noexcept { return !(lhs < rhs); }

inline date::time_zone const *current_time_zone;

struct log_message_base {
    char const *source_path;
    int source_line;
    cpu_counter_clock::time_point timestamp;
    char const *format;

    log_message_base(char const *source_path, int source_line, cpu_counter_clock::time_point timestamp, char const *format) noexcept :
        source_path(source_path), source_line(source_line), timestamp(timestamp), format(format) {}

    virtual std::string string() const noexcept = 0;
    virtual log_level level() const noexcept = 0;
};

template<log_level Level, typename... Args>
struct log_message: public log_message_base {
    static constexpr char const *LogLevelName = to_const_string(Level);

    std::tuple<std::decay_t<Args>...> format_args;

    log_message(char const * const source_path, int const source_line, cpu_counter_clock::time_point const timestamp, char const *const format, Args &&... args) noexcept :
        log_message_base(source_path, source_line, timestamp, format), format_args(std::forward<Args>(args)...) {}

    log_level level() const noexcept override {
        return Level;
    }

    std::string string() const noexcept override {
        auto f = [format=this->format](auto const&... args) {
            return fmt::format(format, args...);
        };

        let msg = std::apply(f, format_args);

        let source_filename = filename_from_path(source_path);

        let utc_timestamp = hiperf_utc_clock::convert(timestamp);
        let local_timestring = format_full_datetime(utc_timestamp, current_time_zone);

        return fmt::format("{} {:5} {}.    {}:{}", local_timestring, LogLevelName, msg, source_filename, source_line);
    }
};


/*! A class with which to log messages to a file or console.
 * This will primarilly used with get_singleton<>().
 */
class logger_type {
    static constexpr size_t MAX_MESSAGE_SIZE = 224;
    static constexpr size_t MESSAGE_ALIGNMENT = 256;
    static constexpr size_t MAX_NR_MESSAGES = 4096;

    using message_type = polymorphic_value<log_message_base,MAX_MESSAGE_SIZE>;
    using message_queue_type = wfree_mpsc_message_queue<message_type,MAX_NR_MESSAGES,MESSAGE_ALIGNMENT>;

    std::atomic<bool> logged_fatal_message = false;

    //! the message queue must work correctly before main() is executed.
    message_queue_type message_queue;

    bool logger_thread_stop = false;
    std::thread logger_thread;

public:
    log_level minimum_log_level = log_level::Debug;

    logger_type(bool test=false) noexcept;

    ~logger_type();

    void loop() noexcept;

    template<log_level Level, typename... Args>
    void log(char const * const source_file, int const source_line, char const * const format, Args &&... args) noexcept {
        if (Level < minimum_log_level) {
            return;
        }

        // Add messages in the queue, block when full.
        // * This reduces amount of instructions needed to be executed during logging.
        // * Simplifies logged_fatal_message logic.
        // * Will make sure everything gets logged.
        // * Blocking is bad in a real time thread, so maybe count the number of times it is blocked.
        {
            let timestamp = cpu_counter_clock::now();

            auto message = message_queue.write<"logger_block"_tag>();
            // derefence the message so that we get the polymorphic_value, so this assignment will work correctly.
            message->emplace<log_message<Level, Args...>>(source_file, source_line, timestamp, format, std::forward<Args>(args)...);
        }

        if constexpr (Level >= log_level::Fatal) {
            // Wait until the logger-thread is finished.
            wait_for_transition(logged_fatal_message, true);
            std::terminate();
        }
    }


private:
    void write(std::string const &str) noexcept;
    void writeToFile(std::string str) noexcept;
    void writeToConsole(std::string str) noexcept;
};

// The constructor of logger only starts the logging thread.
// The ring buffer of the logger is trivaliy constructed and can be used before the logger's constructor is stared.
inline logger_type logger = {};

}

constexpr char const *foo(char const *str)
{
    return static_cast<char const *>(str);
}

#define LOG_DEBUG(...) logger.log<log_level::Debug>(__FILE__, __LINE__, __VA_ARGS__);
#define LOG_INFO(...) logger.log<log_level::Info>(__FILE__, __LINE__, __VA_ARGS__);
#define LOG_AUDIT(...) logger.log<log_level::Audit>(__FILE__, __LINE__, __VA_ARGS__);
#define LOG_WARNING(...) logger.log<log_level::Warning>(__FILE__, __LINE__, __VA_ARGS__);
#define LOG_ERROR(...) logger.log<log_level::Error>(__FILE__, __LINE__, __VA_ARGS__);
#define LOG_CRITICAL(...) logger.log<log_level::Critical>(__FILE__, __LINE__, __VA_ARGS__);
#define LOG_FATAL(...) logger.log<log_level::Fatal>(__FILE__, __LINE__, __VA_ARGS__);
