/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.03.0110 */
/* at Mon Sep 08 11:19:17 1997
 */
/* Compiler settings for prevband.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
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

const IID IID_IPreviewBand = {0xA582D7F3,0x0386,0x11D1,{0xB2,0xF5,0x00,0xAA,0x00,0x37,0x50,0x31}};


const IID LIBID_PREVBANDLib = {0xA582D7E6,0x0386,0x11D1,{0xB2,0xF5,0x00,0xAA,0x00,0x37,0x50,0x31}};


const CLSID CLSID_PreviewBand = {0xA582D7F4,0x0386,0x11D1,{0xB2,0xF5,0x00,0xAA,0x00,0x37,0x50,0x31}};


#ifdef __cplusplus
}
#endif

