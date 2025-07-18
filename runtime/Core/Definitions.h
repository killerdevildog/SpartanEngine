/*
Copyright(c) 2015-2025 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

struct sp_info
{
    static constexpr char name[]           = "Spartan";
    static constexpr int  version_major    = 0;
    static constexpr int  version_minor    = 3;
    static constexpr int  version_revision = 3;
};

//= OPTIMISATION ON/OFF ================================================
#if defined(_MSC_VER)
    #define SP_OPTIMISE_OFF __pragma(optimize("", off))
    #define SP_OPTIMISE_ON  __pragma(optimize("", on))
#elif defined(__clang__)
    #define SP_OPTIMISE_OFF _Pragma("clang optimize off")
    #define SP_OPTIMISE_ON  _Pragma("clang optimize on")
#elif defined(__GNUC__) || defined(__GNUG__)
    #define SP_OPTIMISE_OFF         \
        _Pragma("GCC push_options") \
        _Pragma("GCC optimize (\"O0\")")
    #define SP_OPTIMISE_ON _Pragma("GCC pop_options")
#else
    #error "SP_OPTIMISE_* is not implemented for this compiler/platform"
#endif
//======================================================================

//= WARNINGS ON/OFF ====================================================
#if defined(_MSC_VER)
    #define SP_WARNINGS_OFF __pragma(warning(push, 0))
    #define SP_WARNINGS_ON  __pragma(warning(pop))
#elif defined(__clang__)
    #define SP_WARNINGS_OFF              \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Weverything\"")
    #define SP_WARNINGS_ON _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
    #define SP_WARNINGS_OFF                         \
        _Pragma("GCC diagnostic push")              \
        _Pragma("GCC diagnostic ignored \"-Wall\"") \
        _Pragma("GCC diagnostic ignored \"-Wextra\"") 
    #define SP_WARNINGS_ON _Pragma("GCC diagnostic pop")
#else
    #error "SP_WARNINGS_* is not implemented for this compiler/platform"
#endif
//======================================================================

//= DEBUG BREAK =========================================================
#if defined(_MSC_VER)
#define SP_DEBUG_BREAK() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
    #define SP_DEBUG_BREAK() __builtin_trap()
#else
    #error "SP_DEBUG_BREAK is not implemented for this compiler/platform"
#endif
//=======================================================================

//= WINDOWS ===================================================================================
#define WIDE_STR_HELPER(x) L ## x
#define WIDE_STR(x) WIDE_STR_HELPER(x)

#if defined(_WIN32)
    #define SP_INFO_WINDOW(text_message)                                                      \
    {                                                                                         \
        MessageBeep(MB_ICONINFORMATION);                                                      \
        HWND hwnd = GetConsoleWindow();                                                       \
        MessageBoxW(hwnd, L##text_message, L"Info", MB_OK | MB_TOPMOST | MB_ICONINFORMATION); \
    }
#else
    #define SP_INFO_WINDOW(text_message)    \
    {                                       \
        printf("Info: %s\n", text_message); \
    }
#endif

#if defined(_WIN32)
    #define SP_WARNING_WINDOW(text_message)                                                  \
    {                                                                                        \
        MessageBeep(MB_ICONWARNING);                                                         \
        HWND hwnd = GetConsoleWindow();                                                      \
        MessageBoxW(hwnd, L##text_message, L"Warning", MB_OK | MB_TOPMOST | MB_ICONWARNING); \
    }
#else
    #define SP_WARNING_WINDOW(text_message)    \
    {                                          \
        printf("Warning: %s\n", text_message); \
    }
#endif

#if defined(_WIN32)
    #define SP_ERROR_WINDOW(text_message)                                                \
    {                                                                                    \
        MessageBeep(MB_ICONERROR);                                                       \
        HWND hwnd = GetConsoleWindow();                                                  \
        MessageBoxW(hwnd, L##text_message, L"Error", MB_OK | MB_TOPMOST | MB_ICONERROR); \
        SP_DEBUG_BREAK();                                                                \
    }
#else
    #define SP_ERROR_WINDOW(text_message)    \
    {                                        \
        printf("Error: %s\n", text_message); \
        SP_DEBUG_BREAK();                    \
    }
#endif
//=============================================================================================

//= STACKTRACE========================
namespace spartan
{
    const char* get_callstack_c_str();
}
//====================================

//= ASSERT =====================================================================
#include <cassert>
#define SP_ASSERT(expression)                                         \
if (!(expression))                                                    \
{                                                                     \
    spartan::Log::SetLogToFile(true);                                 \
    SP_LOG_ERROR("Assertion failed: " #expression);                   \
    SP_LOG_ERROR("Callstack:\n%s",    spartan::get_callstack_c_str());\
    assert(expression);                                               \
}

#define SP_ASSERT_MSG(expression, text_message)                       \
if (!(expression))                                                    \
{                                                                     \
    spartan::Log::SetLogToFile(true);                                 \
    SP_LOG_ERROR("Assertion failed: " #expression);                   \
    SP_LOG_ERROR("Message: %s",       text_message);                  \
    SP_LOG_ERROR("Callstack:\n%s",    spartan::get_callstack_c_str());\
    assert(expression && text_message);                               \
}

// a static assert
#define SP_ASSERT_STATIC_IS_TRIVIALLY_COPYABLE(T) \
static_assert(std::is_trivially_copyable_v<T>, "Type is not trivially copyable")
//==============================================================================

#if defined(_MSC_VER)

// avoid conflicts with numeric limit min/max 
#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif
