

#if !defined(_MSC_VER) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#include <stdlib.h>
#include <intrin.h>

void
byteReverse(unsigned char *buf, unsigned longs)
{
    unsigned int t;

    do
    {
#if 0
        t = (unsigned int)((unsigned)buf[3] << 8 | buf[2]) << 16 |
            ((unsigned)buf[1] << 8 | buf[0]);
#else
        t = _byteswap_ulong(*(unsigned int *)buf);
#endif
        *(unsigned int *)buf = t;
        buf += 4;
    } while (--longs);
}

#endif // !defined(_MSC_VER) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

