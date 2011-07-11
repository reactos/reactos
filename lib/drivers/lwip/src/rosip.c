#include "lwip/sys.h"
#include "lwip/tcpip.h"

#include "rosip.h"

#include <debug.h>

void
LibIPInsertPacket(void *ifarg,
                  const void *const data,
                  const u32_t size)
{
    struct pbuf *p, *p1;
    u32_t i;
    
    ASSERT(ifarg);
    ASSERT(data);
    ASSERT(size > 0);

    p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_POOL);
    if (p)
    {
        for (i = 0, p1 = p; i < size; i += p1->len, p1 = p1->next)
        {
            ASSERT(p1);
            RtlCopyMemory(p1->payload, ((PUCHAR)data) + i, p1->len);
        }

        DbgPrint("LibIPInsertPacket: called 0x%x\n", *((struct netif *)ifarg)->input);

        ((struct netif *)ifarg)->input(p, ifarg);
    }
}

void
LibIPInitialize(void)
{
    /* This completes asynchronously */
    tcpip_init(NULL, NULL);
}

void
LibIPShutdown(void)
{
    /* This is synchronous */
    sys_shutdown();
}