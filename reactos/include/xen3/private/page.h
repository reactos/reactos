#ifndef XEN_PAGE_H_INCLUDED
#define XEN_PAGE_H_INCLUDED

#include <ddk/winddk.h>

#define PAGE_MASK       (~(PAGE_SIZE-1))

#define pgd_val(x) ((x).pgd)

#endif /* XEN_PAGE_H_INCLUDED */

/* EOF */
