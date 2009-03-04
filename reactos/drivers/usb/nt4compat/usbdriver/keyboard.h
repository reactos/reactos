#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

typedef struct _KEYBOARD_DRVR_EXTENSION
{
    //INTERRUPT_DATA_BLOCK    idb;
    PUSB_INTERFACE_DESC pif_desc;
    UCHAR               if_idx, out_endp_idx, in_endp_idx, int_endp_idx;
    PUSB_ENDPOINT_DESC  pout_endp_desc, pin_endp_desc, pint_endp_desc;

    PUSB_DEV_MANAGER dev_mgr;
    UCHAR kbd_data[8];
    UCHAR kbd_old[8];
} KEYBOARD_DRVR_EXTENSION, *PKEYBOARD_DRVR_EXTENSION;


BOOLEAN
kbd_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

BOOLEAN
kbd_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

#endif
