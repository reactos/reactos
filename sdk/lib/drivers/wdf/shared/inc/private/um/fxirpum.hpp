//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXIRPUM_HPP_
#define _FXIRPUM_HPP_

typedef struct {
    ULONG   Type;
    MdIrp   Irp;
    PIO_CSQ Csq;
} MdIoCsqIrpContext, *PMdIoCsqIrpContext;

#include "FxIrp.hpp"


#endif // _FXIRPUM_HPP_
