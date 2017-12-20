/*
    vfdver.h

    Virtual Floppy Drive for Windows
    common version definition

    Copyright (c) 2003-2008 Ken Kato
*/

#ifndef _VFDVER_H_
#define _VFDVER_H_

//  product version information
#define VFD_PRODUCT_NAME            "Virtual Floppy Drive for Windows"
#define VFD_PRODUCT_MAJOR           2
#define VFD_PRODUCT_MINOR           1

//  driver file version information
#define VFD_DRIVER_FILENAME         "vfd.sys"
#define VFD_DRIVER_MAJOR            2
#define VFD_DRIVER_MINOR            1

//  build year and month/date
#define VFD_BUILD_YEAR              2008
#define VFD_BUILD_MDAY              0206

//  copyright information
#define VFD_COMPANY_NAME            "Ken Kato"
#define VFD_COPYRIGHT_YEARS         "2003-2008"

//  version information language and code page
//  LANG_ENGLISH/SUBLANG_ENGLISH_US, Unicode CP
#define VFD_VERSIONINFO_LANG        "040904B0"
#define VFD_VERSIONINFO_TRANS       0x0409, 0x04B0

#if ((DBG) || defined(_DEBUG))
#define VFD_DEBUG_FLAG              0x80000000
#define VFD_DEBUG_TAG               " (debug)"
#else
#define VFD_DEBUG_FLAG              0
#define VFD_DEBUG_TAG
#endif

//
//  Version manipulation macros
//
#define VFD_PRODUCT_VERSION_VAL     \
    ((ULONG)((USHORT)VFD_PRODUCT_MAJOR<<16)|((USHORT)VFD_PRODUCT_MINOR))

#define VFD_DRIVER_VERSION_VAL      \
    ((ULONG)((USHORT)VFD_DRIVER_MAJOR<<16)|((USHORT)VFD_DRIVER_MINOR))

#define VFD_FILE_VERSION_VAL        \
    ((ULONG)((USHORT)VFD_FILE_MAJOR<<16)|((USHORT)VFD_FILE_MINOR))

#define VFD_VERSION_STR2(a,b)       #a "." #b
#define VFD_VERSION_STR(a,b)        VFD_VERSION_STR2(a,b)
#define VFD_PRODUCT_VERSION_STR     VFD_VERSION_STR(VFD_PRODUCT_MAJOR,VFD_PRODUCT_MINOR)
#define VFD_DRIVER_VERSION_STR      VFD_VERSION_STR(VFD_DRIVER_MAJOR,VFD_DRIVER_MINOR)
#define VFD_FILE_VERSION_STR        VFD_VERSION_STR(VFD_FILE_MAJOR,VFD_FILE_MINOR)
#define VFD_BUILD_DATE_STR          VFD_VERSION_STR(VFD_BUILD_YEAR,VFD_BUILD_MDAY)

//
//  Product description
//
#define VFD_PRODUCT_DESC            \
    VFD_PRODUCT_NAME " " VFD_PRODUCT_VERSION_STR "." VFD_BUILD_DATE_STR VFD_DEBUG_TAG

#define VFD_COPYRIGHT_STR           \
    "Copyright (c) " VFD_COPYRIGHT_YEARS " " VFD_COMPANY_NAME

#endif  //  _VFDVER_H_
