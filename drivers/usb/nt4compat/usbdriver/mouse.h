#ifndef __MOUSE_H__
#define __MOUSE_H__

typedef struct _MOUSE_DRVR_EXTENSION
{
    //INTERRUPT_DATA_BLOCK    idb;
    PUSB_INTERFACE_DESC pif_desc;
    UCHAR               if_idx, out_endp_idx, in_endp_idx, int_endp_idx;
    PUSB_ENDPOINT_DESC  pout_endp_desc, pin_endp_desc, pint_endp_desc;

    PUSB_DEV_MANAGER dev_mgr;
    signed char mouse_data[8];
    UCHAR btn_old;
} MOUSE_DRVR_EXTENSION, *PMOUSE_DRVR_EXTENSION;


BOOLEAN
mouse_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

BOOLEAN
mouse_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

#endif
