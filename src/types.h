#pragma once

#include <stdint.h>  // TODO(ted): Check if always correct.
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint64_t uintn;  // Native-sized int.

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef int64_t intn;   // Native-sized int.

typedef float  f32;
typedef double f64;

typedef u8  char8;
typedef u16 char16;

typedef size_t  usize;
//typedef ssize_t isize;

typedef struct Array {
    u8*    data;
    size_t size;
} Array;

typedef struct String {
    char16*  data;
    size_t  size;
} String;

//#define LITERAL_TO_STRING(x) ((const String) { .data=(char16 *) L##x, .size=sizeof(x) })

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))


#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)
#define FILE_WCHAR WIDE1(__FILE__)
