/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOLE2.H
 *  WOW32 16-bit OLE2 special stuff
 *
 *  History:
 *  Created 04-May-1994 by Bob Day (bobday)
--*/

#define ISTASKALIAS(htask16)    (((htask16) & 0x4) == 0 && (htask16 <= 0xffe0) && (htask16))

/* Function prototypes
 */
HTASK16 AddHtaskAlias( DWORD ThreadID32 );
HTASK16 FindHtaskAlias( DWORD ThreadID32 );
void RemoveHtaskAlias( HTASK16 htask16 );
DWORD GetHtaskAlias( HTASK16 htask16, LPDWORD lpdwProcessID32 );
UINT GetHtaskAliasProcessName( HTASK16 htask16, LPSTR lpNameBuffer, UINT cNameBufferSize );

extern UINT cHtaskAliasCount;
