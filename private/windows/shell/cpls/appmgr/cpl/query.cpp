/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    query.cpp

Abstract:

    This module contains the function to bring up the standard published
    application query.

Author:

    Dave Hastings (daveh) creation-date 20-Nov-1997

Revision History:

--*/

#include <windows.h>
#include <ole2.h>
#include <cmnquery.h>

#define INITGUID
#include <initguid.h>
#include <cmnquery.h>
#include "guid.h"

#ifdef __cplusplus
extern "C" {
#endif

VOID
FindApplication(
    HWND Parent
    );

#ifdef __cplusplus
}
#endif

VOID 
FindApplication(
    HWND Parent
    )
/*++

Routine Description:

    This function uses the standard query support in OLE to bring
    up a modal version of the published application query.

Arguments:

    Parent - Supplies the window handle of the window this query should
        be modal to.

Return Value:

    None.

--*/
{
    HRESULT hr;
    ICommonQuery *ICommonQuery;
    OPENQUERYWINDOW QueryWindow;
    IDataObject *IDataObject;

    hr = CoCreateInstance(
        CLSID_CommonQuery,
        NULL,
        CLSCTX_ALL,
        IID_ICommonQuery,
        (VOID **)&ICommonQuery
        );

    // bugbug error

    QueryWindow.cbStruct = sizeof(OPENQUERYWINDOW);
    QueryWindow.dwFlags =  OQWF_OKCANCEL | OQWF_REMOVESCOPES | OQWF_REMOVEFORMS;
    QueryWindow.clsidHandler = CLSID_PublishedApplicationQuery;
    QueryWindow.pHandlerParameters = NULL;

    ICommonQuery->OpenQueryWindow(
        Parent,
        &QueryWindow,
        &IDataObject
        );

    return;
}
