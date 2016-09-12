#define WIN32_NO_STATUS
#define _INC_WINDOWS
#include <windef.h>
#include <winsock2.h>

const char *
WSAAPI
inet_ntop (int af,
           const void *src,
           char *dst,
           size_t cnt)
{
    struct in_addr in;
    char *text_addr;

    if (af == AF_INET)
    {
        memcpy(&in.s_addr, src, sizeof(in.s_addr));
        text_addr = inet_ntoa(in);
        if (text_addr && dst)
        {
            strncpy(dst, text_addr, cnt);
            return dst;
        }
    }

    return 0;
}

