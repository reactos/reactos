//*******************************************************************************************
//
// Filename : CabObj.h
//	
// Shell interfaces IPersistFolder, IShellBrowser, IShellView ICommdlgBrowser
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


#ifndef _CABOBJ_H_
#define _CABOBJ_H_
#include "shlobj.h"
void WINAPI SHFree(LPVOID pv);                        

DEFINE_SHLGUID(IID_IPersistFolder,   	0x000214EAL, 0, 0); 
DEFINE_SHLGUID(IID_IShellBrowser,   	0x000214E2L, 0, 0); 
DEFINE_SHLGUID(IID_IShellView,      	0x000214E3L, 0, 0); 


#define _IOffset(class, itf)         ((UINT)&(((class *)0)->itf))                     
#define IToClass(class, itf, pitf)   ((class  *)(((LPSTR)pitf)-_IOffset(class, itf))) 

                                                                                  
             

#define STRRET_OLESTR   0x0000                  



#endif // _CABOBJ_H_
