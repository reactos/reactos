#include "stdafx.h"
#pragma hdrstop

/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Apr 03 11:59:43 1997
 */
/* Compiler settings for ADMover.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IDeskMovr = {0x72267F69,0xA6F9,0x11D0,{0xBC,0x94,0x00,0xC0,0x4F,0xB6,0x78,0x63}};


const IID IID_IDeskSizr = {0x72267F6C,0xA6F9,0x11D0,{0xBC,0x94,0x00,0xC0,0x4F,0xB6,0x78,0x63}};


const IID LIBID_ADMOVERLib = {0x72267F5C,0xA6F9,0x11D0,{0xBC,0x94,0x00,0xC0,0x4F,0xB6,0x78,0x63}};


const CLSID CLSID_DeskMovr = {0x72267F6A,0xA6F9,0x11D0,{0xBC,0x94,0x00,0xC0,0x4F,0xB6,0x78,0x63}};


const CLSID CLSID_DeskSizr = {0x72267F6D,0xA6F9,0x11D0,{0xBC,0x94,0x00,0xC0,0x4F,0xB6,0x78,0x63}};


#ifdef __cplusplus
}
#endif

