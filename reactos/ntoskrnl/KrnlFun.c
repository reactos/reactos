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
//  - Add tracing to iofunc.c, file.c and device.c
//  - Add Access Checks in IopParseDevice.
//  - Add validation checks in IoCreateFile.
//  - Add probe/alignment checks for Query/Set routines.
//  - Verify ShareAccess APIs, XP added some new semantics.
//  - Add support for some fast-paths when querying/setting data.
//  - Add support for Fast Dispatch I/O.
//
// Ob:
//  - Fix bug related to Deferred Loading (don't requeue active work item).
//  - Add Directory Lock.
//  - Use Object Type Mutex/Lock.
//
// Ex:
//  - Use pushlocks for handle implementation.
//
///////////////////////////////////////////////////////////////////////////////

