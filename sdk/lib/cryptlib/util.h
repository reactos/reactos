
#pragma once

#if defined(_MSC_VER) || (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define byteReverse(buf, len)  ((void)0)
#else
void
byteReverse(unsigned char *buf, unsigned longs);
#endif
