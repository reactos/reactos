/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    link.c

Abstract:

        This file implements the code to save to a link file.

Author:

    Rick Turner (RickTu) Sep-12-1995

--*/

#include "precomp.h"
#pragma hdrstop

#include "shlobj.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "shlobjp.h"
#include "initguid.h"
#include "oleguid.h"
#include "shlguid.h"
#include "shlguidp.h"



BOOL PathIsLink(LPCTSTR szFile)
{
    return lstrcmpi(TEXT(".lnk"), PathFindExtension(szFile)) == 0;
}


BOOL
WereWeStartedFromALnk()
{
    STARTUPINFO si;

    GetStartupInfo( &si );

    // Check to make sure we were started from a link
    if (si.dwFlags & STARTF_TITLEISLINKNAME)
    {
        if (PathIsLink(si.lpTitle))
            return TRUE;
    }

    return FALSE;
}



BOOL
SetLinkValues(
    PCONSOLE_STATE_INFO pStateInfo
    )

/*++

Routine Description:

    This routine writes values to the link file that spawned this console
    window.  The link file name is still in the startinfo structure.

Arguments:

    pStateInfo - pointer to structure containing information

Return Value:

    none

--*/

{

    STARTUPINFO si;
    IShellLink * psl;
    IPersistFile * ppf;
    IShellLinkDataList * psldl;
    NT_CONSOLE_PROPS props;
#if defined(FE_SB)
    NT_FE_CONSOLE_PROPS fe_props;
#endif
    BOOL bRet;

    GetStartupInfo( &si );

    // Check to make sure we were started from a link
    if (!(si.dwFlags & STARTF_TITLEISLINKNAME) )
        return FALSE;

    // Make sure we are dealing w/a link file
    if (!PathIsLink(si.lpTitle))
        return FALSE;

    // Ok, load the link so we can modify it...
    if (FAILED(SHCoCreateInstance( NULL, &CLSID_ShellLink, NULL, &IID_IShellLink, &psl )))
        return FALSE;

    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf)))
    {
        WCHAR wszPath[ MAX_PATH ];

        StrToOleStr(wszPath, si.lpTitle );
        if (FAILED(ppf->lpVtbl->Load(ppf, wszPath, 0)))
        {
            ppf->lpVtbl->Release(ppf);
            psl->lpVtbl->Release(psl);
            return FALSE;
        }
    }

    // Now the link is loaded, generate new console settings section to replace
    // the one in the link.

    ((LPDBLIST)&props)->cbSize      = sizeof(props);
    ((LPDBLIST)&props)->dwSignature = NT_CONSOLE_PROPS_SIG;
    props.wFillAttribute            = pStateInfo->ScreenAttributes;
    props.wPopupFillAttribute       = pStateInfo->PopupAttributes;
    props.dwScreenBufferSize        = pStateInfo->ScreenBufferSize;
    props.dwWindowSize              = pStateInfo->WindowSize;
    props.dwWindowOrigin.X          = (SHORT)pStateInfo->WindowPosX;
    props.dwWindowOrigin.Y          = (SHORT)pStateInfo->WindowPosY;
    props.nFont                     = 0;
    props.nInputBufferSize          = 0;
    props.dwFontSize                = pStateInfo->FontSize;
    props.uFontFamily               = pStateInfo->FontFamily;
    props.uFontWeight               = pStateInfo->FontWeight;
    CopyMemory( props.FaceName, pStateInfo->FaceName, sizeof(props.FaceName) );
    props.uCursorSize               = pStateInfo->CursorSize;
    props.bFullScreen               = pStateInfo->FullScreen;
    props.bQuickEdit                = pStateInfo->QuickEdit;
    props.bInsertMode               = pStateInfo->InsertMode;
    props.bAutoPosition             = pStateInfo->AutoPosition;
    props.uHistoryBufferSize        = pStateInfo->HistoryBufferSize;
    props.uNumberOfHistoryBuffers   = pStateInfo->NumberOfHistoryBuffers;
    props.bHistoryNoDup             = pStateInfo->HistoryNoDup;
    CopyMemory( props.ColorTable, pStateInfo->ColorTable, sizeof(props.ColorTable) );

#if defined(FE_SB)
    ((LPDBLIST)&fe_props)->cbSize      = sizeof(fe_props);
    ((LPDBLIST)&fe_props)->dwSignature = NT_FE_CONSOLE_PROPS_SIG;
    fe_props.uCodePage                 = pStateInfo->CodePage;
#endif

    if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IShellLinkDataList, &psldl)))
    {
        //
        // Store the changes back into the link...
        //
        psldl->lpVtbl->RemoveDataBlock( psldl, NT_CONSOLE_PROPS_SIG );
        psldl->lpVtbl->AddDataBlock( psldl, (LPVOID)&props );

#if defined(FE_SB)
        if (gfFESystem) {
            psldl->lpVtbl->RemoveDataBlock( psldl, NT_FE_CONSOLE_PROPS_SIG );
            psldl->lpVtbl->AddDataBlock( psldl, (LPVOID)&fe_props );
        }
#endif

        psldl->lpVtbl->Release( psldl );
    }



    bRet = SUCCEEDED(ppf->lpVtbl->Save( ppf, NULL, TRUE ));
    ppf->lpVtbl->Release(ppf);
    psl->lpVtbl->Release(psl);

    return bRet;
}
