/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Aug 06 14:16:25 1998
 */
/* Compiler settings for d:\NDOC\PRIVATE\NETDOCS\nquill\qs\ITreeSync.idl:
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

const IID IID_ITreeSyncBehavior = {0x8860B602,0x178E,0x11d2,{0x96,0xAE,0x00,0x80,0x5F,0x85,0x2B,0x4D}};


const IID IID_ITreeSyncServices = {0x8860B601,0x178E,0x11d2,{0x96,0xAE,0x00,0x80,0x5F,0x85,0x2B,0x4D}};


#ifdef __cplusplus
}
#endif

