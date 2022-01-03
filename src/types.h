#pragma once

#include <stdint.h>  /* The sized integer types */
#include <stddef.h>  /* size_t */

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef float  f32;
typedef double f64;

typedef size_t usize;

typedef u32  rune;  /* Unicode code point   */
typedef char utf8;  /* Unicode byte         */

typedef i8 bool;
#define true  1
#define false 0