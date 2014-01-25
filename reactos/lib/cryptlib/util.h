
#pragma once


#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define byteReverse(buf, long)((void)(buf, long))
#else
void
byteReverse(unsigned char *buf, unsigned longs);
#endif
