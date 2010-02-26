#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "kbdmou.h"

typedef struct _KEYBOARD_DRVR_EXTENSION
{
    PUSB_INTERFACE_DESC pif_desc;
    DEV_HANDLE dev_handle;

    PUSB_DEV_MANAGER dev_mgr;

    UCHAR kbd_data[8];
    UCHAR kbd_old[8];

    UCHAR leds;
    UCHAR leds_old;

    struct _KEYBOARD_DEVICE_EXTENSION *device_ext; // back pointer
} KEYBOARD_DRVR_EXTENSION, *PKEYBOARD_DRVR_EXTENSION;

typedef struct _KEYBOARD_DEVICE_EXTENSION
{
    DEVEXT_HEADER                 hdr; // mandatory header
    PKEYBOARD_DRVR_EXTENSION      DriverExtension;
    KEYBOARD_INDICATOR_PARAMETERS KeyboardIndicators;
    CONNECT_DATA                  ConnectData;
    PDEVICE_OBJECT                Fdo;
} KEYBOARD_DEVICE_EXTENSION, *PKEYBOARD_DEVICE_EXTENSION;

BOOLEAN
kbd_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

BOOLEAN
kbd_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

#endif
