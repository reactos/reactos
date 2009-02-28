/*
 *	OLECLI library
 *
 *	Copyright 1995	Martin von Loewis
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

/*	At the moment, these are only empty stubs.
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "wine/windef16.h"
#include "winbase.h"
#include "wingdi.h"
#include "wownt32.h"
#include "objbase.h"
#include "olecli.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct _OLEOBJECTVTBL16 {
    void CALLBACK *(*QueryProtocol)(_LPOLEOBJECT,LPCOLESTR16);
    OLESTATUS      (CALLBACK *Release)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *Show)(_LPOLEOBJECT,BOOL16);
    OLESTATUS      (CALLBACK *DoVerb)(_LPOLEOBJECT,UINT16,BOOL16,BOOL16);
    OLESTATUS      (CALLBACK *GetData)(_LPOLEOBJECT,OLECLIPFORMAT,HANDLE16 *);
    OLESTATUS      (CALLBACK *SetData)(_LPOLEOBJECT,OLECLIPFORMAT,HANDLE16);
    OLESTATUS      (CALLBACK *SetTargetDevice)(_LPOLEOBJECT,HGLOBAL16);
    OLESTATUS      (CALLBACK *SetBounds)(_LPOLEOBJECT,LPRECT16);
    OLESTATUS      (CALLBACK *EnumFormats)(_LPOLEOBJECT,OLECLIPFORMAT);
    OLESTATUS      (CALLBACK *SetColorScheme)(_LPOLEOBJECT,struct tagLOGPALETTE*);
    OLESTATUS      (CALLBACK *Delete)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *SetHostNames)(_LPOLEOBJECT,LPCOLESTR16,LPCOLESTR16);
    OLESTATUS      (CALLBACK *SaveToStream)(_LPOLEOBJECT,struct _OLESTREAM*);
    OLESTATUS      (CALLBACK *Clone)(_LPOLEOBJECT,LPOLECLIENT,LHCLIENTDOC,LPCOLESTR16,_LPOLEOBJECT *);
    OLESTATUS      (CALLBACK *CopyFromLink)(_LPOLEOBJECT,LPOLECLIENT,LHCLIENTDOC,LPCOLESTR16,_LPOLEOBJECT *);
    OLESTATUS      (CALLBACK *Equal)(_LPOLEOBJECT,_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *CopyToClipBoard)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *Draw)(_LPOLEOBJECT,HDC16,LPRECT16,LPRECT16,HDC16);
    OLESTATUS      (CALLBACK *Activate)(_LPOLEOBJECT,UINT16,BOOL16,BOOL16,HWND16,LPRECT16);
    OLESTATUS      (CALLBACK *Execute)(_LPOLEOBJECT,HGLOBAL16,UINT16);
    OLESTATUS      (CALLBACK *Close)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *Update)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *Reconnect)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *ObjectConvert)(_LPOLEOBJECT,LPCOLESTR16,LPOLECLIENT,LHCLIENTDOC,LPCOLESTR16,_LPOLEOBJECT*);
    OLESTATUS      (CALLBACK *GetLinkUpdateOptions)(_LPOLEOBJECT,LPOLEOPT_UPDATE);
    OLESTATUS      (CALLBACK *SetLinkUpdateOptions)(_LPOLEOBJECT,OLEOPT_UPDATE);
    OLESTATUS      (CALLBACK *Rename)(_LPOLEOBJECT,LPCOLESTR16);
    OLESTATUS      (CALLBACK *QueryName)(_LPOLEOBJECT,LPSTR,LPUINT16);
    OLESTATUS      (CALLBACK *QueryType)(_LPOLEOBJECT,LPLONG);
    OLESTATUS      (CALLBACK *QueryBounds)(_LPOLEOBJECT,LPRECT16);
    OLESTATUS      (CALLBACK *QuerySize)(_LPOLEOBJECT,LPDWORD);
    OLESTATUS      (CALLBACK *QueryOpen)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *QueryOutOfDate)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *QueryReleaseStatus)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *QueryReleaseError)(_LPOLEOBJECT);
    OLE_RELEASE_METHOD (CALLBACK *QueryReleaseMethod)(_LPOLEOBJECT);
    OLESTATUS      (CALLBACK *RequestData)(_LPOLEOBJECT,OLECLIPFORMAT);
    OLESTATUS      (CALLBACK *ObjectLong)(_LPOLEOBJECT,UINT16,LPLONG);
} OLEOBJECTVTBL;
typedef OLEOBJECTVTBL *LPOLEOBJECTVTBL;

typedef struct _OLEOBJECT
{
    const OLEOBJECTVTBL *lpvtbl;
} OLEOBJECT16;

static LONG OLE_current_handle;

/******************************************************************************
 *		OleSavedClientDoc	[OLECLI.45]
 */
OLESTATUS WINAPI OleSavedClientDoc16(LHCLIENTDOC hDoc)
{
    FIXME("(%d: stub\n", hDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRegisterClientDoc	[OLECLI.41]
 */
OLESTATUS WINAPI OleRegisterClientDoc16(LPCSTR classname, LPCSTR docname,
                                        LONG reserved, LHCLIENTDOC *hRet )
{
    FIXME("(%s,%s,...): stub\n",classname,docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRenameClientDoc	[OLECLI.43]
 */
OLESTATUS WINAPI OleRenameClientDoc16(LHCLIENTDOC hDoc, LPCSTR newName)
{
    FIXME("(%d,%s,...): stub\n",hDoc, newName);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeClientDoc	[OLECLI.42]
 */
OLESTATUS WINAPI OleRevokeClientDoc16(LHCLIENTDOC hServerDoc)
{
    FIXME("(%d): stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevertClientDoc	[OLECLI.44]
 */
OLESTATUS WINAPI OleRevertClientDoc16(LHCLIENTDOC hServerDoc)
{
    FIXME("(%d): stub\n", hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleEnumObjects	[OLECLI.47]
 */
OLESTATUS WINAPI OleEnumObjects16(LHCLIENTDOC hServerDoc, SEGPTR data)
{
    FIXME("(%d, %04x:%04x): stub\n", hServerDoc, HIWORD(data),
	LOWORD(data));
    return OLE_OK;
}

/******************************************************************************
 *	     OleCreateLinkFromClip	[OLECLI.11]
 */
OLESTATUS WINAPI OleCreateLinkFromClip16( LPCSTR name, SEGPTR olecli, LHCLIENTDOC hclientdoc,
                                          LPCSTR xname, SEGPTR lpoleob, UINT16 render,
                                          UINT16 clipformat )
{
	FIXME("(%s, %04x:%04x, %d, %s, %04x:%04x, %d, %d): stub!\n",
              name, HIWORD(olecli), LOWORD(olecli), hclientdoc, xname, HIWORD(lpoleob),
              LOWORD(lpoleob), render, clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryLinkFromClip	[OLECLI.9]
 */
OLESTATUS WINAPI OleQueryLinkFromClip16(LPCSTR name, UINT16 render, UINT16 clipformat)
{
	FIXME("(%s, %d, %d): stub!\n", name, render, clipformat);
	return OLE_OK;
}

/******************************************************************************
 *           OleQueryCreateFromClip	[OLECLI.10]
 */
OLESTATUS WINAPI OleQueryCreateFromClip16(LPCSTR name, UINT16 render, UINT16 clipformat)
{
	FIXME("(%s, %d, %d): stub!\n", name, render, clipformat);
	return OLE_OK;
}

/******************************************************************************
 *		OleIsDcMeta	[OLECLI.60]
 */
BOOL16 WINAPI OleIsDcMeta16(HDC16 hdc)
{
    return GetObjectType( HDC_32(hdc) ) == OBJ_METADC;
}

/******************************************************************************
 *		OleQueryType	[OLECLI.14]
 */
OLESTATUS WINAPI OleQueryType16(_LPOLEOBJECT oleob,  SEGPTR xlong) {
	FIXME("(%p, %p): stub!\n", oleob, MapSL(xlong));
	return OLE_OK;
}

/******************************************************************************
 *		OleCreateFromClip	[OLECLI.12]
 */
OLESTATUS WINAPI OleCreateFromClip16( LPCSTR name, SEGPTR olecli, LHCLIENTDOC hclientdoc,
                                      LPCSTR xname, SEGPTR lpoleob,
                                      UINT16 render, UINT16 clipformat )
{
	FIXME("(%s, %04x:%04x, %d, %s, %04x:%04x, %d, %d): stub!\n",
              name, HIWORD(olecli), LOWORD(olecli), hclientdoc, xname, HIWORD(lpoleob),
              LOWORD(lpoleob), render, clipformat);
	return OLE_OK;
}
