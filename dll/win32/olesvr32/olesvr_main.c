/*
 *	OLESVR library
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef enum
{
    OLE_OK,
    OLE_WAIT_FOR_RELEASE,
    OLE_BUSY,
    OLE_ERROR_PROTECT_ONLY,
    OLE_ERROR_MEMORY,
    OLE_ERROR_STREAM,
    OLE_ERROR_STATIC,
    OLE_ERROR_BLANK,
    OLE_ERROR_DRAW,
    OLE_ERROR_METAFILE,
    OLE_ERROR_ABORT,
    OLE_ERROR_CLIPBOARD,
    OLE_ERROR_FORMAT,
    OLE_ERROR_OBJECT,
    OLE_ERROR_OPTION,
    OLE_ERROR_PROTOCOL,
    OLE_ERROR_ADDRESS,
    OLE_ERROR_NOT_EQUAL,
    OLE_ERROR_HANDLE,
    OLE_ERROR_GENERIC,
    OLE_ERROR_CLASS,
    OLE_ERROR_SYNTAX,
    OLE_ERROR_DATATYPE,
    OLE_ERROR_PALETTE,
    OLE_ERROR_NOT_LINK,
    OLE_ERROR_NOT_EMPTY,
    OLE_ERROR_SIZE,
    OLE_ERROR_DRIVE,
    OLE_ERROR_NETWORK,
    OLE_ERROR_NAME,
    OLE_ERROR_TEMPLATE,
    OLE_ERROR_NEW,
    OLE_ERROR_EDIT,
    OLE_ERROR_OPEN,
    OLE_ERROR_NOT_OPEN,
    OLE_ERROR_LAUNCH,
    OLE_ERROR_COMM,
    OLE_ERROR_TERMINATE,
    OLE_ERROR_COMMAND,
    OLE_ERROR_SHOW,
    OLE_ERROR_DOVERB,
    OLE_ERROR_ADVISE_NATIVE,
    OLE_ERROR_ADVISE_PICT,
    OLE_ERROR_ADVISE_RENAME,
    OLE_ERROR_POKE_NATIVE,
    OLE_ERROR_REQUEST_NATIVE,
    OLE_ERROR_REQUEST_PICT,
    OLE_ERROR_SERVER_BLOCKED,
    OLE_ERROR_REGISTRATION,
    OLE_ERROR_ALREADY_REGISTERED,
    OLE_ERROR_TASK,
    OLE_ERROR_OUTOFDATE,
    OLE_ERROR_CANT_UPDATE_CLIENT,
    OLE_ERROR_UPDATE,
    OLE_ERROR_SETDATA_FORMAT,
    OLE_ERROR_STATIC_FROM_OTHER_OS,
    OLE_WARN_DELETE_DATA = 1000
} OLESTATUS;

typedef enum {
    OLE_SERVER_MULTI,
    OLE_SERVER_SINGLE
} OLE_SERVER_USE;

typedef LONG LHSERVER;
typedef LONG LHSERVERDOC;
typedef LPCSTR LPCOLESTR16;

typedef struct _OLESERVERDOC *LPOLESERVERDOC;

struct _OLESERVERDOCVTBL;
typedef struct _OLESERVERDOC
{
    const struct _OLESERVERDOCVTBL *lpvtbl;
    /* server provided state info */
} OLESERVERDOC;

typedef struct _OLESERVER *LPOLESERVER;
typedef struct _OLESERVERVTBL
{
    OLESTATUS (CALLBACK *Open)(LPOLESERVER,LHSERVERDOC,LPCOLESTR16,LPOLESERVERDOC *);
    OLESTATUS (CALLBACK *Create)(LPOLESERVER,LHSERVERDOC,LPCOLESTR16,LPCOLESTR16,LPOLESERVERDOC*);
    OLESTATUS (CALLBACK *CreateFromTemplate)(LPOLESERVER,LHSERVERDOC,LPCOLESTR16,LPCOLESTR16,LPCOLESTR16,LPOLESERVERDOC *);
    OLESTATUS (CALLBACK *Edit)(LPOLESERVER,LHSERVERDOC,LPCOLESTR16,LPCOLESTR16,LPOLESERVERDOC *);
    OLESTATUS (CALLBACK *Exit)(LPOLESERVER);
    OLESTATUS (CALLBACK *Release)(LPOLESERVER);
    OLESTATUS (CALLBACK *Execute)(LPOLESERVER);
} OLESERVERVTBL, *LPOLESERVERVTBL;

typedef struct _OLESERVER
{
    const OLESERVERVTBL *lpvtbl;
    /* server specific data */
} OLESERVER;

static LONG OLE_current_handle;

/******************************************************************************
 *		OleBlockServer	[OLESVR32.4]
 */
OLESTATUS WINAPI OleBlockServer(LHSERVER hServer)
{
    FIXME("(%ld): stub\n",hServer);
    return OLE_OK;
}

/******************************************************************************
 *		OleUnblockServer	[OLESVR32.5]
 */
OLESTATUS WINAPI OleUnblockServer(LHSERVER hServer, BOOL *block)
{
    FIXME("(%ld): stub\n",hServer);
    /* no more blocked messages :) */
    *block=FALSE;
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeServerDoc	[OLESVR32.7]
 */
OLESTATUS WINAPI OleRevokeServerDoc(LHSERVERDOC hServerDoc)
{
    FIXME("(%ld): stub\n",hServerDoc);
    return OLE_OK;
}

/******************************************************************************
 * OleRegisterServer [OLESVR32.2]
 */
OLESTATUS WINAPI OleRegisterServer(LPCSTR svrname,LPOLESERVER olesvr,LHSERVER* hRet,HINSTANCE hinst,OLE_SERVER_USE osu) {
	FIXME("(%s,%p,%p,%p,%d): stub!\n",svrname,olesvr,hRet,hinst,osu);
    	*hRet=++OLE_current_handle;
	return OLE_OK;
}

/******************************************************************************
 * OleRegisterServerDoc [OLESVR32.6]
 */
OLESTATUS WINAPI OleRegisterServerDoc( LHSERVER hServer, LPCSTR docname,
                                         LPOLESERVERDOC document,
                                         LHSERVERDOC *hRet)
{
    FIXME("(%ld,%s): stub\n", hServer, docname);
    *hRet=++OLE_current_handle;
    return OLE_OK;
}

/******************************************************************************
 *		OleRenameServerDoc	[OLESVR32.8]
 *
 */
OLESTATUS WINAPI OleRenameServerDoc(LHSERVERDOC hDoc, LPCSTR newName)
{
    FIXME("(%ld,%s): stub.\n",hDoc, newName);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevertServerDoc	[OLESVR32.9]
 *
 */
OLESTATUS WINAPI OleRevertServerDoc(LHSERVERDOC hDoc)
{
    FIXME("(%ld): stub.\n", hDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleSavedServerDoc	[OLESVR32.10]
 *
 */
OLESTATUS WINAPI OleSavedServerDoc(LHSERVERDOC hDoc)
{
    FIXME("(%ld): stub.\n", hDoc);
    return OLE_OK;
}

/******************************************************************************
 *		OleRevokeServer	[OLESVR32.3]
 *
 */
OLESTATUS WINAPI OleRevokeServer(LHSERVER hServer)
{
    FIXME("(%ld): stub.\n", hServer);
    return OLE_OK;
}
