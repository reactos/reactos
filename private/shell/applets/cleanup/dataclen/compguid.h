/*
**------------------------------------------------------------------------------
** Module:  NTFS Compression Disk Cleanup Cleaner
** File:    compguid.h
**
** Purpose: Defines A 'NTFS Compression Cleaner' Class ID for OLE 2.0
** Notes:   The unique Class ID of this Compression Cleaner class is:
**
**          {B50F5260-0C21-11D2-AB56-00A0C9082678}
**
** Mod Log: Created by Jason Cobb (2/97)
**          Adapted for Compression Cleaner by DSchott (6/98)
**
** Copyright (c)1997-1998 Microsoft Corporation. All Rights Reserved.
**------------------------------------------------------------------------------
*/
#ifndef COMPGUID_H
#define COMPGUID_H


/*
**------------------------------------------------------------------------------
** Microsoft C++ include files 
**------------------------------------------------------------------------------
*/
#include <objbase.h>
#include <initguid.h>


/*
**------------------------------------------------------------------------------
** Class ID
**------------------------------------------------------------------------------
*/

// {B50F5260-0C21-11D2-AB56-00A0C9082678}
DEFINE_GUID(CLSID_CompCleaner,
0xB50F5260L, 0x0C21, 0x11D2, 0xAB, 0x56, 0x00, 0xA0, 0xC9, 0x08, 0x26, 0x78);

#define REG_COMPCLEANER_GUID             TEXT("{B50F5260-0C21-11D2-AB56-00A0C9082678}")
#define REG_COMPCLEANER_CLSID            TEXT("CLSID\\{B50F5260-0C21-11D2-AB56-00A0C9082678}")
#define REG_COMPCLEANER_INPROCSERVER32   TEXT("CLSID\\{B50F5260-0C21-11D2-AB56-00A0C9082678}\\InProcServer32")
#define REG_COMPCLEANER_DEFAULTICON      TEXT("CLSID\\{B50F5260-0C21-11D2-AB56-00A0C9082678}\\DefaultIcon")
#define ID_COMPCLEANER                   2

#endif // COMPGUID_H
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
