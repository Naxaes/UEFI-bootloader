inline u32 abs32(i32 n)
{
    u32 mask   = n >> 31;
    u32 result = (mask^n) - mask;
    return result;
}

inline u64 abs64(i64 n)
{
    u64 mask   = n >> 63;
    u64 result = (mask^n) - mask;
    return result;
}