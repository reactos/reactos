#ifndef _FXPKGIO_H_
#define _FXPKGIO_H_

#include "common/fxpackage.h"

//
// This flag is or-ed with a pointer value that is ptr aligned, only lower 2 bits are available.
//
#define FX_IN_DISPATCH_CALLBACK     0x00000001

enum FxIoIteratorList {
    FxIoQueueIteratorListInvalid = 0,
    FxIoQueueIteratorListPowerOn,
    FxIoQueueIteratorListPowerOff,
};

#define IO_ITERATOR_FLUSH_TAG (PVOID) 'sulf'
#define IO_ITERATOR_POWER_TAG (PVOID) 'ewop'

//
// This class is allocated by the driver frameworks manager
// PER DEVICE, and is not package global, but per device global,
// data.
//
// This is similar to the NT DeviceExtension.
//
class FxPkgIo : public FxPackage {
};

#endif //_FXPKGIO_H_