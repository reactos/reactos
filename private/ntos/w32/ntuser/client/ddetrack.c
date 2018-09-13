/****************************** Module Header ******************************\
* Module Name: ddetrack.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* client sied DDE tracking routines
*
* 10-22-91 sanfords created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


DWORD _ClientCopyDDEIn1(
    HANDLE hClient, // client handle to dde data or ddepack data
    PINTDDEINFO pi) // info for transfer
{
    PBYTE pData;
    DWORD flags;

    //
    // zero out everything but the flags
    //
    flags = pi->flags;
    RtlZeroMemory(pi, sizeof(INTDDEINFO));
    pi->flags = flags;
    USERGLOBALLOCK(hClient, pData);

    if (pData == NULL) {                            // bad hClient
        RIPMSG0(RIP_WARNING, "_ClientCopyDDEIn1:GlobalLock failed.");
        return (FAIL_POST);
    }

    if (flags & XS_PACKED) {

        if (UserGlobalSize(hClient) < sizeof(DDEPACK)) {
            /*
             * must be a low memory condition. fail.
             */
            return(FAIL_POST);
        }

        pi->DdePack = *(PDDEPACK)pData;
        USERGLOBALUNLOCK(hClient);
        UserGlobalFree(hClient);    // packed data handles are not WOW matched.
        hClient = NULL;

        if (!(flags & (XS_LOHANDLE | XS_HIHANDLE))) {
            if (flags & XS_EXECUTE && flags & XS_FREESRC) {
                /*
                 * free execute ACK data
                 */
                WOWGLOBALFREE((HANDLE)pi->DdePack.uiHi);
            }
            return (DO_POST); // no direct data
        }

        if (flags & XS_LOHANDLE) {
            pi->hDirect = (HANDLE)pi->DdePack.uiLo;
        } else {
            pi->hDirect = (HANDLE)pi->DdePack.uiHi;
        }

        if (pi->hDirect == 0) {
            return (DO_POST); // must be warm link
        }

        USERGLOBALLOCK(pi->hDirect, pi->pDirect);
        if (pi->pDirect == NULL) {
            RIPMSG1(RIP_ERROR, "_ClientCopyDDEIn1:GlobalLock failed for hDirect %lx.",pi->hDirect);
            return FAILNOFREE_POST;
        }
        pData = pi->pDirect;
        pi->cbDirect = (UINT)UserGlobalSize(pi->hDirect);

    } else {    // not packed - must be execute data or we wouldn't be called

        UserAssert(flags & XS_EXECUTE);

        pi->cbDirect = (UINT)UserGlobalSize(hClient);
        pi->hDirect = hClient;
        pi->pDirect = pData;
        hClient = NULL;
    }

    if (flags & XS_DATA) {
        PDDE_DATA pDdeData = (PDDE_DATA)pData;

        /*
         * Assert that the hClient has been freed. If not this code will return
         * the wrong thing on failure
         */
        UserAssert(flags & XS_PACKED);

        //
        // check here for indirect data
        //

        switch (pDdeData->wFmt) {
        case CF_BITMAP:
        case CF_DSPBITMAP:
            //
            // Imediately following the dde data header is a bitmap handle.
            //
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->hIndirect = (HANDLE)pDdeData->Data;
            if (pi->hIndirect == 0) {
                RIPMSG0(RIP_WARNING, "_ClientCopyDDEIn1:GdiConvertBitmap failed");
                return(FAILNOFREE_POST);
            }
            // pi->cbIndirect = 0; // zero init.
            // pi->pIndirect = NULL; // zero init.
            pi->flags |= XS_BITMAP;
            break;

        case CF_DIB:
            //
            // Imediately following the dde data header is a global data handle
            // to the DIB bits.
            //
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->flags |= XS_DIB;
            pi->hIndirect = (HANDLE)pDdeData->Data;
            USERGLOBALLOCK(pi->hIndirect, pi->pIndirect);
            if (pi->pIndirect == NULL) {
                RIPMSG0(RIP_WARNING, "_ClientCopyDDEIn1:CF_DIB GlobalLock failed.");
                return (FAILNOFREE_POST);
            }
            pi->cbIndirect = (UINT)UserGlobalSize(pi->hIndirect);
            break;

        case CF_PALETTE:
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->hIndirect = (HANDLE) pDdeData->Data;
            if (pi->hIndirect == 0) {
                RIPMSG0(RIP_WARNING, "_ClientCopyDDEIn1:GdiConvertPalette failed.");
                return(FAILNOFREE_POST);
            }
            // pi->cbIndirect = 0; // zero init.
            // pi->pIndirect = NULL; // zero init.
            pi->flags |= XS_PALETTE;
            break;

        case CF_DSPMETAFILEPICT:
        case CF_METAFILEPICT:
            //
            // This format holds a global data handle which contains
            // a METAFILEPICT structure that in turn contains
            // a GDI metafile.
            //
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->hIndirect = GdiConvertMetaFilePict((HANDLE)pDdeData->Data);
            if (pi->hIndirect == 0) {
                RIPMSG0(RIP_WARNING, "_ClientCopyDDEIn1:GdiConvertMetaFilePict failed");
                return(FAILNOFREE_POST);
            }
            // pi->cbIndirect = 0; // zero init.
            // pi->pIndirect = NULL; // zero init.
            pi->flags |= XS_METAFILEPICT;
            break;

        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
            UserAssert(pi->cbDirect >= sizeof(DDE_DATA));
            pi->hIndirect = GdiConvertEnhMetaFile((HENHMETAFILE)pDdeData->Data);
            if (pi->hIndirect == 0) {
                RIPMSG0(RIP_WARNING, "_ClientCopyDDEIn1:GdiConvertEnhMetaFile failed");
                return(FAILNOFREE_POST);
            }
            // pi->cbIndirect = 0; // zero init.
            // pi->pIndirect = NULL; // zero init.
            pi->flags |= XS_ENHMETAFILE;
            break;
        }
    }

    return (DO_POST);
}


/*
 * unlocks and frees DDE data pointers as appropriate
 */
VOID _ClientCopyDDEIn2(
    PINTDDEINFO pi)
{
    if (pi->cbDirect) {
        USERGLOBALUNLOCK(pi->hDirect);
        if (pi->flags & XS_FREESRC) {
            WOWGLOBALFREE(pi->hDirect);
        }
    }

    if (pi->cbIndirect) {
        USERGLOBALUNLOCK(pi->hIndirect);
        if (pi->flags & XS_FREESRC) {
            WOWGLOBALFREE(pi->hIndirect);
        }
    }
}



/*
 * returns fHandleValueChanged.
 */
BOOL FixupDdeExecuteIfNecessary(
HGLOBAL *phCommands,
BOOL fNeedUnicode)
{
    UINT cbLen;
    UINT cbSrc = (UINT)GlobalSize(*phCommands);
    LPVOID pstr;
    HGLOBAL hTemp;
    BOOL fHandleValueChanged = FALSE;

    USERGLOBALLOCK(*phCommands, pstr);

    if (cbSrc && pstr != NULL) {
        BOOL fIsUnicodeText;
#ifdef ISTEXTUNICODE_WORKS
        int flags;

        flags = (IS_TEXT_UNICODE_UNICODE_MASK |
                IS_TEXT_UNICODE_REVERSE_MASK |
                (IS_TEXT_UNICODE_NOT_UNICODE_MASK &
                (~IS_TEXT_UNICODE_ILLEGAL_CHARS)) |
                IS_TEXT_UNICODE_NOT_ASCII_MASK);
        fIsUnicodeText = RtlIsTextUnicode(pstr, cbSrc - 2, &flags);
#else
        fIsUnicodeText = ((cbSrc >= sizeof(WCHAR)) && (((LPSTR)pstr)[1] == '\0'));
#endif
        if (!fIsUnicodeText && fNeedUnicode) {
            LPWSTR pwsz;
            /*
             * Contents needs to be UNICODE.
             */
            cbLen = strlen(pstr) + 1;
            cbSrc = min(cbSrc, cbLen);
            pwsz = UserLocalAlloc(HEAP_ZERO_MEMORY, cbSrc * sizeof(WCHAR));
            if (pwsz != NULL) {
                if (NT_SUCCESS(RtlMultiByteToUnicodeN(
                        pwsz,
                        cbSrc * sizeof(WCHAR),
                        NULL,
                        (PCHAR)pstr,
                        cbSrc))) {
                    USERGLOBALUNLOCK(*phCommands);
                    if ((hTemp = GlobalReAlloc(
                            *phCommands,
                            cbSrc * sizeof(WCHAR),
                            GMEM_MOVEABLE)) != NULL) {
                        fHandleValueChanged = (hTemp != *phCommands);
                        *phCommands = hTemp;
                        USERGLOBALLOCK(*phCommands, pstr);
                        pwsz[cbSrc - 1] = L'\0';
                        wcscpy(pstr, pwsz);
                    }
                }
                UserLocalFree(pwsz);
            }
        } else if (fIsUnicodeText && !fNeedUnicode) {
            LPSTR psz;
            /*
             * Contents needs to be ANSI.
             */
            cbLen = (wcslen(pstr) + 1) * sizeof(WCHAR);
            cbSrc = min(cbSrc, cbLen);
            psz = UserLocalAlloc(HEAP_ZERO_MEMORY, cbSrc);
            if (psz != NULL) {
                if (NT_SUCCESS(RtlUnicodeToMultiByteN(
                        psz,
                        cbSrc,
                        NULL,
                        (PWSTR)pstr,
                        cbSrc))) {
                    USERGLOBALUNLOCK(*phCommands);
                    if ((hTemp = GlobalReAlloc(
                            *phCommands,
                            cbSrc / sizeof(WCHAR),
                            GMEM_MOVEABLE)) != NULL) {
                        fHandleValueChanged = (hTemp != *phCommands);
                        *phCommands = hTemp;
                        USERGLOBALLOCK(*phCommands, pstr);
                        UserAssert(pstr);
                        psz[cbSrc - 1] = '\0';
                        strcpy(pstr, psz);
                    }
                }
                UserLocalFree(psz);
            }
        }
        USERGLOBALUNLOCK(*phCommands);
    }
    return(fHandleValueChanged);
}



/*
 * Allocates and locks global handles as appropriate in preperation
 * for thunk copying.
 */
HANDLE _ClientCopyDDEOut1(
    PINTDDEINFO pi)
{
    HANDLE hDdePack = NULL;
    PDDEPACK pDdePack = NULL;

    if (pi->flags & XS_PACKED) {
        /*
         * make a wrapper for the data
         */
        hDdePack = UserGlobalAlloc(GMEM_DDESHARE | GMEM_FIXED,
                sizeof(DDEPACK));
        pDdePack = (PDDEPACK)hDdePack;
        if (pDdePack == NULL) {
            RIPMSG0(RIP_WARNING, "_ClientCopyDDEOut1:Couldn't allocate DDEPACK");
            return (NULL);
        }
        *pDdePack = pi->DdePack;
    }

    if (pi->cbDirect) {
        pi->hDirect = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, pi->cbDirect);
        if (pi->hDirect == NULL) {
            RIPMSG0(RIP_WARNING, "_ClientCopyDDEOut1:Couldn't allocate hDirect");
            if (hDdePack) {
                UserGlobalFree(hDdePack);
            }
            return (NULL);
        }

        USERGLOBALLOCK(pi->hDirect, pi->pDirect);
        UserAssert(pi->pDirect);

        // fixup packed data reference to direct data

        if (pDdePack != NULL) {
            if (pi->flags & XS_LOHANDLE) {
                pDdePack->uiLo = HandleToUlong(pi->hDirect);
                UserAssert((ULONG_PTR)pDdePack->uiLo == (ULONG_PTR)pi->hDirect);
            } else if (pi->flags & XS_HIHANDLE) {
                pDdePack->uiHi = HandleToUlong(pi->hDirect);
                UserAssert((ULONG_PTR)pDdePack->uiHi == (ULONG_PTR)pi->hDirect);
            }
        }

        if (pi->cbIndirect) {
            pi->hIndirect = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
                    pi->cbIndirect);
            if (pi->hIndirect == NULL) {
                RIPMSG0(RIP_WARNING, "_ClientCopyDDEOut1:Couldn't allocate hIndirect");
                USERGLOBALUNLOCK(pi->hDirect);
                UserGlobalFree(pi->hDirect);
                if (hDdePack) {
                    UserGlobalFree(hDdePack);
                }
                return (NULL);
            }
            USERGLOBALLOCK(pi->hIndirect, pi->pIndirect);
            UserAssert(pi->pIndirect);
        }
    }

    if (hDdePack) {
        return (hDdePack);
    } else {
        return (pi->hDirect);
    }
}



/*
 * Fixes up internal poniters after thunk copy and unlocks handles.
 */
BOOL _ClientCopyDDEOut2(
    PINTDDEINFO pi)
{
    BOOL fSuccess = TRUE;
    /*
     * done with copies - now fixup indirect references
     */
    if (pi->hIndirect) {
        PDDE_DATA pDdeData = (PDDE_DATA)pi->pDirect;

        switch (pDdeData->wFmt) {
        case CF_BITMAP:
        case CF_DSPBITMAP:
        case CF_PALETTE:
            pDdeData->Data = (ULONG_PTR)pi->hIndirect;
            fSuccess = (BOOL)pDdeData->Data;
            break;

        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
            pDdeData->Data = (ULONG_PTR)GdiCreateLocalMetaFilePict(pi->hIndirect);
            fSuccess = (BOOL)pDdeData->Data;
            break;

        case CF_DIB:
            pDdeData->Data = (ULONG_PTR)pi->hIndirect;
            fSuccess = (BOOL)pDdeData->Data;
            USERGLOBALUNLOCK(pi->hIndirect);
            break;

        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
            pDdeData->Data = (ULONG_PTR)GdiCreateLocalEnhMetaFile(pi->hIndirect);
            fSuccess = (BOOL)pDdeData->Data;
            break;

        default:
            RIPMSG0(RIP_WARNING, "_ClientCopyDDEOut2:Unknown format w/indirect data.");
            fSuccess = FALSE;
            USERGLOBALUNLOCK(pi->hIndirect);
        }
    }

    UserAssert(pi->hDirect); // if its null, we didn't need to call this function.
    USERGLOBALUNLOCK(pi->hDirect);
    if (pi->flags & XS_EXECUTE) {
        /*
         * Its possible that in RAW DDE cases where the app allocated the
         * execute data as non-moveable, we have a different hDirect
         * than we started with.  This needs to be noted and passed
         * back to the server. (Very RARE case)
         */
        FixupDdeExecuteIfNecessary(&pi->hDirect,
                pi->flags & XS_UNICODE);
    }
    return fSuccess;
}



/*
 * This routine is called by the tracking layer when it frees DDE objects
 * on behalf of a client.   This cleans up the LOCAL objects associated
 * with the DDE objects.  It should NOT remove truely global objects such
 * as bitmaps or palettes except in the XS_DUMPMSG case which is for
 * faked Posts.
 */

#if DBG
    /*
     * Help track down a bug where I suspect the xxxFreeListFree is
     * freeing a handle already freed by some other means which has
     * since been reallocated and is trashing the client heap. (SAS)
     */
    HANDLE DDEHandleLastFreed = 0;
#endif

BOOL _ClientFreeDDEHandle(
HANDLE hDDE,
DWORD flags)
{
    PDDEPACK pDdePack;
    HANDLE hNew;

    if (flags & XS_PACKED) {
        pDdePack = (PDDEPACK)hDDE;
        if (pDdePack == NULL) {
            return (FALSE);
        }
        if (flags & XS_LOHANDLE) {
            hNew = (HANDLE)pDdePack->uiLo;
        } else {
            hNew = (HANDLE)pDdePack->uiHi;

        }
        WOWGLOBALFREE(hDDE);
        hDDE = hNew;

    }

   /*
    * Do a range check and call GlobalFlags to validate, just to prevent heap checking
    * from complaining during the GlobalSize call.
    * Is this leaking atoms??
    */
    if ((hDDE <= (HANDLE)0xFFFF)
        || (GlobalFlags(hDDE) == GMEM_INVALID_HANDLE)
        || !GlobalSize(hDDE)) {
            /*
             * There may be cases where apps improperly freed stuff
             * when they shouldn't have so make sure this handle
             * is valid by the time it gets here.
             *
             * See SvSpontAdvise; it posts a message with an atom in uiHi. Then from _PostMessage
             *  in the kernel side, we might end up here. So it's not only for apps...
             */
            return(FALSE);
    }

    if (flags & XS_DUMPMSG) {
        if (flags & XS_PACKED) {
            if (!IS_PTR(hNew)) {
                GlobalDeleteAtom(LOWORD((ULONG_PTR)hNew));
                if (!(flags & XS_DATA)) {
                    return(TRUE);     // ACK
                }
            }
        } else {
            if (!(flags & XS_EXECUTE)) {
                GlobalDeleteAtom(LOWORD((ULONG_PTR)hDDE));   // REQUEST, UNADVISE
                return(TRUE);
            }
        }
    }
    if (flags & XS_DATA) {
        // POKE, DATA
#if DBG
        DDEHandleLastFreed = hDDE;
#endif
        FreeDDEData(hDDE,
                (flags & XS_DUMPMSG) ? FALSE : TRUE,    // fIgnorefRelease
                (flags & XS_DUMPMSG) ? TRUE : FALSE);    // fDestroyTruelyGlobalObjects
    } else {
        // ADVISE, EXECUTE
#if DBG
        DDEHandleLastFreed = hDDE;
#endif
        WOWGLOBALFREE(hDDE);   // covers ADVISE case (fmt but no data)
    }
    return (TRUE);
}


DWORD _ClientGetDDEFlags(
HANDLE hDDE,
DWORD flags)
{
    PDDEPACK pDdePack;
    PWORD pw;
    HANDLE hData;
    DWORD retval = 0;

    pDdePack = (PDDEPACK)hDDE;
    if (pDdePack == NULL) {
        return (0);
    }

    if (flags & XS_DATA) {
        if (pDdePack->uiLo) {
            hData = (HANDLE)pDdePack->uiLo;
            USERGLOBALLOCK(hData, pw);
            if (pw != NULL) {
                retval = (DWORD)*pw; // first word is hData is wStatus
                USERGLOBALUNLOCK(hData);
            }
        }
    } else {
        retval = (DWORD)pDdePack->uiLo;
    }

    return (retval);
}


LPARAM APIENTRY PackDDElParam(
UINT msg,
UINT_PTR uiLo,
UINT_PTR uiHi)
{
    PDDEPACK pDdePack;
    HANDLE h;

    switch (msg) {
    case WM_DDE_EXECUTE:
        return((LPARAM)uiHi);

    case WM_DDE_ACK:
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        h = UserGlobalAlloc(GMEM_DDESHARE | GMEM_FIXED, sizeof(DDEPACK));
        pDdePack = (PDDEPACK)h;
        if (pDdePack == NULL) {
            return(0);
        }
        pDdePack->uiLo = uiLo;
        pDdePack->uiHi = uiHi;
        return((LPARAM)h);

    default:
        return(MAKELONG((WORD)uiLo, (WORD)uiHi));
    }
}



BOOL APIENTRY UnpackDDElParam(
UINT msg,
LPARAM lParam,
PUINT_PTR puiLo,
PUINT_PTR puiHi)
{
    PDDEPACK pDdePack;

    switch (msg) {
    case WM_DDE_EXECUTE:
        if (puiLo != NULL) {
            *puiLo = 0L;
        }
        if (puiHi != NULL) {
            *puiHi = (UINT_PTR)lParam;
        }
        return(TRUE);

    case WM_DDE_ACK:
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        pDdePack = (PDDEPACK)lParam;
        if (pDdePack == NULL || !GlobalHandle(pDdePack)) {
            return(FALSE);
        }
        if (puiLo != NULL) {
            *puiLo = pDdePack->uiLo;
        }
        if (puiHi != NULL) {
            *puiHi = pDdePack->uiHi;
        }
        return(TRUE);

    default:
        if (puiLo != NULL) {
            *puiLo = (UINT)LOWORD(lParam);
        }
        if (puiHi != NULL) {
            *puiHi = (UINT)HIWORD(lParam);
        }
        return(TRUE);
    }
}



BOOL APIENTRY FreeDDElParam(
UINT msg,
LPARAM lParam)
{
    switch (msg) {
    case WM_DDE_ACK:
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        /*
         * Do a range check and call GlobalFlags to validate,
         * just to prevent heap checking from complaining
         */
        if ((lParam > (LPARAM)0xFFFF) && GlobalFlags((HANDLE)lParam) != GMEM_INVALID_HANDLE) {
            if (GlobalHandle((HANDLE)lParam))
                return(UserGlobalFree((HANDLE)lParam) == NULL);
        }

    default:
        return(TRUE);
    }
}


LPARAM APIENTRY ReuseDDElParam(
LPARAM lParam,
UINT msgIn,
UINT msgOut,
UINT_PTR uiLo,
UINT_PTR uiHi)
{
    PDDEPACK pDdePack;

    switch (msgIn) {
    case WM_DDE_ACK:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
    case WM_DDE_ADVISE:
        //
        // Incoming message was packed...
        //
        switch (msgOut) {
        case WM_DDE_EXECUTE:
            FreeDDElParam(msgIn, lParam);
            return((LPARAM)uiHi);

        case WM_DDE_ACK:
        case WM_DDE_ADVISE:
        case WM_DDE_DATA:
        case WM_DDE_POKE:
            /*
             * This must be a valid handle
             */
            UserAssert(GlobalFlags((HANDLE)lParam) != GMEM_INVALID_HANDLE);
            UserAssert(GlobalSize((HANDLE)lParam) == sizeof(DDEPACK));
            //
            // Actual cases where lParam can be reused.
            //
            pDdePack = (PDDEPACK)lParam;
            if (pDdePack == NULL) {
                return(0);          // the only error case
            }
            pDdePack->uiLo = uiLo;
            pDdePack->uiHi = uiHi;
            return(lParam);


        default:
            FreeDDElParam(msgIn, lParam);
            return(MAKELONG((WORD)uiLo, (WORD)uiHi));
        }

    default:
        //
        // Incoming message was not packed ==> PackDDElParam()
        //
        return(PackDDElParam(msgOut, uiLo, uiHi));
    }
}
