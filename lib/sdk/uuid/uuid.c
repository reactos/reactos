/*
 * GUID definitions
 *
 * Copyright 2000 Alexandre Julliard
 * Copyright 2000 Francois Gouget
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define COM_NO_WINDOWS_H
#include "initguid.h"

/* GUIDs defined in uuids.lib */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "objbase.h"
#include "servprov.h"

#include "oleauto.h"
#include "oleidl.h"
#include "objidl.h"
#include "olectl.h"

#include "mshtmhst.h"
#include "docobj.h"
#include "exdisp.h"

#include "hlink.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shldisp.h"
#include "comcat.h"
#include "urlmon.h"
#include "objsel.h"
#include "richole.h"

/* FIXME: cguids declares GUIDs but does not define their values */

/* other GUIDs */

#include "vfw.h"
#include "mapiguid.h"

#if 0 /* FIXME */
#include "uuids.h"
#endif

/* the GUID for these interfaces are already defined by dxguid.c */
#define __IReferenceClock_INTERFACE_DEFINED__
#define __IKsPropertySet_INTERFACE_DEFINED__
#if 0 /* FIXME */
#include "strmif.h"
#endif
#if 0 /* FIXME */
#include "control.h"
#endif

/* GUIDs not declared in an exported header file */
DEFINE_GUID(IID_IDirectPlaySP,0xc9f6360,0xcc61,0x11cf,0xac,0xec,0x00,0xaa,0x00,0x68,0x86,0xe3);
DEFINE_GUID(IID_ISFHelper,0x1fe68efb,0x1874,0x9812,0x56,0xdc,0x00,0x00,0x00,0x00,0x00,0x00);
DEFINE_GUID(IID_IDPLobbySP,0x5a4e5a20,0x2ced,0x11d0,0xa8,0x89,0x00,0xa0,0xc9,0x05,0x43,0x3c);

DEFINE_GUID(FMTID_SummaryInformation,0xF29F85E0,0x4FF9,0x1068,0xAB,0x91,0x08,0x00,0x2B,0x27,0xB3,0xD9);
DEFINE_GUID(FMTID_DocSummaryInformation,0xD5CDD502,0x2E9C,0x101B,0x93,0x97,0x08,0x00,0x2B,0x2C,0xF9,0xAE);
DEFINE_GUID(FMTID_UserDefinedProperties,0xD5CDD505,0x2E9C,0x101B,0x93,0x97,0x08,0x00,0x2B,0x2C,0xF9,0xAE);

DEFINE_GUID(CLSID_HTMLDocument, 0x25336920, 0x03f9, 0x11cf, 0x8f,0xd0, 0x00,0xaa,0x00,0x68,0x6f,0x13);

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

DEFINE_GUID(CLSID_Picture_Metafile,       0x00000315,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_Picture_Dib,            0x00000316,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_StdGlobalInterfaceTable,0x00000323,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
