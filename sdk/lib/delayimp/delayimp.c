/*
 * PROJECT:         ReactOS SDK Library
 * LICENSE:         LGPL, see LGPL.txt in top level directory.
 * FILE:            lib/sdk/delayimp/delayimp.c
 * PURPOSE:         Library for delay importing from dlls
 * PROGRAMMERS:     Timo Kreuzer <timo.kreuzer@reactos.org>
 *                  Mark Jansen
 *
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <delayimp.h>

/**** Linker magic: provide a default (NULL) pointer, but allow the user to override it ****/

/* The actual items we use */
PfnDliHook __pfnDliNotifyHook2;
PfnDliHook __pfnDliFailureHook2;

#if !defined(__GNUC__)
/* The fallback symbols */
PfnDliHook __pfnDliNotifyHook2Default = NULL;
PfnDliHook __pfnDliFailureHook2Default = NULL;

/* Tell the linker to use the fallback symbols */
#if defined (_M_IX86)
#pragma comment(linker, "/alternatename:___pfnDliNotifyHook2=___pfnDliNotifyHook2Default")
#pragma comment(linker, "/alternatename:___pfnDliFailureHook2=___pfnDliFailureHook2Default")
#else
#pragma comment(linker, "/alternatename:__pfnDliNotifyHook2=__pfnDliNotifyHook2Default")
#pragma comment(linker, "/alternatename:__pfnDliFailureHook2=__pfnDliFailureHook2Default")
#endif
#endif


/**** Helper functions to convert from RVA to address ****/

FORCEINLINE
unsigned
IndexFromPImgThunkData(PCImgThunkData pData, PCImgThunkData pBase)
{
    return pData - pBase;
}

extern const IMAGE_DOS_HEADER __ImageBase;

FORCEINLINE
PVOID
PFromRva(RVA rva)
{
    return (PVOID)(((ULONG_PTR)(rva)) + ((ULONG_PTR)&__ImageBase));
}


/**** load helper ****/

FARPROC WINAPI
__delayLoadHelper2(PCImgDelayDescr pidd, PImgThunkData pIATEntry)
{
    DelayLoadInfo dli = {0};
    int index;
    PImgThunkData pIAT;
    PImgThunkData pINT;
    HMODULE *phMod;

    pIAT = PFromRva(pidd->rvaIAT);
    pINT = PFromRva(pidd->rvaINT);
    phMod = PFromRva(pidd->rvaHmod);
    index = IndexFromPImgThunkData(pIATEntry, pIAT);

    dli.cb = sizeof(dli);
    dli.pidd = pidd;
    dli.ppfn = (FARPROC*)&pIAT[index].u1.Function;
    dli.szDll = PFromRva(pidd->rvaDLLName);
    dli.dlp.fImportByName = !IMAGE_SNAP_BY_ORDINAL(pINT[index].u1.Ordinal);
    if (dli.dlp.fImportByName)
    {
        /* u1.AdressOfData points to a IMAGE_IMPORT_BY_NAME struct */
        PIMAGE_IMPORT_BY_NAME piibn = PFromRva((RVA)pINT[index].u1.AddressOfData);
        dli.dlp.szProcName = (LPCSTR)&piibn->Name;
    }
    else
    {
        dli.dlp.dwOrdinal = IMAGE_ORDINAL(pINT[index].u1.Ordinal);
    }

    if (__pfnDliNotifyHook2)
    {
        dli.pfnCur = __pfnDliNotifyHook2(dliStartProcessing, &dli);
        if (dli.pfnCur)
        {
            pIAT[index].u1.Function = (DWORD_PTR)dli.pfnCur;
            if (__pfnDliNotifyHook2)
                __pfnDliNotifyHook2(dliNoteEndProcessing, &dli);

            return dli.pfnCur;
        }
    }

    dli.hmodCur = *phMod;

    if (dli.hmodCur == NULL)
    {
        if (__pfnDliNotifyHook2)
            dli.hmodCur = (HMODULE)__pfnDliNotifyHook2(dliNotePreLoadLibrary, &dli);
        if (dli.hmodCur == NULL)
        {
            dli.hmodCur = LoadLibraryA(dli.szDll);
            if (dli.hmodCur == NULL)
            {
                dli.dwLastError = GetLastError();
                if (__pfnDliFailureHook2)
                    dli.hmodCur = (HMODULE)__pfnDliFailureHook2(dliFailLoadLib, &dli);

                if (dli.hmodCur == NULL)
                {
                    ULONG_PTR args[] = { (ULONG_PTR)&dli };
                    RaiseException(VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND), 0, 1, args);

                    /* If we survive the exception, we are expected to use pfnCur directly.. */
                    return dli.pfnCur;
                }
            }
        }
        *phMod = dli.hmodCur;
    }

    dli.dwLastError = ERROR_SUCCESS;

    if (__pfnDliNotifyHook2)
        dli.pfnCur = (FARPROC)__pfnDliNotifyHook2(dliNotePreGetProcAddress, &dli);
    if (dli.pfnCur == NULL)
    {
        /* dli.dlp.szProcName might also contain the ordinal */
        dli.pfnCur = GetProcAddress(dli.hmodCur, dli.dlp.szProcName);
        if (dli.pfnCur == NULL)
        {
            dli.dwLastError = GetLastError();
            if (__pfnDliFailureHook2)
               dli.pfnCur = __pfnDliFailureHook2(dliFailGetProc, &dli);

            if (dli.pfnCur == NULL)
            {
                ULONG_PTR args[] = { (ULONG_PTR)&dli };
                RaiseException(VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND), 0, 1, args);
            }

            //return NULL;
        }
    }

    pIAT[index].u1.Function = (DWORD_PTR)dli.pfnCur;
    dli.dwLastError = ERROR_SUCCESS;

    if (__pfnDliNotifyHook2)
        __pfnDliNotifyHook2(dliNoteEndProcessing, &dli);

    return dli.pfnCur;
}

