/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004-2005 ReactOS Team
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
/* $Id$
 *
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/aclui/misc.c
 * PURPOSE:         Access Control List Editor
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      07/01/2005  Created
 */
#include <precomp.h>

#define NDEBUG
#include <debug.h>

static PCWSTR ObjectPickerAttributes[] =
{
    L"ObjectSid",
};

static INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        return -1;
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
        (lpStr = LockResource(hRes)))
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* position in the block, same as % 16 */
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }

        /* Found the string */
        return (int)(*lpStr);
    }
    return -1;
}

static INT
AllocAndLoadString(OUT LPWSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(WCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadStringW(hInst, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...)
{
    DWORD Ret = 0;
    LPWSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) > 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (LPWSTR)lpTarget,
                             0,
                             &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}

LPARAM
ListViewGetSelectedItemData(IN HWND hwnd)
{
    int Index;

    Index = ListView_GetNextItem(hwnd,
                                 -1,
                                 LVNI_SELECTED);
    if (Index != -1)
    {
        LVITEM li;

        li.mask = LVIF_PARAM;
        li.iItem = Index;
        li.iSubItem = 0;

        if (ListView_GetItem(hwnd,
                             &li))
        {
            return li.lParam;
        }
    }

    return 0;
}

BOOL
ListViewSelectItem(IN HWND hwnd,
                   IN INT Index)
{
    LVITEM li;

    li.mask = LVIF_STATE;
    li.iItem = Index;
    li.iSubItem = 0;
    li.state = LVIS_SELECTED;
    li.stateMask = LVIS_SELECTED;

    return ListView_SetItem(hwnd,
                            &li);
}

HRESULT
InitializeObjectPicker(IN PCWSTR ServerName,
                       IN PSI_OBJECT_INFO ObjectInfo,
                       OUT IDsObjectPicker **pDsObjectPicker)
{
    HRESULT hRet;

    *pDsObjectPicker = NULL;

    hRet = CoCreateInstance(&CLSID_DsObjectPicker,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            &IID_IDsObjectPicker,
                            (LPVOID*)pDsObjectPicker);
    if (SUCCEEDED(hRet))
    {
        DSOP_INIT_INFO InitInfo;
        UINT i;
        static DSOP_SCOPE_INIT_INFO Scopes[] =
        {
            {
                sizeof(DSOP_SCOPE_INIT_INFO),
                DSOP_SCOPE_TYPE_TARGET_COMPUTER,
                DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS | DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS |
                    DSOP_SCOPE_FLAG_STARTING_SCOPE,
                {
                    {
                        0,
                        0,
                        0
                    },
                    DSOP_DOWNLEVEL_FILTER_USERS | DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS |
                        DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS | DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS
                },
                NULL,
                NULL,
                S_OK
            },
        };

        InitInfo.cbSize = sizeof(InitInfo);
        InitInfo.pwzTargetComputer = ServerName;
        InitInfo.cDsScopeInfos = sizeof(Scopes) / sizeof(Scopes[0]);
        InitInfo.aDsScopeInfos = Scopes;
        InitInfo.flOptions = DSOP_FLAG_MULTISELECT;
        InitInfo.cAttributesToFetch = sizeof(ObjectPickerAttributes) / sizeof(ObjectPickerAttributes[0]);
        InitInfo.apwzAttributeNames = ObjectPickerAttributes;

        for (i = 0; i < InitInfo.cDsScopeInfos; i++)
        {
            if ((ObjectInfo->dwFlags & SI_SERVER_IS_DC) &&
                (InitInfo.aDsScopeInfos[i].flType & DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN))
            {
                /* only set the domain controller string if we know the target
                   computer is a domain controller and the scope type is an
                   up-level domain to which the target computer is joined */
                InitInfo.aDsScopeInfos[i].pwzDcName = InitInfo.pwzTargetComputer;
            }
        }

        hRet = (*pDsObjectPicker)->lpVtbl->Initialize(*pDsObjectPicker,
                                                      &InitInfo);

        if (FAILED(hRet))
        {
            /* delete the object picker in case initialization failed! */
            (*pDsObjectPicker)->lpVtbl->Release(*pDsObjectPicker);
        }
    }

    return hRet;
}

HRESULT
InvokeObjectPickerDialog(IN IDsObjectPicker *pDsObjectPicker,
                         IN HWND hwndParent  OPTIONAL,
                         IN POBJPICK_SELECTED_SID SelectedSidCallback,
                         IN PVOID Context  OPTIONAL)
{
    IDataObject *pdo = NULL;
    HRESULT hRet;

    hRet = pDsObjectPicker->lpVtbl->InvokeDialog(pDsObjectPicker,
                                                 hwndParent,
                                                 &pdo);
    if (hRet == S_OK)
    {
        STGMEDIUM stm;
        FORMATETC fe;

        fe.cfFormat = (WORD)RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);
        fe.ptd = NULL;
        fe.dwAspect = DVASPECT_CONTENT;
        fe.lindex = -1;
        fe.tymed = TYMED_HGLOBAL;

        hRet = pdo->lpVtbl->GetData(pdo,
                                    &fe,
                                    &stm);
        if (SUCCEEDED(hRet))
        {
            PDS_SELECTION_LIST SelectionList = (PDS_SELECTION_LIST)GlobalLock(stm.hGlobal);
            if (SelectionList != NULL)
            {
                LPVARIANT vSid;
                PSID pSid;
                UINT i;
                BOOL contLoop = TRUE;

                for (i = 0; i < SelectionList->cItems && contLoop; i++)
                {
                    vSid = SelectionList->aDsSelection[i].pvarFetchedAttributes;

                    if (vSid != NULL && V_VT(vSid) == (VT_ARRAY | VT_UI1))
                    {
                        hRet = SafeArrayAccessData(V_ARRAY(vSid),
                                                   (void **)&pSid);
                        if (FAILED(hRet))
                        {
                            break;
                        }

                        if (pSid != NULL)
                        {
                            contLoop = SelectedSidCallback(pDsObjectPicker,
                                                           hwndParent,
                                                           pSid,
                                                           Context);
                        }

                        SafeArrayUnaccessData(V_ARRAY(vSid));
                    }
                }

                GlobalUnlock(stm.hGlobal);

                if (SUCCEEDED(hRet))
                {
                    /* return S_OK instead of possible other success codes if
                       everything went well */
                    hRet = S_OK;
                }
            }
            else
            {
                /* unable to translate the selection pointer handle, indicate
                   failure */
                hRet = E_FAIL;
            }

            ReleaseStgMedium(&stm);
        }

        pdo->lpVtbl->Release(pdo);
    }

    return hRet;
}

VOID
FreeObjectPicker(IN IDsObjectPicker *pDsObjectPicker)
{
    pDsObjectPicker->lpVtbl->Release(pDsObjectPicker);
}

