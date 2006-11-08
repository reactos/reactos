#include "usbcommon.h"

#define USB_UHCI_TAG TAG('u','s','b','u')

/* declare basic init functions and structures */
int uhci_hcd_init(void);
void uhci_hcd_cleanup(void);
int STDCALL usb_init(void);
void STDCALL usb_exit(void);
