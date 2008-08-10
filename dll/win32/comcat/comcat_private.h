/*
 *	includes for comcat.dll
 *
 * Copyright (C) 2002 John K. Hohm
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "ole2.h"
#include "comcat.h"
#include "wine/unicode.h"

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

/**********************************************************************
 * Dll lifetime tracking declaration for comcat.dll
 */
extern LONG dll_ref;

/**********************************************************************
 * ClassFactory declaration for comcat.dll
 */
typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl *lpVtbl;
    LONG ref;
} ClassFactoryImpl;

extern ClassFactoryImpl COMCAT_ClassFactory;

/**********************************************************************
 * StdComponentCategoriesMgr declaration for comcat.dll
 */
typedef struct
{
    /* IUnknown fields */
    const IUnknownVtbl *unkVtbl;
    const ICatRegisterVtbl *regVtbl;
    const ICatInformationVtbl *infVtbl;
    LONG ref;
} ComCatMgrImpl;

extern ComCatMgrImpl COMCAT_ComCatMgr;
extern const ICatRegisterVtbl COMCAT_ICatRegister_Vtbl;
extern const ICatInformationVtbl COMCAT_ICatInformation_Vtbl;

/**********************************************************************
 * Global string constant declarations
 */
extern const WCHAR clsid_keyname[6];
