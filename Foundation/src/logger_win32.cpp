// Copyright 2019 Pokitec
// All rights reserved.

#include "TTauri/Foundation/logger.hpp"
#include "TTauri/Foundation/trace.hpp"
#include "TTauri/Foundation/cpu_utc_clock.hpp"
#include "TTauri/Foundation/globals.hpp"
#include "TTauri/Foundation/required.hpp"
#include "TTauri/Foundation/URL.hpp"
#include "TTauri/Foundation/strings.hpp"
#include "TTauri/Foundation/thread.hpp"
#include <fmt/ostream.h>
#include <fmt/format.h>
#include <exception>
#include <memory>
#include <iostream>
#include <ostream>
#include <chrono>
#include <Windows.h>
#include <debugapi.h>

namespace TTauri {

using namespace std::literals::chrono_literals;

void logger_type::writeToConsole(std::string str) noexcept {
    str += "\r\n";
    OutputDebugStringW(translateString<std::wstring>(str).data());
}

[[noreturn]] void terminateOnFatalError(std::string &&message) noexcept {
    Foundation_globals->stopMaintenanceThread();

    if (IsDebuggerPresent()) {
        let messageWString = translateString<std::wstring>(message);
        OutputDebugStringW(messageWString.data());
        DebugBreak();

    } else {
        let longMessage = fmt::format(
            "Fatal error: {}.\n\n"
            "This is a serious bug in this application, please email support@pokitec.com with the error message above. "
            "Press OK to quit the application.",
            message
        );
        let messageWString = translateString<std::wstring>(longMessage);
        let captionWString = translateString<std::wstring>(std::string("Fatal error"));
        MessageBoxW(nullptr, messageWString.data(), captionWString.data(), MB_APPLMODAL | MB_OK | MB_ICONERROR);
    }
    std::terminate();
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
