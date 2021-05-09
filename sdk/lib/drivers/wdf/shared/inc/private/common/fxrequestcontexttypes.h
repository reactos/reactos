//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXREQUESTCONTEXTTYPES_H_
#define _FXREQUESTCONTEXTTYPES_H_

//
// Current typedef for a FX_REQUEST_CONTEXT_TYPE is a byte big
//
#define USB_BASE    (0x10)

//
// FX_REQUEST_CONTEXT_TYPE_Xxx is very long. Just use FX_RCT_Xxx instead.
//
enum FxRequestContextTypes {
    FX_REQUEST_CONTEXT_TYPE_NONE                    = 0x00,
    FX_RCT_IO                                       = 0x01,
    FX_RCT_INTERNAL_IOCTL_OTHERS                    = 0x02,
    FX_RCT_USB_PIPE_XFER                            = USB_BASE+0x00,
    FX_RCT_USB_URB_REQUEST                          = USB_BASE+0x01,
    FX_RCT_USB_PIPE_REQUEST                         = USB_BASE+0x02,
    FX_RCT_USB_CONTROL_REQUEST                      = USB_BASE+0x03,
    FX_RCT_USB_STRING_REQUEST                       = USB_BASE+0x04,
};


#endif // _FXREQUESTCONTEXTTYPES_H_
