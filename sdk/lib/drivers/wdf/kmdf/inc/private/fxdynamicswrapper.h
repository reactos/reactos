//
//    Copyright (C) Microsoft.  All rights reserved.
//
//
// Since #include'ing FxDymamics.h requires a bunch of headers before it, we
// do this in one place because multiple spots in the code include this header.
//

extern "C" {
#include <usbdrivr.h>

#include <wdfusb.h>
#include <wdfminiport.h>
#include "fxdynamics.h"
#include "vffxdynamics.h"
}
