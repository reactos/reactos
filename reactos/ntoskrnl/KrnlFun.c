///////////////////////////////////////////////////////////////////////////////
//
//            Alex's Big Ol' List of FIXMEs, bugs and regressions
//              If you see something here, Alex *KNOWS ABOUT IT*.
//                         Do NOT bug him about it.
//                      Do NOT ask if he knows about it.
//                        Do NOT complain about it.
//                     Do NOT ask when it will be fixed.
//              Failure to respect this will *ACHIEVE NOTHING*.
//
// Io:
//  - Add support for Fast Dispatch I/O.
//  - Verify ShareAccess APIs, XP added some new semantics.
//  - Add Access Checks in IopParseDevice.
//  - Add validation checks in IoCreateFile.
//  - Fix double-reference in IopCreateFile.
//  - Add tracing to file.c
//  - See why queueing IRPs and cancelling them causes crashes
//  - Add SEH to some places where it's missing (MDLs, etc) (iofunc).
//  - Add a generic Cleanup/Exception Routine (iofunc).
//  - Add probe/alignment checks for Query/Set routines.
//  - Add another parameter to IopCleanupFailedIrp.
//  - Add support for Fast Dispatch I/O.
//  - Add support for some fast-paths when querying/setting data.
//  - Add tracing to iofunc.c
//
// Ps:
//  - Use Process Rundown.
//  - Use Process Pushlock Locks.
//  - Use Safe Referencing in PsGetNextProcess/Thread.
//  - Use Guarded Mutex instead of Fast Mutex for Active Process Locks.
//  - Use Security Locks in security.c
//  - Fix referencing problem.
//  - Generate process cookie for user-more thread.
//  - Add security calls where necessary.
//  - Add tracing.
//  - Fix crash on shutdown due to possibly incorrect win32k uninitailization.
//  - Add failure/race checks for thread creation.
//
// Ob:
//  - Possible bug in deferred deletion under Cc Rewrite branch.
//  - Add Directory Lock.
//  - Use Object Type Mutex/Lock.
//  - Implement handle database if anyone needs it.
//  - Figure out why cmd.exe won't close anymore.
//
// Ex:
//  - Use pushlocks for handle implementation.
//  - Figure out why cmd.exe won't close anymore.
//
// Ke:
//  - Add code for interval recalulation when wait interrupted by an APC
//
///////////////////////////////////////////////////////////////////////////////