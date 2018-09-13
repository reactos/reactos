/*****************************************************************/
/**               Microsoft Windows for Workgroups              **/
/**           Copyright (C) Microsoft Corp., 1991-1992          **/
/*****************************************************************/

/* NPORD.H -- Network service provider ordinal definitions.
 *
 * This is a PRIVATE header file.  Nobody but the MNR needs to call
 * a network provider directly.
 *
 * History:
 *  03/29/93    gregj   Created
 *  05/27/97    gregj   Taken from WNET source to implement NP delay load stub
 *
 */

#ifndef _INC_NPORD
#define _INC_NPORD

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#define ORD_GETCONNECTIONS      12
#define ORD_GETCAPS             13
//#define ORD_DEVICEMODE          14      /* no longer supported */
#define ORD_GETUSER             16
#define ORD_ADDCONNECTION       17
#define ORD_CANCELCONNECTION    18
//#define ORD_PROPERTYDIALOG      29      /* no longer supported */
//#define ORD_GETDIRECTORYTYPE    30      /* no longer supported */      
//#define ORD_DIRECTORYNOTIFY     31      /* no longer supported */     
//#define ORD_GETPROPERTYTEXT     32      /* no longer supported */
#define ORD_OPENENUM            33
#define ORD_ENUMRESOURCE        34
#define ORD_CLOSEENUM           35
#define ORD_GETUNIVERSALNAME    36
//#define ORD_SEARCHDIALOG        38      /* no longer supported */
#define ORD_GETRESOURCEPARENT   41
#define ORD_VALIDDEVICE         42
#define ORD_LOGON               43
#define ORD_LOGOFF              44
#define ORD_GETHOMEDIRECTORY    45
#define ORD_FORMATNETWORKNAME   46
#define ORD_GETCONNPERFORMANCE  49
#define ORD_GETPOLICYPATH    	50
#define ORD_GETRESOURCEINFORMATION   52

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif  /* !_INC_NPORD */
