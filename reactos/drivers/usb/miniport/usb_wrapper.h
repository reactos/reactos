//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <ntos/types.h>
//#include <extypes.h>

/*
  TODO: Do a major cleanup of this file!
        Lots of definitions should go to corresponding files
*/

//#include <ntifs.h>
#include "ntddk.h"
#include <kbdmou.h>
#include <debug.h>

// a couple of defines
#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE 1
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING 2

// Define bit scan intrinsics, so we can use them without the need to include wnet ddk
#if defined(_MSC_VER)
#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse
#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndReset _interlockedbittestandreset

BOOLEAN _BitScanForward (OUT ULONG *Index, IN ULONG Mask);
BOOLEAN _BitScanReverse (OUT ULONG *Index, IN ULONG Mask);
BOOLEAN _interlockedbittestandset (IN LONG *Base, IN LONG Offset);
BOOLEAN _interlockedbittestandreset (IN LONG *Base, IN LONG Offset);

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_interlockedbittestandset)
#pragma intrinsic(_interlockedbittestandreset)

#define inline __inline
#endif


#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL_LESS_OR_EQUAL(x) ASSERT(KeGetCurrentIrql()<=(x))
#define ASSERT_IRQL_EQUAL(x) ASSERT(KeGetCurrentIrql()==(x))
#define ASSERT_IRQL_LESS(x) ASSERT(KeGetCurrentIrql()<(x))


void wait_ms(int mils);
void my_udelay(int us);

// For now this is needed only for correct operation of embedded keyboard and mouse drvs
typedef struct _USBPORT_INTERFACE
{
	PCONNECT_DATA KbdConnectData;
	PCONNECT_DATA MouseConnectData;
} USBPORT_INTERFACE, *PUSBPORT_INTERFACE;

// Register in usbcore.sys
void STDCALL RegisterPortDriver(PDRIVER_OBJECT, PUSBPORT_INTERFACE);

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
