#ifndef USBXHCI_H__
#define USBXHCI_H__

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <wdm.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include "..\usbmport.h"
#include "hardware_xhci.h"

extern USBPORT_REGISTRATION_PACKET RegPacket;

#endif /* USBXHCI_H__ */