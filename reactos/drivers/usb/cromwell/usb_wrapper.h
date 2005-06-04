//#include <stdlib.h>
//#include <ntos/types.h>
//#include <ddk/extypes.h>
#include <ddk/ntddk.h>
#define NDEBUG
#include <debug.h>

void wait_ms(int mils);

#ifndef _snprintf
int _snprintf(char * buf, size_t cnt, const char *fmt, ...);
#endif
#ifndef sprintf
int sprintf(char * buf, const char *fmt, ...);
#endif
#ifndef swprintf
int swprintf(wchar_t *buf, const wchar_t *fmt, ...);
#endif

#include "linux/linux_wrapper.h"
#define __KERNEL__
#undef CONFIG_PCI
#define CONFIG_PCI

#include "linux/usb.h"
#include "linux/pci_ids.h"

