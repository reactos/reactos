/*++ BUILD Version: 0001    // Increment this if a change has global effects

    FILE:           BREAKPTS.H

    PURPOSE:        Defines and prototypes for the QCQP persistent
                    breakpoint handler.

**********************************************************************/

/*
**  QCWin breakpoint support functions
*/

void      PASCAL BPTResolveAll(HPID,BOOL);
DWORD     PASCAL BPTIsUnresolvedCount(HPID hpid);
void      PASCAL BPTUnResolve(HEXE);
void      PASCAL BPTUnResolveAll(HPID);
int       PASCAL BPTResolve( LPSTR, PVOID, PCXF, BOOL);
void      PASCAL BPTUnResolvePidTid( HPID, HTID);
BOOL      PASCAL BPTCanIUseThunk( LPSTR );


//
//  Workspace support.
//
BOOL ClearWndProcHistory ( void );
BOOL SetWndProcHistory ( LPSTR, DWORD );
LPSTR   GetWndProcHistory ( DWORD* );



#define BP_LINELEADER '@'

