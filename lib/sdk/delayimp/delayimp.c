/*
 * PROJECT:         ReactOS SDK Library
 * LICENSE:         LGPL, see LGPL.txt in top level directory.
 * FILE:            lib/sdk/delayimp/delayimp.c
 * PURPOSE:         Library for delay importing from dlls
 * PROGRAMMERS:     Timo Kreuzer <timo.kreuzer@reactos.org>
 *
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <delayimp.h>

/**** load helper ****/

FARPROC WINAPI
__delayLoadHelper2(PCImgDelayDescr pidd, PImgThunkData pIATEntry)
{
	DelayLoadInfo dli;
	int index;
	PImgThunkData pIAT;
	PImgThunkData pINT;
	HMODULE *phMod;
	FARPROC pProc;

	pIAT = PFromRva(pidd->rvaIAT);
	pINT = PFromRva(pidd->rvaINT);
	phMod = PFromRva(pidd->rvaHmod);
	index = IndexFromPImgThunkData(pIATEntry, pIAT);

	dli.cb = sizeof(dli);
	dli.pidd = pidd;
	dli.ppfn = (FARPROC*)pIATEntry->u1.Function;
	dli.szDll = PFromRva(pidd->rvaDLLName);
	dli.dlp.fImportByName = !(pINT[index].u1.Ordinal & IMAGE_ORDINAL_FLAG);
	if (dli.dlp.fImportByName)
	{
		/* u1.AdressOfData points to a IMAGE_IMPORT_BY_NAME struct */
		PIMAGE_IMPORT_BY_NAME piibn = PFromRva((RVA)pINT[index].u1.AddressOfData);
		dli.dlp.szProcName = (LPCSTR)&piibn->Name;
	}
	else
	{
		dli.dlp.dwOrdinal = pINT[index].u1.Ordinal & ~IMAGE_ORDINAL_FLAG;
	}
	dli.hmodCur = *phMod;
	dli.pfnCur = (FARPROC)pIAT[index].u1.Function;
	dli.dwLastError = GetLastError();
	pProc = __pfnDliNotifyHook2(dliStartProcessing, &dli);
	if (pProc)
	{
		pIAT[index].u1.Function = (DWORD_PTR)pProc;
		return pProc;
	}

	if (dli.hmodCur == NULL)
	{
		dli.hmodCur = LoadLibraryA(dli.szDll);
		if (!dli.hmodCur)
		{
			dli.dwLastError = GetLastError();
			__pfnDliFailureHook2(dliFailLoadLib, &dli);
//			if (ret)
//			{
//			}
			// FIXME: raise exception;
			return NULL;
		}
		*phMod = dli.hmodCur;
	}

	/* dli.dlp.szProcName might also contain the ordinal */
	pProc = GetProcAddress(dli.hmodCur, dli.dlp.szProcName);
	if (!pProc)
	{
		dli.dwLastError = GetLastError();
		__pfnDliFailureHook2(dliFailGetProc, &dli);
		// FIXME: handle return value & raise exception
		return NULL;
	}
	pIAT[index].u1.Function = (DWORD_PTR)pProc;

	return pProc;
}

/*** The default hooks ***/

FARPROC WINAPI
DefaultDliNotifyHook2(unsigned dliNotify, PDelayLoadInfo pdli)
{
	return NULL;
}

FARPROC WINAPI
DefaultDliFailureHook2(unsigned dliNotify, PDelayLoadInfo pdli)
{
	return NULL;
}

PfnDliHook __pfnDliNotifyHook2 = DefaultDliNotifyHook2;
PfnDliHook __pfnDliFailureHook2 = DefaultDliFailureHook2;
