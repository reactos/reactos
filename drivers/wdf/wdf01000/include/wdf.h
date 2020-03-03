
#ifndef _WDF_H
#define _WDF_H

#define __deref_out_range(x,y)

#include <ntddk.h>
//
// TODO: Add wdh headers to reactos sdk and replace hardcoded path
//
#include "F:\WinDDK\7600.16385.1\inc\wdf\kmdf\1.9\wdf.h"
#include "common/fxmacros.h"
#include "wdf10.h"
#include "wdf11.h"
#include "wdf17.h"
#include "wdf19.h"
#include "wdf111.h"

#define WDFNOTIMPLEMENTED() (DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, __FUNCTION__" not implemented\r\n"))
typedef USHORT WDFOBJECT_OFFSET, *PWDFOBJECT_OFFSET;

#endif //_WDF_H
