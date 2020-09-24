//
//    Copyright (C) Microsoft.  All rights reserved.
//
//
// Since #include'ing FxDymamics.h requires a bunch of headers before it, we
// do this in one place because multiple spots in the code include this header.
//

extern "C" {
#pragma  warning(disable:4200)     // zero-sized array in struct/union
#include <usbdrivr.h>
#pragma  warning(default:4200)

#include <wdfusb.h>
#include <wdfminiport.h>
#include "FxDynamics.h"
#include "VfFxDynamics.h"
}
