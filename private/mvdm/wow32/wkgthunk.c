/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1992, Microsoft Corporation
 *
 *  wkgthunk.C
 *  WOW32 Generic Thunk Mechanism (for OLE 2.0 and others)
 *
 *  History:
 *  Created 11-MARCH-1993 by Matt Felton (mattfe)
 *
--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(wkgthunk.c);


#ifdef i386 // on RISC this is implemented in this file.
extern DWORD WK32ICallProc32MakeCall(DWORD pfn, DWORD cbArgs, DWORD *pArgs);
#endif


#ifdef WX86

typedef
HMODULE
(*PFNWX86LOADX86DLL)(
   LPCWSTR lpLibFileName,
   DWORD dwFlags
   );

typedef
BOOL
(*PFNWX86FREEX86DLL)(
    HMODULE hMod
    );

typedef
PVOID
(*PFNWX86THUNKPROC)(
    PVOID pvAddress,
    PVOID pvCBDispatch,
    BOOL  fNativeToX86
    );

typedef
ULONG
(*PFNWX86EMULATEX86)(
    PVOID  StartAddress,
    ULONG  nParameters,
    PULONG Parameters
    );

typedef
(*PFNWX86THUNKEMULATEX86)(
    ULONG  nParameters,
    PULONG Parameters
    );

typedef
BOOL
(*PFNWX86THUNKINFO)(
    PVOID  ThunkProc,
    PVOID  *pAddress,
    BOOL   *pfNativeToX86
    );

HMODULE hWx86Dll = FALSE;
PFNWX86LOADX86DLL Wx86LoadX86Dll= NULL;
PFNWX86FREEX86DLL Wx86FreeX86Dll= NULL;
PFNWX86THUNKPROC Wx86ThunkProc= NULL;
PFNWX86THUNKEMULATEX86 Wx86ThunkEmulateX86= NULL;
PFNWX86EMULATEX86 Wx86EmulateX86= NULL;
PFNWX86THUNKINFO Wx86ThunkInfo= NULL;

VOID
TermWx86System(
   VOID
   )
{
   if (hWx86Dll) {
       FreeLibrary(hWx86Dll);
       hWx86Dll = NULL;
       Wx86LoadX86Dll = NULL;
       Wx86FreeX86Dll = NULL;
       Wx86ThunkProc = NULL;
       Wx86ThunkEmulateX86 = NULL;
       Wx86EmulateX86 = NULL;
       }
}



BOOL
InitWx86System(
   VOID
   )
{
   if (hWx86Dll) {
       return TRUE;
       }

   hWx86Dll = LoadLibraryExW(L"Wx86.Dll", NULL, 0);
   if (!hWx86Dll) {
       return FALSE;
       }

   Wx86LoadX86Dll = (PFNWX86LOADX86DLL) GetProcAddress(hWx86Dll, "Wx86LoadX86Dll");
   Wx86FreeX86Dll = (PFNWX86FREEX86DLL) GetProcAddress(hWx86Dll, "Wx86FreeX86Dll");
   Wx86ThunkProc  = (PFNWX86THUNKPROC)  GetProcAddress(hWx86Dll, "Wx86ThunkProc");
   Wx86ThunkEmulateX86 = (PFNWX86THUNKEMULATEX86) GetProcAddress(hWx86Dll, "Wx86ThunkEmulateX86");
   Wx86EmulateX86 = (PFNWX86EMULATEX86) GetProcAddress(hWx86Dll, "Wx86EmulateX86");

   if (!Wx86LoadX86Dll || !Wx86FreeX86Dll || !Wx86ThunkProc ||
       !Wx86ThunkEmulateX86 || !Wx86EmulateX86)
     {
       TermWx86System();
       return FALSE;
       }

   return TRUE;
}


BOOL
IsX86Dll(
   HMODULE hModule
   )
{
   if (((ULONG)hModule & 0x01) || !hWx86Dll) {
       return FALSE;
       }

   return (RtlImageNtHeader((PVOID)hModule)->FileHeader.Machine == IMAGE_FILE_MACHINE_I386);
}


ULONG
ThunkProcDispatchP32(
    ULONG p1, ULONG p2, ULONG p3, ULONG p4,
    ULONG p5, ULONG p6, ULONG p7, ULONG p8,
    ULONG p9, ULONG p10, ULONG p11, ULONG p12,
    ULONG p13, ULONG p14, ULONG p15, ULONG p16,
    ULONG p17, ULONG p18, ULONG p19, ULONG p20,
    ULONG p21, ULONG p22, ULONG p23, ULONG p24,
    ULONG p25, ULONG p26, ULONG p27, ULONG p28,
    ULONG p29, ULONG p30, ULONG p31, ULONG p32
    )
{
    ULONG Parameters[32];

    Parameters[0]  = p1;
    Parameters[1]  = p2;
    Parameters[2]  = p3;
    Parameters[3]  = p4;
    Parameters[4]  = p5;
    Parameters[5]  = p6;
    Parameters[6]  = p7;
    Parameters[7]  = p8;
    Parameters[8]  = p9;
    Parameters[9]  = p10;
    Parameters[10] = p11;
    Parameters[11] = p12;
    Parameters[12] = p13;
    Parameters[13] = p14;
    Parameters[14] = p15;
    Parameters[15] = p16;
    Parameters[16] = p17;
    Parameters[17] = p18;
    Parameters[18] = p19;
    Parameters[19] = p20;
    Parameters[20] = p21;
    Parameters[21] = p22;
    Parameters[22] = p23;
    Parameters[23] = p24;
    Parameters[24] = p25;
    Parameters[25] = p26;
    Parameters[26] = p27;
    Parameters[27] = p28;
    Parameters[28] = p29;
    Parameters[29] = p30;
    Parameters[30] = p31;
    Parameters[31] = p32;

    return (*Wx86ThunkEmulateX86)(32, Parameters);
}

#endif





ULONG FASTCALL WK32LoadLibraryEx32W(PVDMFRAME pFrame)
{
    PSZ psz1;
    HINSTANCE hinstance;
    PLOADLIBRARYEX32W16 parg16;

#ifdef i386
    BYTE FpuState[108];

    // Save the 487 state
    _asm {
        lea    ecx, [FpuState]
        fsave  [ecx]
    }
#endif

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETVDMPTR(parg16->lpszLibFile,0,psz1);

    //
    // Make sure the Win32 current directory matches this task's.
    //

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    hinstance = LoadLibraryEx(psz1, (HANDLE)parg16->hFile, parg16->dwFlags);


#ifdef WX86

    //
    // If load failed it might be an x86 binary on risc.
    // try it thru Wx86
    //

    if (!hinstance) {
        LONG LastError;
        NTSTATUS Status;
        ANSI_STRING AnsiString;
        UNICODE_STRING UniString;

        //
        // Prserve the LastError, if wx86 can't handle it, we will restore it
        // so caller won't see any difference.
        //

        LastError = GetLastError();

        if (InitWx86System()) {
            RtlInitString(&AnsiString, psz1);
            if (AreFileApisANSI()) {
                Status = RtlAnsiStringToUnicodeString(&UniString, &AnsiString, TRUE);
                }
            else {
                Status = RtlOemStringToUnicodeString(&UniString, &AnsiString, TRUE);
                }

            if (NT_SUCCESS(Status)) {
                hinstance = (*Wx86LoadX86Dll)(UniString.Buffer, parg16->dwFlags);
                RtlFreeUnicodeString(&UniString);
                }
            }

        if (!hinstance) {
            SetLastError(LastError);
            }
        }
#endif

    FREEARGPTR(parg16);

#ifdef i386
    // Restore the 487 state
    _asm {
        lea    ecx, [FpuState]
        frstor [ecx]
    }
#endif

    return (ULONG)hinstance;
}


ULONG FASTCALL WK32FreeLibrary32W(PVDMFRAME pFrame)
{
    ULONG fResult;
    PFREELIBRARY32W16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

#ifdef WX86
    if (IsX86Dll((HMODULE)parg16->hLibModule)) {
        fResult = (*Wx86FreeX86Dll)((HMODULE)parg16->hLibModule);

        FREEARGPTR(parg16);
        return (fResult);
        }
#endif

    fResult = FreeLibrary((HMODULE)parg16->hLibModule);

    FREEARGPTR(parg16);
    return (fResult);
}


ULONG FASTCALL WK32GetProcAddress32W(PVDMFRAME pFrame)
{
    ULONG lpAddress;
    PSZ psz1;
    PGETPROCADDRESS32W16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETPSZIDPTR(parg16->lpszProc, psz1);


    lpAddress = (ULONG) GetProcAddress((HMODULE)parg16->hModule, psz1);

#ifdef WX86
    if (lpAddress && IsX86Dll((HMODULE)parg16->hModule)) {
        PVOID pv;

        pv = (*Wx86ThunkProc)((PVOID)lpAddress, ThunkProcDispatchP32, TRUE);
        if (pv && pv != (PVOID)-1) {
            lpAddress = (ULONG)pv;
            }
        else {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            lpAddress = 0;
            }
        }
#endif

    FREEARGPTR(parg16);
    return (lpAddress);
}


ULONG FASTCALL WK32GetVDMPointer32W(PVDMFRAME pFrame)
{
    ULONG lpAddress;
    PGETVDMPOINTER32W16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    lpAddress = (ULONG) WOWGetVDMPointer(parg16->lpAddress, 0, parg16->fMode);

    FREEARGPTR(parg16);
    return(lpAddress);
}


#ifndef i386
//
// x86 code in i386\callpr32.asm.
//

DWORD WK32ICallProc32MakeCall(DWORD pfn, DWORD cbArgs, DWORD *pArgs)
{
    typedef int (FAR WINAPIV *FARFUNC)();
    DWORD dw;

#ifdef WX86
    if (Wx86ThunkInfo) {
        PVOID  Address;

        if (Wx86ThunkInfo((PVOID)pfn, &Address, NULL)) {
            return Wx86EmulateX86(Address, cbArgs/sizeof(DWORD), pArgs);
            }
        }
#endif

    if (cbArgs <= (4 * sizeof(DWORD))) {
        dw = ((FARFUNC) pfn) (
                   pArgs[ 0], pArgs[ 1], pArgs[ 2], pArgs[ 3] );
    } else if (cbArgs <= (8 * sizeof(DWORD))) {
        dw = ((FARFUNC) pfn) (
                   pArgs[ 0], pArgs[ 1], pArgs[ 2], pArgs[ 3],
                   pArgs[ 4], pArgs[ 5], pArgs[ 6], pArgs[ 7] );
    } else {
        dw = ((FARFUNC) pfn) (
                   pArgs[ 0], pArgs[ 1], pArgs[ 2], pArgs[ 3],
                   pArgs[ 4], pArgs[ 5], pArgs[ 6], pArgs[ 7],
                   pArgs[ 8], pArgs[ 9], pArgs[10], pArgs[11],
                   pArgs[12], pArgs[13], pArgs[14], pArgs[15],
                   pArgs[16], pArgs[17], pArgs[18], pArgs[19],
                   pArgs[20], pArgs[21], pArgs[22], pArgs[23],
                   pArgs[24], pArgs[25], pArgs[26], pArgs[27],
                   pArgs[28], pArgs[29], pArgs[30], pArgs[31] );
    }

    return dw;
}
#endif


ULONG FASTCALL WK32ICallProc32W(PVDMFRAME pFrame)
{

    register DWORD dwReturn;
    PICALLPROC32W16 parg16;
    UNALIGNED DWORD *pArg;
    DWORD  fAddress;
    BOOL    fSourceCDECL;
    UINT    cParams;
    UINT    nParam;
    UNALIGNED DWORD *lpArgs;
    DWORD   dwTemp[32];

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    fSourceCDECL = HIWORD(parg16->cParams) & CPEX32_SOURCE_CDECL;
    // fDestCDECL =   HIWORD(parg16->cParams) & CPEX32_DEST_CDECL; // not needed

    // We only support up to 32 parameters

    cParams = LOWORD(parg16->cParams);

    if (cParams > 32)
	return(0);

    // Don't call to Zero

    if (parg16->lpProcAddress == 0) {
	LOGDEBUG(LOG_ALWAYS,("WK32ICallProc32 - Error calling to 0 not allowed"));
	return(0);
    }

    lpArgs = &parg16->p1;

    // Convert Any 16:16 Addresses to 32 bit
    // flat as required by fAddressConvert

    pArg = lpArgs;

    fAddress = parg16->fAddressConvert;

    while (fAddress != 0) {
        if (fAddress & 0x1) {
            *pArg = (DWORD) GetPModeVDMPointer(*pArg, 0);
        }
        pArg++;
        fAddress = fAddress >> 1;
    }

    //
    // The above code is funny.  It means that parameter translation will
    // occur before accounting for the calling convention.  This means that
    // they will be specifying the bit position for CallProc32W by counting the
    // parameters from the end, whereas with CallProc32ExW, they count from the
    // beginning.  Weird for pascal, but that is compatible with what we've
    // already shipped.  cdecl should be more understandable.
    //

    //
    // Make sure the Win32 current directory matches this task's.
    //

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    if (!fSourceCDECL) {

        //
        // Invert the parameters
        //
        pArg = lpArgs;

        nParam = cParams;
        while ( nParam != 0 ) {
            --nParam;
            dwTemp[nParam] = *pArg;
            pArg++;
        }
    }  else  {

        //
        // To make usage of WK32ICallProc32MakeCall consistent on all
        // platforms we copy the parameters to dwTemp, to ensure the parameter
        // array is dword aligned. Impact is insignificnt since primary calling
        // convention for win16 is PASCAL.
        //

        memcpy(dwTemp, lpArgs, cParams * sizeof(DWORD));
    }


    //
    // dwTemp now points to the very first parameter in any calling convention
    // And all of the parameters have been appropriately converted to flat ptrs.
    //
    // Note that on the 32-bit side, the parameter ordering is always push
    // right-to-left, so the first parameter is at the lowest address.  This
    // is true for x86 _cdecl and _stdcall as well as RISC, which has only
    // _cdecl.
    //

    //
    // On x86 we call an assembly routine to actually make the call to
    // the client's Win32 routine.  The code is much more compact
    // this way, and it's the only way we can be compatible with
    // Win95's implementation, which cleans up the stack if the
    // routine doesn't.
    //
    // This assembly routine "pushes" the arguments by copying
    // them as a block, so they must be in the proper order for
    // the destination calling convention.
    //
    // The RISC C code for this routine is just below.  On RISC the caller
    // is always responsible for cleaning up the stack, so that shouldn't
    // be a problem.
    //

    dwReturn = WK32ICallProc32MakeCall(
                   parg16->lpProcAddress,
                   cParams * sizeof(DWORD),
                   dwTemp
                   );


    FREEARGPTR(parg16);
    return(dwReturn);
}


//
// Chicago has WOWGetVDMPointerFix, which is just like WOWGetVDMPointer
// but also calls GlobalFix to keep the 16-bit memory from moving.  It
// has a companion WOWGetVDMPointerUnfix, which is basically a Win32-callable
// GlobalUnfix.
//
// Chicago found the need for these functions because their global heaps
// can be rearranged while Win32 code called from a generic thunk is
// executing.  In Windows NT, global memory cannot move while in a thunk
// unless the thunk calls back to the 16-bit side.
//
// Our exported WOWGetVDMPointerFix is simply an alias to WOWGetVDMPointer --
// it does *not* call GlobalFix because it is not needed in 99% of the
// cases.  WOWGetVDMPointerUnfix is implemented below as NOP.
//

VOID WOWGetVDMPointerUnfix(VPVOID vp)
{
    UNREFERENCED_PARAMETER(vp);

    return;
}


//
// Yielding functions allow 32-bit thunks to avoid 4 16<-->32 transitions
// involved in calling back to 16-bit side to call Yield or DirectedYield,
// which are thunked back to user32.
//

VOID WOWYield16(VOID)
{
    //
    // Since WK32Yield (the thunk for Yield) doesn't use pStack16,
    // just call it rather than duplicate the code.
    //

    WK32Yield(NULL);
}

VOID WOWDirectedYield16(WORD hTask16)
{
    //
    // This is duplicating the code of WK32DirectedYield, the
    // two must be kept synchronized.
    //

    BlockWOWIdle(TRUE);

    (pfnOut.pfnDirectedYield)(THREADID32(hTask16));

    BlockWOWIdle(FALSE);
}


#ifdef DEBUG // called by test code in checked wowexec

DWORD WINAPI WOWStdCall32ArgsTestTarget(
                DWORD p1,
                DWORD p2,
                DWORD p3,
                DWORD p4,
                DWORD p5,
                DWORD p6,
                DWORD p7,
                DWORD p8,
                DWORD p9,
                DWORD p10,
                DWORD p11,
                DWORD p12,
                DWORD p13,
                DWORD p14,
                DWORD p15,
                DWORD p16,
                DWORD p17,
                DWORD p18,
                DWORD p19,
                DWORD p20,
                DWORD p21,
                DWORD p22,
                LPDWORD p23,
                DWORD p24,
                DWORD p25,
                DWORD p26,
                DWORD p27,
                DWORD p28,
                DWORD p29,
                DWORD p30,
                DWORD p31,
                LPDWORD p32
                )
{
    return ((((p1+p2+p3+p4+p5+p6+p7+p8+p9+p10) -
              (p11+p12+p13+p14+p15+p16+p17+p18+p19+p20)) << p21) +
            ((p22+*p23+p24+p25+p26) - (p27+p28+p29+p30+p31+*p32)));
}

#endif // DEBUG
