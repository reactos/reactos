#include "usbcommon.h"

#define USB_UHCI_TAG TAG('u','s','b','u')

/* declare basic init functions and structures */
int uhci_hcd_init(void);
void uhci_hcd_cleanup(void);
int NTAPI usb_init(void);
void NTAPI usb_exit(void);
