#pragma once

#define BIT_SET(x, bit)    ((x) |= (typeof(x))  (1ULL << (bit)))
#define BIT_CLEAR(x, bit)  ((x) &= (typeof(x)) ~(1ULL << (bit)))
#define BIT_FLIP(x, bit)   ((x) ^= (typeof(x))  (1ULL << (bit)))
#define BIT_CHECK(x, bit)  (!!((x) & (1ULL << (bit))))

#define BITMASK_SET(x, mask)        ((x) |=   (mask))
#define BITMASK_CLEAR(x, mask)      ((x) &= (~(mask)))
#define BITMASK_FLIP(x, mask)       ((x) ^=   (mask))
#define BITMASK_CHECK_ALL(x, mask)  (!(~(x) & (mask)))
#define BITMASK_CHECK_ANY(x, mask)  ((x) &    (mask))
