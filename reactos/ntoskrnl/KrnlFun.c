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
//  - Fix double-reference in IopCreateFile.
//  - Add SEH to some places where it's missing (MDLs, etc) (iofunc).
//  - Add a generic Cleanup/Exception Routine (iofunc).
//  - See why queueing IRPs and cancelling them causes crashes
//  - Add another parameter to IopCleanupFailedIrp.
//  - Add Access Checks in IopParseDevice.
//  - Add validation checks in IoCreateFile.
//  - Add probe/alignment checks for Query/Set routines.
//  - Add tracing to iofunc.c
//  - Add tracing to file.c
//  - Add support for some fast-paths when querying/setting data.
//  - Verify ShareAccess APIs, XP added some new semantics.
//  - Add support for Fast Dispatch I/O.
//
// Ps:
//  - Figure out why processes don't die.
//  - Generate process cookie for user-more thread.
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
///////////////////////////////////////////////////////////////////////////////

