// Copyright 2019 Pokitec
// All rights reserved.

#include "logger.hpp"
#include "strings.hpp"
#include "os_detect.hpp"
#include "URL.hpp"
#include <exception>
#include <memory>
#include <iostream>
#include <chrono>

#if OPERATING_SYSTEM == OS_WINDOWS
#include <Windows.h>
#include <debugapi.h>
#endif

namespace TTauri {

using namespace std::literals::chrono_literals;

logger_type::logger_type(bool test) noexcept {
    // The logger is the first object that will use the timezone database.
    // Zo we will initialize it here.
#if USE_OS_TZDB == 0
    let tzdata_location = URL::urlFromResourceDirectory() / "tzdata";
    date::set_install(tzdata_location.nativePath());
#endif
    current_time_zone = date::current_zone();

    if (!test) {
        logger_thread = std::thread([&]() {
            this->loop();
            });
    }
}

logger_type::~logger_type() {
    if (logger_thread.joinable()) {
        logger_thread_stop = true;
        logger_thread.join();
    }
}


void logger_type::writeToFile(std::string str) noexcept {
}

void logger_type::writeToConsole(std::string str) noexcept {
#if OPERATING_SYSTEM == OS_WINDOWS
    str += "\r\n";
    OutputDebugStringW(translateString<std::wstring>(str).data());
#else
    cerr << str << endl;
#endif
}

/*! Write to a log file and console.
 * This will write to the console if one is open.
 * It will also create a log file in the application-data directory.
 */
void logger_type::write(std::string const &str) noexcept {
    writeToFile(str);
    writeToConsole(str);
}

void logger_type::loop() noexcept {
    bool last_iteration = false;

    do {
        if (logger_thread_stop) {
            // We need to check the message queue one more time to log all messages
            // left in the queue before completely finishing.
            last_iteration = true;
        }

        auto found_fatal_message = false;
        while (!message_queue.empty()) {
            auto message = message_queue.read();

            let str = (*message)->string();
            if ((*message)->level() >= log_level::Fatal) {
                found_fatal_message = true;
            }

            write(str);
        }

        if (found_fatal_message) {
            logged_fatal_message.store(true);
        }
        std::this_thread::sleep_for(100ms);

    } while (!last_iteration);
}

gsl_suppress(i.11)
std::string getLastErrorMessage()
{
    DWORD const errorCode = GetLastError();
    size_t const messageSize = 32768;
    wchar_t* const c16_message = new wchar_t[messageSize];;

    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, // source
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        c16_message,
        messageSize,
        NULL
    );

    let message = translateString<std::string>(std::wstring(c16_message));
    delete [] c16_message;

    return message;
}

}
