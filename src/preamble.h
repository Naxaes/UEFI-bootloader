#pragma once

#include <stdint.h>  /* The sized integer types */
#include <stddef.h>  /* size_t */
#include <stdarg.h>  /* va_arg */

/* Some sources I've taken from:
https://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit
https://dev.to/rdentato/
*/

/* ---- MACRO UTILITIES ----  */
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*(x)))

/* NOTE(ted): Needed to evaluate other macros.  */
#define INTERNAL_CONCAT_HELP(x, y)  x ## y
#define INTERNAL_CONCATENATE(x, y)  INTERNAL_CONCAT_HELP(x, y)
#ifndef __COUNTER__
    /* NOTE(ted): Can cause conflicts if used multiple times at the same line, even
        if the lines are in different files. */
    #define UNIQUE_NAME(name)  INTERNAL_CONCATENATE(name ## _, __func__)
#else
    #define UNIQUE_NAME(name)  INTERNAL_CONCATENATE(name ## _, __COUNTER__)
#endif


#define STATIC_ASSERT(x) extern int UNIQUE_NAME(STATIC_ASSERTION)[(x) ? 1 : -1]

#define NO_DEFAULT      default: ERROR(NO_DEFAULT, L"Default case was unexpectedly hit.")
#define INVALID_PATH    ERROR(INVALID_PATH, L"Invalid path.")
#define NOT_IMPLEMENTED ERROR(NOT_IMPLEMENTED, L"Not implemented.")


/* ---- BIT MANIPULATION ---- */
#define BIT_SET(x, bit)    ((x) |=   (1ULL << (bit)))
#define BIT_CLEAR(x, bit)  ((x) &=  ~(1ULL << (bit)))
#define BIT_FLIP(x, bit)   ((x) ^=   (1ULL << (bit)))
#define BIT_CHECK(x, bit)  (!!((x) & (1ULL << (bit))))

#define BITMASK_SET(x, mask)        ((x) |=   (mask))
#define BITMASK_CLEAR(x, mask)      ((x) &= (~(mask)))
#define BITMASK_FLIP(x, mask)       ((x) ^=   (mask))
#define BITMASK_CHECK_ALL(x, mask)  (!(~(x) & (mask)))
#define BITMASK_CHECK_ANY(x, mask)  ((x) &    (mask))


/* ---- MACROS ---- */
#if __has_builtin(__builtin_expect)
    #define UNLIKELY(expression) __builtin_expect(!!(expression), 0)
    #define LIKELY(expression)   __builtin_expect(!!(expression), 1)
#else
    #define UNLIKELY(expression) expression
    #define LIKELY(expression)   expression
#endif

#define MAKE_WIDE_STRING_PREFIX(x) L##x
#define MAKE_WIDE_STRING(x) MAKE_WIDE_STRING_PREFIX(x)
#define FILE_WCHAR MAKE_WIDE_STRING(__FILE__)


/* ---- ENUMS ----
Generate enums and matching strings by writing a macro that takes a parameter F,
and then define your enums by writing F(<name>).

    // Define
    #define FOR_EACH_MY_ENUM(F) \
        F(MY_ENUM_1) \
        F(MY_ENUM_2) \
        ...

    // Declare
    typedef enum { FOR_EACH_MY_ENUM(GENERATE_ENUM) } MyThing;
    static String MY_ENUM_STRINGS[] = { FOR_EACH_MY_ENUM(GENERATE_STRING) };

You can also declare your own macro `F` for other types of generation. For
example:

    // Generate a new type for each enum.
    #define GENERATE_STRUCTS(name) typedef struct name { } name;
    // Generate global constants for your enums.
    #define GENERATE_CONSTANTS(name) const u8 MY_THING_ ## name = name;
*/
#define GENERATE_ENUM(name) name,
#define GENERATE_STRING(name) (String) { #name , sizeof(#name) },


/* ---- DEFAULT ARGUMENTS ----
Wrap the function in a variadic macro that calls to WITH_DEFAULTS with a
name and __VA_ARGS__. For each parameter passed in it'll now dispatch
to macros (or functions) named "nameX", where X is the number of parameters.

    // Define the dispatcher.
    #define greet(...) WITH_DEFAULTS(greet, __VA_ARGS__)

    // Define the overloaded functions (or function-like macros) you want.
    #define greet1(name)           printf("%s %s!", "Hello", name)
    #define greet2(greeting, name) printf("%s %s!", greeting, name)

    // Call.
    greet("Sailor");                      // printf("%s %s!", "Hello",     "Sailor");
    greet("Greetings", "Sailor");         // printf("%s %s!", "Greetings", "Sailor");
    greet("Greetings", "Sailor", "!!!");  // Error: greet3 is not defined.

This is restricted to a minimum of 1 argument and a maximum of 8.
*/
#define POP_10TH_ARG(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, argN, ...) argN
#define VA_ARGS_COUNT(...)  POP_10TH_ARG(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define SELECT_FUNCTION(function, postfix) CONCATENATE(function, postfix)
#define WITH_DEFAULTS(f, ...) SELECT_FUNCTION(f, VA_ARGS_COUNT(__VA_ARGS__))(__VA_ARGS__)


/* ---- FORMATTING ----
`printf` doesn't support printing in binary. These macros can be used to
accomplish that.

    u64 x = ...;
    printf(BINARY_FORMAT_PATTERN_64 "\n", BYTE_TO_BINARY_CHARS_64(x));
*/
#ifndef BINARY_FORMAT_PATTERN_PREFIX
    #define BINARY_FORMAT_PATTERN_PREFIX L"0b"
#endif
#ifndef BINARY_FORMAT_PATTERN_DELIMITER
    #define BINARY_FORMAT_PATTERN_DELIMITER L"_"
#endif

#define BINARY_FORMAT_PATTERN_8  BINARY_FORMAT_PATTERN_PREFIX L"%c%c%c%c%c%c%c%c"
#define BINARY_FORMAT_PATTERN_16 BINARY_FORMAT_PATTERN_PREFIX L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c"
#define BINARY_FORMAT_PATTERN_32 BINARY_FORMAT_PATTERN_PREFIX L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c"
#define BINARY_FORMAT_PATTERN_64 BINARY_FORMAT_PATTERN_PREFIX "%c%c%c%c%c%c%c%cL" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c" BINARY_FORMAT_PATTERN_DELIMITER L"%c%c%c%c%c%c%c%c"

#define BYTE_TO_BINARY_CHARS_8(x)  BIT_CHECK((u8)(x), 7) ? '1' : '0', BIT_CHECK((u8)(x), 6) ? '1' : '0', BIT_CHECK((u8)(x), 5) ? '1' : '0', BIT_CHECK((u8)(x), 4) ? '1' : '0', BIT_CHECK((u8)(x), 3) ? '1' : '0', BIT_CHECK((u8)(x), 2) ? '1' : '0', BIT_CHECK((u8)(x), 1) ? '1' : '0',  BIT_CHECK((u8)(x), 0) ? '1' : '0'
#define BYTE_TO_BINARY_CHARS_16(x) BYTE_TO_BINARY_CHARS_8(((u16)  (x)) >> 8),  BYTE_TO_BINARY_CHARS_8(((u16)  (x)))
#define BYTE_TO_BINARY_CHARS_32(x) BYTE_TO_BINARY_CHARS_16(((u32) (x)) >> 16), BYTE_TO_BINARY_CHARS_16(((u32) (x)))
#define BYTE_TO_BINARY_CHARS_64(x) BYTE_TO_BINARY_CHARS_32(((u64) (x)) >> 32), BYTE_TO_BINARY_CHARS_32(((u64) (x)))


