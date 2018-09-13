#include "precomp.h"
#pragma hdrstop

#include "i386troj.h"
#include <stddef.h>

typedef struct
{
        LPCSTR  lszLabel;
        DWORD   dwOff;
} LABEL;

// Code copied from 4.2 trojan.c and much removed
// IMPORTANT: Single process only due to the globals below

UOFFSET         uoffTrjBase;
UOFFSET         uoffTrjDllBase;

LABEL           rglbl[10];      // array of labels
UINT            clbl;

LABEL           rglblref[10];   // array of references to labels
UINT            clblref;

#define CELEM_ARRAY(a) ( sizeof(a) / sizeof((a)[0]) )


BOOL FTrjStatic(LPCSTR lsz, BYTE *rgb, UINT *pib)
{
        _ftcscpy(rgb + *pib, lsz);
        *pib += _ftcslen(lsz);
        return TRUE;
}

BOOL FTrjRelOff(LPCSTR lsz, BYTE *rgb, UINT *pib)
{
        *(DWORD*)(rgb + *pib) = (DWORD)uoffTrjBase + (DWORD)lsz;

        *pib += sizeof(DWORD);

        return TRUE;
}

BOOL FTrjDword(LPCSTR lsz, BYTE *rgb, UINT *pib)
{
        *(DWORD*)(rgb + *pib) = (DWORD) lsz;

        *pib += sizeof(DWORD);

        return TRUE;
}

BOOL FTrjJz(LPCSTR lsz, BYTE *rgb, UINT *pib)
{
        UINT i;
        BYTE bOff;

        for (i=0; i<clbl; ++i)
                if (_ftcscmp(lsz, rglbl[i].lszLabel) == 0)
                        break;

        // did we find this label?  if not, it's a forward reference
        if (i<clbl)
        {
                // yes, we found it
                // relative-adjust the offset
                bOff = (BYTE) (rglbl[i].dwOff - (*pib + 2));
        }
        else
        {
                // no, we didn't find it
                // save this label-reference address to be filled in later
                bOff = 0;
                rglblref[clblref].lszLabel = lsz;
                rglblref[clblref].dwOff = *pib + 1;
                clblref++;
                assert(clblref < CELEM_ARRAY(rglblref));
        }

        rgb[(*pib)++] = 0x74;   // "jz"
        rgb[(*pib)++] = bOff;   // where to jump to

        return TRUE;
}

BOOL FTrjLabel(LPCSTR lsz, BYTE *rgb, UINT *pib)
{
        UINT i;
        BYTE bOff;

        rglbl[clbl].lszLabel = lsz;
        rglbl[clbl].dwOff = *pib;
        clbl++;
        assert(clbl < CELEM_ARRAY(rglbl));

        for (i=0; i<clblref; )
        {
                if (_ftcscmp(lsz, rglblref[i].lszLabel) == 0)
                {
                        bOff = *pib - (rglblref[i].dwOff + 1);
                        rgb[rglblref[i].dwOff] = bOff;
                        _fmemmove(&rglblref[i], &rglblref[i+1],
                                (clblref-i-1)*sizeof(LABEL));
                        clblref--;
                }
                else
                {
                        i++;
                }
        }

        return TRUE;
}

BOOL FTrjHinst(LPCSTR lsz, BYTE *rgb, UINT *pib)
{
        *(DWORD*)(rgb + *pib) = (DWORD)uoffTrjDllBase;

        *pib += sizeof(DWORD);

        return TRUE;
}


typedef struct
{
        BOOL    (*pfn)(LPCSTR, BYTE *, UINT *);
        LPCSTR  lsz;
} TROJAN;

/*** FExecTrojan
 *
 * PURPOSE:
 *              Executes a trojan.  Inserts it into the memory space of the
 *              debuggee, and runs it.
 *
 * INPUT:
 *              pthd            Thread in which to execute the trojan.
 *              ptrj            Pointer to array of TROJAN structures.
 *              rgbBuf          Pointer to buffer into which to write the trojan.
 *              cbBuf           Size of rgbBuf.
 *
 * OUTPUT:
 *              returns         TRUE for success, FALSE for failure.
 *              rgbBuf          Copied back from debuggee's memory space after running.
 *              pcxt            Registers after trojan was run.
 *
 ********************************************************************/

BOOL FExecTrojan(HTHDX pthd, TROJAN *ptrj, LPB rgbBuf, UINT cbBuf, CONTEXT *pcxt)
{
    BOOL    fOK = FALSE;
    CONTEXT cxtOld;
    int             i;
    UINT    ib = 0;
    LPB             lpbOld = NULL;
    BOOL    fRestoreBuf = FALSE;
    DWORD   cbGot;
    DEBUG_EVENT de;
    HPRCX   hprc = pthd->hprc;
    HTHDX   hthdCurr;
    
    
    
    // if we do a GetContext here on NT, the EIP points to one past the initial int 3
    // so we use the cached value instead
    cxtOld = pthd->context;
    
    if (IsChicago()) {
        //
        // On Chicago, we cannot write Trojans into the stack memory space,
        // because Chicago trashes that memory when we return control to
        // the debuggee.  So we instead try to write the Trojan to the
        // current EIP.  If that fails, we'll return FALSE.
        //        
        uoffTrjBase = pthd->context.Eip;
        lpbOld = MHAlloc(cbBuf);
        if (!lpbOld) {
            goto end;
        }
        if (!ReadProcessMemory(pthd->hprc->rwHand, (LPCVOID)uoffTrjBase, lpbOld, cbBuf, &cbGot) || (cbGot!=cbBuf) ) {
            goto end;
        }
        fRestoreBuf = TRUE;
    } else {
        //
        // On NT, we'll write the Trojan into the stack memory space.
        // We'll readjust the stack pointer to point just before the
        // Trojan.
        //              Stack before writing Trojan:
        //              +-------------------------------------------------------+
        //              |                                     | user stack data |
        //              +-------------------------------------------------------+
        //                                                   esp
        //
        //              Stack after writing Trojan:
        //              +-------------------------------------------------------+
        //              |       stack used by trojan | trojan | user stack data |
        //              +-------------------------------------------------------+
        //                                          esp
        //
        
        // Round up to DWORD.
        pthd->context.Esp -= ((cbBuf + 3) / 4) * 4;
        uoffTrjBase = pthd->context.Eip = pthd->context.Esp;
    }
    
    clbl = clblref = 0;
    
    for (i=0; ptrj[i].pfn; ++i)
    {
        if (!ptrj[i].pfn(ptrj[i].lsz, rgbBuf, &ib))
            goto end;
    }
    
    assert(ib <= cbBuf);    // make sure we didn't overflow
    
    // Write the trojan code to remote memory
    if (!WriteProcessMemory(pthd->hprc->rwHand, (LPVOID)uoffTrjBase, rgbBuf, cbBuf, &cbGot) || (cbGot!=cbBuf)) {
        goto end;
    }
    
    // set the new EIP/ESP
    VERIFY(SetThreadContext( pthd->rwHand, &pthd->context ));
    
    // Freeze all threads except our one
    for (hthdCurr = hprc->hthdChild; hthdCurr != NULL; hthdCurr = hthdCurr->nextSibling) {
        if (hthdCurr != pthd) {
            if (SuspendThread(hthdCurr->rwHand) == -1L) {
                ; // Internal error;
            }
        }
    }
    
    // let the trojan run, will hit int 3 before sending create thread (we hope!!)
    VERIFY(ContinueDebugEvent(pthd->hprc->pid, pthd->tid, DBG_CONTINUE));
    if (!WaitForDebugEvent(&de, INFINITE)) {
        assert( !"WaitForDebugEvent failed during trojan" );
    } else {
        assert(de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT);
        
        if (de.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT) {
#if DEBUG
            // didn't hit the bp - get some other info to see what is going on
            CONTEXT cxt;
            DWORD ip;
            BYTE buf[20];
            GetThreadContext(pthd->rwHand, &pthd->context);
            ip = cxt.Eip;
            
            ReadProcessMemory( pthd->hprc->rwHand, (LPCVOID)(ip-10), buf, sizeof(buf), &cbGot );
#endif
            assert( !"Trojan code got unexpected event" );
        }
        
        // Get eax
        VERIFY(GetThreadContext(pthd->rwHand, &pthd->context));
        *pcxt = pthd->context;
    }
    
    /* Resume all threads except this one */
    for (hthdCurr = hprc->hthdChild; hthdCurr != NULL; hthdCurr = hthdCurr->nextSibling) {
        if (hthdCurr != pthd) {
            if (ResumeThread(hthdCurr->rwHand) == -1L) {
                assert(FALSE); // Internal error;
            }
        }
    }
    
    // Read back the new contents of the trojan to get return values etc
    if (!ReadProcessMemory(pthd->hprc->rwHand, (LPCVOID)uoffTrjBase, rgbBuf, cbBuf, &cbGot) || (cbGot!=cbBuf) ) {
        goto end;
    }
    
    fOK = TRUE;
    
end:
    
    // Restore old memory
    if (fRestoreBuf) {
        assert(lpbOld);
        VERIFY(WriteProcessMemory(pthd->hprc->rwHand, (LPVOID)uoffTrjBase, lpbOld, cbBuf, &cbGot) &&
            (cbGot == cbBuf));
    }
    
    // Restore old registers always (after memory in case context is stored there)
    pthd->context = cxtOld;
    VERIFY(SetThreadContext(pthd->rwHand, &pthd->context));
    
    if (lpbOld) {
        MHFree(lpbOld);
    }
    
    return fOK;
}



/*** FTrojanCreateSQLThread
 *
 * PURPOSE:
 *              Insert a Trojan into the memory space of the debug process,
 *              to create a SQL thread. Win95 only as NT can use
 *              CreateRemoteThread API call.

 * INPUT:
 *              pthd            The thread from which the load notification came in
 *
 * OUTPUT:
 *              returns         The ID of the thread created
 *
 ********************************************************************/

typedef struct
{
        BYTE rgbAsm[30];
        DWORD id;
        HANDLE handle;
} TRJSQLBUF;

DWORD FTrojanCreateSQLThread( HTHDX pthd, UOFFSET pDebugBreak, HANDLE *pHandle )
{
        // We're going to trojan in the following C code (approximately)
        // into the memory space of the debuggee:
        //
        //DWORD id;
        //HANDLE h = CreateThread( NULL, 0, DebugBreak,
        //                      0, CREATE_SUSPENDED, &id );

        EXPECTED_EVENT *pexevsave;
        CONTEXT cxtNew;
        TRJSQLBUF trjsqlbuf;

        TROJAN Trojan[] =
        {
                FTrjStatic, "\x68",
                FTrjRelOff, (LPCSTR)(offsetof(TRJSQLBUF,id)),           // push offset id
                FTrjStatic, "\x6a\x04",         // push CREATE_SUSPENDED (4)
                FTrjStatic, "\x33\xc0",         // xor eax,eax (cannot do push 0)
                FTrjStatic, "\x50",                     // push eax
                FTrjStatic, "\x68",
                FTrjDword, (LPCSTR)pDebugBreak, // push pDebugBreak
                FTrjStatic, "\x50",                             // push eax (=0)
                FTrjStatic, "\x50",                             // push eax
                FTrjStatic, "\xb8",                             // mov eax,...
                FTrjDword, (LPCSTR)CreateThread,// .. CreateThread
                FTrjStatic,     "\xff\xd0",                     // call eax
                FTrjStatic, "\xa3",
                FTrjRelOff, (LPCSTR)(offsetof(TRJSQLBUF,handle)),       // mov handle,eax
                FTrjStatic, "\xcc",                             // int 3
                0,0
        };

        trjsqlbuf.id = 0;
        trjsqlbuf.handle = 0;

        FExecTrojan( pthd, Trojan, (LPB)&trjsqlbuf, sizeof(trjsqlbuf), &cxtNew );

        if (pHandle)
                *pHandle = trjsqlbuf.handle;

        return trjsqlbuf.id;
}
