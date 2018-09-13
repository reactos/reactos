#ifndef _multinet_h_
#define _multinet_h_

//////////////////////////////////////////////////////////////////////////////
//          Microsoft LAN Manager                   //
//      Copyright(c) Microsoft Corp., 1992              //
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//                                      //
// COMPONENT:   Windows Dual Network DLL/Winball DLL.               //
//                                      //
// FILE:    MULTINET.H                          //
//                                      //
// PURPOSE: General C include file to be included by modules that use   //
//      the multi-network extensions to the WINNET interface.       //
//                                      //
// REVISION HISTORY:                                //
//  lens    20-Apr-1992 First revision for Winball.             //
//                                      //
//////////////////////////////////////////////////////////////////////////////

//
// Return status codes from device/resource/file utilities.
//
#define DN_PT_UNKNOWN       0       /* Unknown or bad device or resource syntax */
#define DN_PT_PRINTER       1       /* Device is a printer */
#define DN_PT_DISK      2       /* Device is a disk drive */
#define DN_PT_UNC       3       /* Resource is a UNC name */
#define DN_PT_ALIAS     4       /* Resource is an alias name */
#define DN_PT_NETWARE       5       /* Resource follows NetWare convention */
#define DN_PT_FILELISTDEVICE    6       /* Device is first entry in file list */

//
// Masks for individual network information
//
#define MNM_NET_PRIMARY     0x0001      /* Network is primary network (Windows network) */

//
// Function prototypes for multi-net extensions
//
HANDLE FAR PASCAL __export MNetGetLastTarget ( void );
WORD   FAR PASCAL __export MNetSetNextTarget ( HANDLE hNetwork );
WORD   FAR PASCAL __export MNetNetworkEnum   ( HANDLE FAR *hNetwork );
WORD   FAR PASCAL __export MNetGetNetInfo    ( HANDLE hNetwork, LPWORD lpwNetInfo, LPSTR lpszButton, LPINT lpcbButton, LPHANDLE lphInstance );

#endif /* _multinet_h_ */
