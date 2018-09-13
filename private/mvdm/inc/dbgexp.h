/*++ BUILD Version: 0001
 *
 *  MVDM v1.0
 *
 *  Copyright (c) 1991,1992 Microsoft Corporation
 *
 *  DBGEXP.H
 *  DBG exports
 *
 *  History:
 *  13-Jan-1992 Bob Day (bobday)
 *  Created.
--*/

extern BOOL DBGInit( int, char *[] );
extern VOID DBGDispatch( void );
extern VOID DBGNotifyNewTask( LPVOID pNTFrame, UINT uFrameSize );
extern VOID DBGNotifyRemoteThreadAddress( LPVOID lpAddress, DWORD lpBlock );
extern VOID DBGNotifyDebugged( BOOL fDebugged );
