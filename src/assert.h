#pragma once

/* ---- DEBUGGING, LOGGING AND ASSERTIONS ---- */
#if defined(USE_WIDE_CHARACTER) && USE_WIDE_CHARACTER == 1
    #define TO_LONG_STRING_HELPER(x) L##x
    #define TO_LONG_STRING(x, ...) TO_LONG_STRING_HELPER(x)

    #ifndef STANDARD_LOGGER
        extern int printf(const short unsigned int* format, ...);
        #define STANDARD_LOGGER(...) printf(TO_LONG_STRING(__VA_ARGS__))
    #endif
    #ifndef ERROR_LOGGER
        extern int printf(const short unsigned int* format, ...);
        #define ERROR_LOGGER(...) printf(TO_LONG_STRING(__VA_ARGS__))
    #endif
    #ifndef DEBUG_BREAK
        extern void debug_break();
        #define DEBUG_BREAK() debug_break()
    #endif

    #define MAKE_WIDE_STRING_PREFIX(x) L##x
    #define MAKE_WIDE_STRING(x) MAKE_WIDE_STRING_PREFIX(x)
    #define FILE_WCHAR MAKE_WIDE_STRING(__FILE__)

    #define HEADER(group)      STANDARD_LOGGER("%s:%d [" #group "]: ", FILE_WCHAR, __LINE__)
    #define ERR_HEADER(group)  ERROR_LOGGER("%s:%d [" #group "]: ", FILE_WCHAR, __LINE__)

    #define LOG(string)       (HEADER(LOG), STANDARD_LOGGER(string "\n\r"))
    #define LOGF(format, ...) (HEADER(LOG), STANDARD_LOGGER(format "\n\r", __VA_ARGS__))

    #define ERROR(group, string)       (ERR_HEADER(group), ERROR_LOGGER(string "\n\r"), DEBUG_BREAK())
    #define ERRORF(group, format, ...) (ERR_HEADER(group), ERROR_LOGGER(format "\n\r", __VA_ARGS__), DEBUG_BREAK())

    #define PANIC(string)       ERROR(PANIC, string)
    #define PANICF(format, ...) ERRORF(PANIC, format, __VA_ARGS__)

    #define ASSERT(x)       ((x) ? 0 : (ERROR(ASSERT, "'" #x "' is false.\n")))
    #define ASSERTF(x, ...) ((x) ? 0 : (ERR_HEADER(ASSERT), ERROR_LOGGER("%s", "'" #x "' is false. "),  ERROR_LOGGER(__VA_ARGS__), ERROR_LOGGER("\n\r"), DEBUG_BREAK()))
#else
    #ifndef STANDARD_LOGGER
        extern int printf(const char* format, ...);
        #define STANDARD_LOGGER(...) printf(__VA_ARGS__)
    #endif
    #ifndef ERROR_LOGGER
        extern int printf(const char* format, ...);
        #define ERROR_LOGGER(...) printf(__VA_ARGS__)
    #endif
    #ifndef DEBUG_BREAK
        extern void debug_break();
        #define DEBUG_BREAK() debug_break()
    #endif

    #define HEADER(group)      STANDARD_LOGGER("%s:%d [" #group "]: ", __FILE__, __LINE__)
    #define ERR_HEADER(group)  ERROR_LOGGER("%s:%d [" #group "]: ", __FILE__, __LINE__)

    #define LOG(string)       (HEADER(LOG), STANDARD_LOGGER(string "\n"))
    #define LOGF(format, ...) (HEADER(LOG), STANDARD_LOGGER(format "\n", __VA_ARGS__))

    #define ERROR(group, string)       (ERR_HEADER(group), ERROR_LOGGER(string "\n"), DEBUG_BREAK())
    #define ERRORF(group, format, ...) (ERR_HEADER(group), ERROR_LOGGER(format "\n", __VA_ARGS__), DEBUG_BREAK())

    #define PANIC(string)       ERROR(PANIC, string)
    #define PANICF(format, ...) ERRORF(PANIC, format, __VA_ARGS__)

    #define ASSERT(x)       ((x) ? 0 : (ERROR(ASSERT, "'" #x "' is false.\n")))
    #define ASSERTF(x, ...) ((x) ? 0 : (ERR_HEADER(ASSERT), ERROR_LOGGER("%s", "'" #x "' is false. "),  ERROR_LOGGER(__VA_ARGS__), ERROR_LOGGER("\n"), DEBUG_BREAK()))
#endif
