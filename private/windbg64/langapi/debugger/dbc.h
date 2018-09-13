//--------------------------------------------------------------------
// DBC.H
//
// This contains a list of all the debugger callback notifications.
//
// There are multiple users of this file.  To use it, you must
// define a DECL_DBC macro to extract the pieces of information that
// you are interested in from this file.  For example, if you want
// the numerical value, name (as a string), and fRequest flag for
// each callback, you could write the following:
//
//  typedef struct {
//      DBC     dbc;
//      LPCSTR  lszDbc;
//      BOOL    fRequest;
//  } DBCINFO;
//
//  #define DECL_DBC(name, fRequest, dbct) { dbc##name, "dbc" #name, fRequest },
//
//  DBCINFO rgdbcinfo[] = {
//      #include "dbc.h"
//  };
//
//  #undef DECL_DBC
//--------------------------------------------------------------------

DECL_DBC(Nil,             FALSE, dbctStop)

DECL_DBC(Bpt,             FALSE, dbctStop)
DECL_DBC(CheckBpt,        TRUE,  dbctContinue)
DECL_DBC(SendBpt,         FALSE, dbctContinue)

DECL_DBC(WatchPoint,      FALSE, dbctStop)
DECL_DBC(CheckWatchPoint, TRUE,  dbctContinue)
DECL_DBC(SendWatchPoint,  FALSE, dbctContinue)

DECL_DBC(MsgBpt,          FALSE, dbctStop)
DECL_DBC(CheckMsgBpt,     TRUE,  dbctContinue)
DECL_DBC(SendMsgBpt,      FALSE, dbctContinue)

DECL_DBC(AsyncStop,       FALSE, dbctStop)     // Async stop has completed
DECL_DBC(EntryPoint,      FALSE, dbctStop)
DECL_DBC(LoadComplete,    FALSE, dbctStop)
DECL_DBC(Signal,          FALSE, dbctStop)
DECL_DBC(Exception,       FALSE, dbctMaybeContinue)

DECL_DBC(ExecuteDone,     FALSE, dbctStop)

DECL_DBC(Step,            FALSE, dbctStop)

DECL_DBC(CanStep,         TRUE,  dbctContinue)

DECL_DBC(NewProc,         FALSE, dbctContinue)
DECL_DBC(ProcTerm,        FALSE, dbctStop)
DECL_DBC(DeleteProc,      FALSE, dbctContinue)

DECL_DBC(CreateThread,    TRUE,  dbctStop)
DECL_DBC(ThreadTerm,      FALSE, dbctStop)
DECL_DBC(DeleteThread,    FALSE, dbctContinue)

DECL_DBC(ModLoad,         TRUE,  dbctContinue)
DECL_DBC(ModFree,         FALSE, dbctContinue)
DECL_DBC(SegLoad,         FALSE, dbctContinue)

DECL_DBC(InfoAvail,       FALSE, dbctContinue) // i.e. OutputDebugString
DECL_DBC(InfoReq,         TRUE,  dbctContinue) // i.e. InputDebugString
DECL_DBC(Error,           FALSE, dbctStop)     // misc error reporting
DECL_DBC(ServiceDone,     FALSE, dbctStop)     // SystemService reporting completion

DECL_DBC(LastAddr,        TRUE,  dbctContinue) // get last address in source line

DECL_DBC(EmChange,        FALSE, dbctContinue)

DECL_DBC(CodeChanged,     FALSE, dbctContinue)
DECL_DBC(MemoryChanged,   FALSE, dbctContinue)

DECL_DBC(ThreadBlocked,   FALSE, dbctStop)

DECL_DBC(FlipScreen,      FALSE, dbctContinue)
DECL_DBC(HardMode,        FALSE, dbctContinue)
DECL_DBC(SoftMode,        FALSE, dbctContinue)

DECL_DBC(RemoteQuit,      FALSE, dbctStop)     // Might these be the same thing?
DECL_DBC(CommError,       FALSE, dbctStop)
DECL_DBC(ExitedFunction,  TRUE,  dbctContinue)
                        // We just exited a function (either stepped
                        // a RET, or stepped over a CALL)
                        //   wParam = nothing
                        //   lParam = LPADDR, points to some address
                        //       in the function we just exited; NOT
                        //       necessarily the very beginning of the func

DECL_DBC(SQLThread,       FALSE, dbctStop)

DECL_DBC(GetExpression,   FALSE, dbctContinue)

DECL_DBC(Max,             FALSE, dbctStop)
