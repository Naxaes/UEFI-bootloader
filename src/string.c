inline int strcmp(const char* a, const char* b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (*a < *b) return -1;
        if (*a > *b) return  1;
        ++a;
        ++b;
    }
    return 0;
}


char* usize_to_string(char* buffer, usize n, usize base)
{
    int i = 0;
    while (1)
    {
        usize reminder = n % base;
        buffer[i++] = (reminder < 10) ? ('0' + (char) reminder) : ('A' + (char) reminder - 10);

        if (!(n /= base))
            break;
    }

    buffer[i--] = '\0';

    for (int j = 0; j < i; j++, i--)
    {
        char temp = buffer[j];
        buffer[j] = buffer[i];
        buffer[i] = temp;
    }

    return buffer;
}
