//*******************************************************************************************
//
// Filename : Cabvw2.h
//	
//			Cab_MergeMenu
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************



#ifndef _ISHELL2_H_
#define _ISHELL2_H_

#ifdef __cplusplus
extern "C" {
#endif
//===========================================================================
// ITEMIDLIST
//===========================================================================

// unsafe macros 
#define _ILSkip(pidl, cb)       ((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)           _ILSkip(pidl, (pidl)->mkid.cb)
#ifdef DEBUG
// Dugging aids for making sure we dont use free pidls
#define VALIDATE_PIDL(pidl) Assert((pidl)->mkid.cb != 0xC5C5)
#else
#define VALIDATE_PIDL(pidl)
#endif

void        ILFree(LPITEMIDLIST pidl);
LPITEMIDLIST ILClone(LPCITEMIDLIST pidl);
UINT ILGetSize(LPCITEMIDLIST pidl);

typedef void (WINAPI FAR* RUNDLLPROC)(HWND hwndStub,                      
        HINSTANCE hAppInstance,                                           
        LPSTR lpszCmdLine, int nCmdShow);                                 

UINT  Cab_MergeMenus(HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags);

//===================================================================
// Cab_MergeMenu parameter
//
#define MM_ADDSEPARATOR		0x00000001L
#define MM_SUBMENUSHAVEIDS	0x00000002L

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))


#ifdef __cplusplus
}
#endif
                                           

#endif // _ISHELL2_H_


