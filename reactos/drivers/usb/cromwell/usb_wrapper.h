//#include <stdlib.h>
//#include <ntos/types.h>
//#include <ddk/extypes.h>
#include <ddk/ntddk.h>
#include <debug.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

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

#undef interface

#include "linux/linux_wrapper.h"
#define __KERNEL__
#undef CONFIG_PCI
#define CONFIG_PCI

#include "linux/usb.h"
#include "linux/pci_ids.h"

#define IOCTL_INTERNAL_USB_GET_ROOT_USB_DEVICE \
	CTL_CODE(FILE_DEVICE_USB, 4000, METHOD_BUFFERED, FILE_ANY_ACCESS)
