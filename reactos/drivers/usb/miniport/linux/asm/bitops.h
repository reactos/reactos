#ifndef _REACTOS_USB_BITOPS_H
#define _REACTOS_USB_BITOPS_H

#ifdef _M_IX86
#include "bitops-i386.h"
#elif defined(_M_PPC)
#include "bitops-ppc.h"
#else
#error "No bitops.h for this arch"
#endif

#endif/*_REACTOS_USB_BITOPS_H*/
