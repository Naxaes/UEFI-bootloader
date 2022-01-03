inline u32 abs32(u32 n)
{
    u32 mask   = n >> 31UL;
    u32 result = (mask^n) - mask;
    return result;
}

inline u64 abs64(u64 n)
{
    u64 mask   = n >> 63ULL;
    u64 result = (mask^n) - mask;
    return result;
}