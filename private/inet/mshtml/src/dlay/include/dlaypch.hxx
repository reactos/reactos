//+----------------------------------------------------------------------------
// dlaypch.hxx:
//
// Copyright: (c) 1995, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// Contents: this file contains the precompiled header for dlay.
//
// History:
// 07-18-95    TerryLu     Created
//

#ifndef _DLAYPCH_HXX_
#define _DLAYPCH_HXX_

#       define                  _OLEAUT32_
#define INC_OLE2
#       define                  WIN32_LEAN_AND_MEAN
#       include                 "shlwrap.h"
#       include                 <w4warn.h>
#       include                 <windows.h>
#       include                 <windowsx.h>
#       include                 <ole2.h>
#       include                 <commdlg.h>
#       include                 <mfmwrap.h>
#       include                 <unaligned.hpp>

#ifdef WIN16
#include <docobj.h>
#include <olectl.h>
#include <objext.h>
#endif

#       include                 <dispex.h>

#       include                 <limits.h>
#       include                 <stddef.h>
#       include                 <search.h>
#       include                 <string.h>
#       include                 <tchar.h>

#       include                 <shlwapi.h>
#       include                 <shlwapip.h>

// core includes

#       include                 <olectl.h>
#       include                 <w4warn.h>
#       include                 <coredisp.h>
#       include                 <qi_impl.h>
#       include                 <wrapdefs.h>
#       include                 <f3debug.h>
#       include                 <f3util.hxx>
#       include                 <docobj.h>
#       include                 <cdutil.hxx>
#       include                 <cstr.hxx>
#       include                 <formsary.hxx>
#       include                 <dll.hxx>
#       include                 <objext.h>
#       include                 <ndump.hxx>
#       include                 <cdbase.hxx>
#       include                 <undo.hxx>

// dlay includes

#       include                 <w4warn.h>
#       include                 <oledb.h>
#       include                 <oledberr.h>
#       include                 <mshtmlrc.h>
#       include                 <fixedbmk.hxx>

#endif   // _DLAYPCH_HXX_
