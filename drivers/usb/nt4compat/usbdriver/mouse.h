#ifndef __MOUSE_H__
#define __MOUSE_H__

#include "kbdmou.h"

typedef struct _MOUSE_DRVR_EXTENSION
{
    PUSB_INTERFACE_DESC pif_desc;

    PUSB_DEV_MANAGER dev_mgr;
    signed char mouse_data[8];
    UCHAR btn_old;

    struct _MOUSE_DEVICE_EXTENSION *device_ext; // back pointer
} MOUSE_DRVR_EXTENSION, *PMOUSE_DRVR_EXTENSION;

typedef struct _MOUSE_DEVICE_EXTENSION
{
    DEVEXT_HEADER           hdr; // mandatory header
    PMOUSE_DRVR_EXTENSION   DriverExtension;
    CONNECT_DATA            ConnectData;
    PDEVICE_OBJECT          Fdo;
} MOUSE_DEVICE_EXTENSION, *PMOUSE_DEVICE_EXTENSION;

BOOLEAN
mouse_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

BOOLEAN
mouse_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

#endif
