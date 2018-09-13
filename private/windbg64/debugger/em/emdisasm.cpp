/**** EMDISASM.C - EM Lego disassembler interface                          *
 *                                                                         *
 *                                                                         *
 *  Copyright <C> 1995, Microsoft Corp                                     *
 *                                                                         *
 *  Created: September 18, 1995 by RafaelL                                 *
 *                                                                         *
 *  Revision History:                                                      *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#include "emdp.h"

#include <simpldis.h>


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY



#define MAXL     20
#define CCHMAX   256


LPSTR
_SHGetSymbol(
    LPADDR  addr1,
    SOP     sop,
    LPADDR  addr2,
    LPSTR   szName,
    LPDWORD Delta
    )
{
    ODR             odr;
    LPSTR   lpstr;

    odr.lszName = szName;
    lpstr = SHGetSymbol (addr1, addr2, sop, &odr);

    if (Delta) {
        *Delta = odr.dwDeltaOff;
    }

    return lpstr;
}


XOSD
Assemble (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPTSTR lszInput
    )
{
    return xosdUnsupported;
}


int
CvRegFromSimpleReg(
    MPT     mpt,
    int     regInstr
    )
{
    switch (mpt) {
    case mptix86:
        switch(regInstr) {
            case SimpleRegEax: return CV_REG_EAX;
            case SimpleRegEcx: return CV_REG_ECX;
            case SimpleRegEdx: return CV_REG_EDX;
            case SimpleRegEbx: return CV_REG_EBX;
            case SimpleRegEsp: return CV_REG_ESP;
            case SimpleRegEbp: return CV_REG_EBP;
            case SimpleRegEsi: return CV_REG_ESI;
            case SimpleRegEdi: return CV_REG_EDI;
        }
        break;

    case mptdaxp:
        return (regInstr + CV_ALPHA_IntV0);

    case mptia64:
        return (regInstr + CV_IA64_IntZero);
 
    }
    return (0);
}

int
SimpleArchFromMPT(
    MPT mpt
    )
{
    switch (mpt) {
        case mptix86:
            return Simple_Arch_X86;

        case mptdaxp:
            return Simple_Arch_AlphaAxp;

        case mptia64:
            return Simple_Arch_IA64;

        default:
            return -1;
    }
}



DWORDLONG
EmQwGetreg(
    PVOID   pv,
    int     regInstr
    )
{
    HTHD hthd = (HTHD)pv;

    XOSD        xosd;
    DWORDLONG   retVal;
    HPID        hpid = HpidFromHthd(hthd);

    xosd = GetRegValue(hpid,
                       HtidFromHthd(hthd),
                       CvRegFromSimpleReg(MPTFromHthd(hthd), regInstr),
                       &retVal
                       );

    return (xosd == xosdNone) ? retVal : 0;
}


size_t
WINAPI
EmCchRegrel(
    PVOID       pv,
    DWORD       ipaddr,
    int         reg,
    DWORD       offset,
    PCHAR       symbol,
    size_t      symsize,
    PDWORD      pDisp
    )
{
    HTHD hthd = (HTHD)pv;

    CHAR    string[512];
    DWORD   dw;
    ADDR    AddrIP;
    ADDR    AddrData;
    LPTHD   lpthd = (LPTHD)LLLock(hthd);
    MPT     mpt = MPTFromHthd(hthd);

    //
    // We will get regrel calls on registers that the compiler
    // does not use as a frame register.  Don't try to resolve
    // those.
    //

    switch (mpt) {
        case mptix86:
            if (reg != SimpleRegEbp) {
                *pDisp = 0;
                symbol[0] = 0;
                return 0;
            }
            break;
        case mptdaxp:
            if (reg != CV_ALPHA_IntSP) {
                *pDisp = 0;
                symbol[0] = 0;
                return 0;
            }
            break;
        case mptia64:
            if (reg != CV_IA64_IntSp) {
                *pDisp = 0;
                symbol[0] = 0;
                return 0;
            }
    }


    AddrInit(&AddrIP,
             NULL,
             (SEGMENT)(lpthd->fFlat? 0 : (ipaddr >> 16)), //seg
             SE32To64( lpthd->fFlat? ipaddr : (ipaddr & 0xffff) ),
             lpthd->fFlat,
             lpthd->fOff32,
             0, // li
             lpthd->fReal);

    AddrInit(&AddrData,
             NULL,
             (SEGMENT)(lpthd->fFlat? 0 : (offset >> 16)), //seg
             SE32To64( (lpthd->fFlat? offset : (offset & 0xffff)) ),
             lpthd->fFlat,
             lpthd->fOff32,
             0,
             (USHORT)lpthd->fReal);

    LLUnlock(hthd);

    if (_SHGetSymbol (&AddrData, sopStack, &AddrIP, string, &dw)) {
        _tcsncpy( symbol, string, symsize );
        *pDisp = dw;
    } else {
        *pDisp = 0;
        symbol[0] = 0;
    }

    return _tcslen(symbol);
}



size_t
EmCchAddr(
    PVOID       pv,
    ULONG       offset,
    char        *symbol,
    size_t      symsize,
    DWORD       *pDisp
    )
{
        HTHD hthd = (HTHD)pv;

    CHAR    string[512];
    DWORD   dw;
    ADDR    AddrIP;
    ADDR    AddrData;
    LPTHD   lpthd = (LPTHD)LLLock(hthd);

    AddrInit(&AddrData,
             NULL,
             (SEGMENT)(lpthd->fFlat? 0 : (offset >> 16)), //seg
             SE32To64( lpthd->fFlat ? offset : (offset & 0xffff) ),
             lpthd->fFlat,
             lpthd->fOff32,
             0, // li
             (USHORT)lpthd->fReal);

    LLUnlock(hthd);

    AddrIP = AddrData;


        if (_SHGetSymbol (&AddrData, sopNone, &AddrIP, string,  &dw )) {
                _tcsncpy( symbol, string, symsize );
                *pDisp = dw;
        } else {
                *pDisp = 0;
                symbol[0] = 0;
        }

    return _tcslen(symbol);
}


size_t
EmCchFixup(
    PVOID       pv,
    DWORD       ipaddr,
    ULONG       offset,
    size_t      size,
    PCHAR       symbol,
    size_t      symsize,
    DWORD       *pDisp
    )
{
    HTHD hthd = (HTHD)pv;

    CHAR    string[512];
    DWORD   dw;
    ADDR    AddrIP;
    ADDR    AddrData;
    LPTHD   lpthd = (LPTHD)LLLock(hthd);
    DWORD64 dw64Tmp;

    if (size != sizeof(DWORD)) {
        return 0;
    }

    //
    // Sign extend all addresses
    //
    dw64Tmp = (DWORD64) ( lpthd->fFlat ? ipaddr : (ipaddr & 0xffff) );
    if (sizeof(ipaddr) == sizeof(DWORD)) {
        dw64Tmp = SE32To64( (DWORD) dw64Tmp );
    }
    AddrInit(&AddrIP,
             NULL,
             (SEGMENT)(lpthd->fFlat? 0 : (ipaddr >> 16)), //seg
             dw64Tmp,
             lpthd->fFlat,
             lpthd->fOff32,
             0, // li
             (USHORT)lpthd->fReal);

    //
    // Sign extend all addresses
    //
    dw64Tmp = (DWORD64) ( lpthd->fFlat ? offset : (offset & 0xffff) );
    if (sizeof(offset) == sizeof(DWORD)) {
        dw64Tmp = SE32To64( (DWORD) dw64Tmp );
    }
    AddrInit(&AddrData,
             NULL,
             (SEGMENT)(lpthd->fFlat? 0 : (offset >> 16)), //seg
             dw64Tmp,
             lpthd->fFlat,
             lpthd->fOff32,
             0,
             (USHORT)lpthd->fReal);

    LLUnlock(hthd);

    if (size==sizeof(DWORD)) {
        // go get the DWORD at address 'offset'
        DWORD dw;
        DWORD res;
        XOSD xosd = ReadBuffer( HpidFromHthd(hthd), HtidFromHthd(hthd), &AddrData, size, (LPBYTE)&dw, &res );
        if (xosd==xosdNone) {
            SE_SetAddrOff( &AddrData, dw );
        } else {
            //
            // couldn't read the memory
            //
            *pDisp = 0;
            symbol[0] = 0;
            return 0;
        }
    }

    if (_SHGetSymbol (&AddrData, sopNone, &AddrIP, string,  &dw )) {
        _tcsncpy( symbol, string, symsize );
        *pDisp = dw;
    } else {
        *pDisp = 0;
        symbol[0] = 0;
    }

    return _tcslen(symbol);
}

int
AddString(
    LPSTR *ppchOut,
    LPINT  pichCur,
    LPINT  pcchMax,
    LPSTR  string,
    BOOL   PackSpaces
    )

/*++

Routine Description:

    Add a string to a packed list of strings.

Arguments:

    ppchOut - Supplies a pointer to a pointer to the position of the next
        entry in the string.  Returns the new position after this string
        is added.

    pichCur - Supplies a pointer to the current index in the string.  Returns
        the new index.

    pcchMax - Supplies a pointer to the number of characters available in the
        string.  Returns the new value of same.

    string - Supplies the string to add to the list.

    PackSpaces - Supplies flag telling it to remove spaces from the string.

Return Value:

    Index to beginning of string (original ichCur).  Returns -1 if the string
    could not be added.

--*/
{
    int r = -1;

    if (!PackSpaces) {
        int l = _tcslen(string);
        if (l >= *pcchMax) {
            l = *pcchMax - 1;
        }

        if (l >= 0) {
            _tcsncpy(*ppchOut, string, l);
            (*ppchOut)[l] = 0;
            *pcchMax -= (l+1);
            r = *pichCur;
            *pichCur += (l+1);
            *ppchOut += (l+1);
        }

    } else {
        LPSTR pout = *ppchOut;
        int max = *pcchMax;
        int ich = *pichCur;
        r = *pichCur;
        while (max > 1 && *string) {
            if (_istspace(*string)) {
                string = _tcsinc(string);
            } else {
                if (_istleadbyte(*string)) {
                    if (max < 2) {
                        r = -1;
                        break;
                    }
                    *pout++ = *string++;
                    max--;
                    ich++;
                }
                *pout++ = *string++;
                max--;
                ich++;
            }
        }
        if (r != -1) {
            *pout++ = 0;
            *pcchMax = max-1;
            *pichCur = ich+1;
            *ppchOut = pout;
        }
    }
    return r;
}

XOSD
Disasm (
    HPID   hpid,
    HTID   htid,
    LPSDI  lpsdi
    )
{
    XOSD        xosd      = xosdNone;
    DWORD       dop       = lpsdi->dop;
    DWORD       cb;
    int         cbUsed    = 0;
    ADDR        addr;
    BYTE        rgb[MAXL];
    PBYTE       prgb;
    int         Bytes;
    DWORD       dwTgtMem;
    SIMPLEDIS   Sdis;
    HPRC        hprc = ValidHprcFromHpid(hpid);
    HTHD        hthd = HthdFromHtid( hprc, htid );
    MPT         mpt = MPTFromHthd(hthd);


    static char String[CCHMAX];
    LPSTR       lpchOut;
    int         ichCur;
    int         cchMax;

#if defined(_IA64_)
    //
    // Required for IA64 until MSDIS.LIB supports IA64
    // Remove when IA64 support is available in MSDIS.LIB
    //
    if (mpt == mptia64) { // in ia64dis.cpp - uses Falcon
        extern XOSD IA64Disasm(HPID, HTID, LPSDI);
        return IA64Disasm(hpid, htid, lpsdi);
    }
#endif

    lpsdi->ichAddr      = -1;
    lpsdi->ichBytes     = -1;
    lpsdi->ichOpcode    = -1;
    lpsdi->ichOperands  = -1;
    lpsdi->ichComment   = -1;
    lpsdi->ichEA0       = -1;
    lpsdi->ichEA1       = -1;
    lpsdi->ichEA2       = -1;

    lpsdi->cbEA0        =  0;
    lpsdi->cbEA1        =  0;
    lpsdi->cbEA2        =  0;

    lpsdi->fAssocNext   =  0;

    lpsdi->lpch         = String;



    //ADDR_IS_FLAT( addrStart ) = TRUE;

    //if (!Memory) {
        xosd = ReadBuffer(hpid, htid, &lpsdi->addr, MAXL, rgb, (unsigned long *) &cb);
        if (xosd != xosdNone) {
            cb = 0;
        }
        prgb = rgb;
    //}
    //else {
        //prgb = (BYTE *) Memory;
        //cb  = (DWORD)MemorySize;
    //}

    if ( cb == 0 ) {

        //
        // Even if we can't read memory we still need to send back the formatted address
        // so the shell can display it.
        // HACK HACK: We make up the address string here from the passed in address. We need to
        // come up with a better solution so we don't have to keep the address formatting
        // between the disassembler and the EM in sync. [sanjays]

        if ((dop & dopAddr) || (dop & dopFlatAddr)) {

            //
            // address of instruction
            //
            sprintf(String,"%08X", GetAddrOff(lpsdi->addr));
            lpsdi->ichAddr = 0;
        }

        //cbUsed = 0;
        cbUsed = 1;
        xosd = xosdGeneral;

    } else {

        if (dop & dopSym) {
            Bytes = SimplyDisassemble(
                prgb,                         // code ptr
                cb,                           // bytes
                (DWORD)GetAddrOff(lpsdi->addr),
                SimpleArchFromMPT(mpt),
                &Sdis,
                EmCchAddr,
                EmCchFixup,
                EmCchRegrel,
                EmQwGetreg,
                (PVOID)hthd
                );
        } else {
            Bytes = SimplyDisassemble(
                prgb,                         // code ptr
                cb,                           // bytes
                (DWORD)GetAddrOff(lpsdi->addr),
                SimpleArchFromMPT(mpt),
                &Sdis,
                NULL,
                NULL,
                NULL,
                EmQwGetreg,
                (PVOID)hthd
                );
        }


        if (Bytes < 0) {
            cbUsed = -Bytes;
            //xosd = xosdGeneral;
        } else {
            cbUsed = Bytes;
        }

        //
        // unpack Sdis
        //


        //
        // fill in addresses, whether asked for or not
        //

        lpsdi->cbEA0 = Sdis.cbEA0;
        lpsdi->cbEA1 = Sdis.cbEA1;
        lpsdi->cbEA2 = Sdis.cbEA2;

        if (lpsdi->cbEA0) {
            AddrInit( &lpsdi->addrEA0,
                      NULL,
                      0,    // SEG
                      (sizeof(Sdis.dwEA0)==8) ? Sdis.dwEA0 : SE32To64(Sdis.dwEA0),
                      1,    // flat
                      1,    // off32
                      0,    // LI
                      0     // real
                      );
        }

        if (lpsdi->cbEA1) {
            AddrInit( &lpsdi->addrEA1,
                      NULL,
                      0,    // SEG
                      (sizeof(Sdis.dwEA1)==8) ? Sdis.dwEA1 : SE32To64(Sdis.dwEA1),
                      1,    // flat
                      1,    // off32
                      0,    // LI
                      0     // real
                      );
        }

        if (lpsdi->cbEA2) {
            AddrInit( &lpsdi->addrEA2,
                      NULL,
                      0,    // SEG
                      (sizeof(Sdis.dwEA2)==8) ? Sdis.dwEA2 : SE32To64(Sdis.dwEA2),
                      1,    // flat
                      1,    // off32
                      0,    // LI
                      0     // real
                      );
        }


        //
        // initialize packed string
        //
        lpchOut   = String;
        ichCur    = 0;
        cchMax    = CCHMAX;

        if ((dop & dopAddr) || (dop & dopFlatAddr)) {

            //
            // address of instruction
            //

            lpsdi->ichAddr = AddString(&lpchOut, &ichCur, &cchMax, Sdis.szAddress, FALSE);

        }

        if (dop & dopRaw) {

            //
            // Raw bytes
            //

            lpsdi->ichBytes = AddString(&lpchOut, &ichCur, &cchMax, Sdis.szRaw, TRUE);

        }

        if (dop & dopOpcode) {

            //
            // opcode...
            //

            lpsdi->ichOpcode = AddString(&lpchOut, &ichCur, &cchMax, Sdis.szOpcode, FALSE);

        }

        if (dop & dopOperands) {

            //
            // operands...
            //

            lpsdi->ichOperands = AddString(&lpchOut, &ichCur, &cchMax, Sdis.szOperands, FALSE);

        }

        {

            //
            // comment
            //

            lpsdi->ichComment   = -1;

        }

        if (dop & dopEA) {

            //
            // show EA(s)
            //

            if (lpsdi->cbEA0) {
                if (!Sdis.cbMemref) {
                    lpsdi->ichEA0 = AddString(&lpchOut, &ichCur, &cchMax, Sdis.szEA0, FALSE);
                } else {
                    ADDR addr = lpsdi->addr;
                    
                    GetAddrOff(addr) = 
                            (sizeof(Sdis.dwEA0)==8) ? Sdis.dwEA0 : SE32To64(Sdis.dwEA0);
                    
                    xosd = ReadBuffer (hpid,
                                       htid,
                                       &addr,
                                       Sdis.cbMemref,
                                       rgb,
                                       &cb
                                       );
                    if (xosd == xosdNone) {
                        char tmpString[CCHMAX];
                        LPSTR lp;
                        DWORD i;
                        switch (Sdis.cbMemref) {
                        case 4:
                            sprintf(tmpString, "%s%08X", Sdis.szEA0, *(DWORD UNALIGNED *)rgb);
                            break;
                        case 2:
                            sprintf(tmpString, "%s%04X", Sdis.szEA0, *(WORD UNALIGNED *)rgb);
                            break;
                        default:
                            sprintf(tmpString, "%s", Sdis.szEA0);
                            lp = tmpString + strlen(tmpString);
                            for (i = 0; i < Sdis.cbMemref; i++) {
                                sprintf(lp, "%02X ", rgb[i]);
                                lp += 3;
                            }
                        }
                        lpsdi->ichEA0 = AddString(&lpchOut, &ichCur, &cchMax, tmpString, FALSE);
                    } else {
                        lpsdi->ichEA0 = AddString(&lpchOut, &ichCur, &cchMax, Sdis.szEA0, FALSE);
                    }
                }
            }

            if (lpsdi->cbEA1) {
                lpsdi->ichEA1 = AddString(&lpchOut, &ichCur, &cchMax, Sdis.szEA1, FALSE);
            }

            if (lpsdi->cbEA2) {
                lpsdi->ichEA2 = AddString(&lpchOut, &ichCur, &cchMax, Sdis.szEA2, FALSE);
            }
        }

    }

    GetAddrOff ( lpsdi->addr ) += cbUsed;

    return xosd;
}


XOSD
BackDisasm(
    HPID hpid,
    HTID htid,
    LPGPIS lpgpis
    )

/*++

Routine Description:

    This will find the instruction which ends nearest to the supplied address
    without consuming the supplied address.

    On machines with fixed instruction size and alignment, this is trivially
    accomplished with arithmetic.

    On machines with variable instruction size, this is done by disassembling
    instructions until a closest fit is found.

    This implementation will not consume the supplied address, but will accept
    either a) the first (longest) match which consumes the byte before the
    address, or b) the match which ends closest to the address.

Arguments:

    hpid -

    htid -

    lpgpis -


Return Value:


--*/
{
    HPRC        hprc = ValidHprcFromHpid(hpid);
    HTHD        hthd = HthdFromHtid( hprc, htid );
    MPT         mpt = MPTFromHthd(hthd);
    SIMPLEDIS   Sdis;

    if (GetAddrOff(*lpgpis->lpaddr) == 0) {

        return xosdBadAddress;

    } else if (mpt != mptix86) {

        //
        // all but X86 can assume DWORD alignment opcodes
        //

        SetAddrOff((lpgpis->lpaddr), GetAddrOff(*(lpgpis->lpaddr))-4);
        *(lpgpis->lpuoffset) = GetAddrOff(*(lpgpis->lpaddr)) & 3;
        SetAddrOff((lpgpis->lpaddr),
                 GetAddrOff(*(lpgpis->lpaddr)) - *(lpgpis->lpuoffset));

        return xosdNone; // Hack for MIPS&Alpha doesn't check page r/w

    } else {

        //
        // x86 is more painful
        // we start 20 bytes before and disassemble forwards until we hit it
        // if we miss it, we start again from the next byte     down
        //

        const int arch = SimpleArchFromMPT(mpt);
        const int X86_BACK_MAX = 20;
        UOFFSET endOffset = GetAddrOff(*lpgpis->lpaddr);         // where we want to end up
        ADDR startAddr = *lpgpis->lpaddr;
        DWORD cbTry;
        BYTE rgb[X86_BACK_MAX];

        // we start X86_BACK_MAX bytes before (checking that we're not too early)
        if (GetAddrOff(startAddr) < X86_BACK_MAX) {
            GetAddrOff(startAddr) = 0;
        } else {
            GetAddrOff(startAddr) -= X86_BACK_MAX;
        }

        XOSD xosd = ReadBuffer(hpid, htid, &startAddr, X86_BACK_MAX, rgb, &cbTry);

        if (xosd != xosdNone) {
            //
            // nothing there.  just decrement 1 and return.
            //
            SetAddrOff( lpgpis->lpaddr, endOffset - 1 );
            return xosd;
        }



        LPBYTE pbTry = rgb;
        UOFFSET tryOffset = GetAddrOff(startAddr);

        while (cbTry > 0) {

            LPBYTE pbCurr = pbTry;
            DWORD cbCurr = cbTry;
            UOFFSET currOffset = tryOffset;

            while (1) {

                int Bytes = SimplyDisassemble(pbCurr,
                                              cbCurr,
                                              (DWORD)currOffset,
                                              arch,
                                              &Sdis,
                                              EmCchAddr,
                                              EmCchFixup,
                                              EmCchRegrel,
                                              EmQwGetreg,
                                              (PVOID)hthd
                                              );
                if (Bytes < 0) {

                    //
                    // no instruction found - slide ahead one
                    // byte and start again.
                    //

                    pbTry += 1;
                    cbTry -= 1;
                    tryOffset += 1;
                    break;
                }

                if (Bytes == 0) {

                    //
                    // this is not supposed to happen
                    //

                    assert(!"SimplyDisassemble returned 0!");
                    SetAddrOff( lpgpis->lpaddr, endOffset - 1 );
                    return xosdGeneral;

                }

                if (Bytes > (int)cbCurr) {

                    //
                    // this is not supposed to happen
                    //

                    assert(!"SimplyDisassemble consumed too many bytes!");
                    SetAddrOff( lpgpis->lpaddr, endOffset - 1 );
                    return xosdGeneral;

                }

                if (Bytes < (int)cbCurr) {

                    //
                    // so far, so good.
                    //

                    cbCurr -= Bytes;
                    pbCurr += Bytes;
                    currOffset += Bytes;
                    continue;
                }


                if (Bytes == (int)cbCurr) {

                    //
                    // perfect fit
                    //

                    // currOffset is the one we want.

                    SetAddrOff(lpgpis->lpaddr, currOffset);
                    return xosdNone;
                }

            }

        }

        //
        // didn't find anything at all, so bail out and
        // pretend it's a one byte op-code
        //

        SetAddrOff( lpgpis->lpaddr, endOffset - 1 );
        return xosdNone;
    }

}
