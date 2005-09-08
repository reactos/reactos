/* 
 * buffer_simple.c -- replacement for usb/core/buffer.c
 *
 * (c) Georg Acher, georg@acher.org
 *
 */

#include "../usb_wrapper.h"
#define __KERNEL__
#define CONFIG_PCI
#include "hcd.h"

/*------------------------------------------------------------------------*/ 
int hcd_buffer_create (struct usb_hcd *hcd)
{
	return 0;
}
/*------------------------------------------------------------------------*/ 
void hcd_buffer_destroy (struct usb_hcd *hcd)
{
}
/*------------------------------------------------------------------------*/ 
void *hcd_buffer_alloc (
        struct usb_bus          *bus,
        size_t                  size,
        int                     mem_flags,
        dma_addr_t              *dma
)
{
	return kmalloc(size,0);
}
/*------------------------------------------------------------------------*/ 
void hcd_buffer_free (
        struct usb_bus          *bus,
        size_t                  size,
        void                    *addr,
        dma_addr_t              dma
)
{
	kfree(addr);
}

