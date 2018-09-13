#include "rcpptype.h"

/* Long to ASCII conversion routine - used by print, and those programs
 * which want to do low level formatted output without hauling in a great
 * deal of extraneous code.  This will convert a long value to an ascii
 * string in any radix from 2 - 16.
 * RETURNS - the number of characters in the converted buffer.
 */

static WCHAR digits[] = {
    L'0', L'1', L'2', L'3', L'4',
    L'5', L'6', L'7', L'8', L'9',
    L'a', L'b', L'c', L'd', L'e', L'f'
};

#define BITS_IN_LONG  (8*sizeof(long))

int
zltoa(
    long aval,
    register WCHAR *buf,
    int base
    )
{
    // if unsigned long wont work on your host, you will probably have
    // to use signed long and accept this as not working for negative
    // numbers.

    register unsigned long val;
    register WCHAR *p;
    WCHAR tbuf[BITS_IN_LONG];
    int size = 0;

    p = tbuf;
    *p++ = L'\0';

    if (aval < 0 && base == 10) {
        *buf++ = L'-';
        val = -aval;
        size++;
    } else {
        val = aval;
    }

    do {
        *p++ = digits[val % base];
    } while (val /= base);

    while ((*buf++ = *--p) != 0) {
        ++size;
    }

    return(size);
}
