//#include <stdlib.h>
//#include <ntos/types.h>
//#include <ddk/extypes.h>
#include <ddk/ntddk.h>
#include <debug.h>

void wait_ms(int mils);

#include "linux/linux_wrapper.h"
#define __KERNEL__
#undef CONFIG_PCI
#define CONFIG_PCI

#include "linux/usb.h"
#include "linux/pci_ids.h"