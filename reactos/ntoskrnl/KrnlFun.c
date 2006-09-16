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
//  - See why queueing IRPs and cancelling them causes crashes.
//  - Add Access Checks in IopParseDevice.
//  - Add validation checks in IoCreateFile.
//  - Add probe/alignment checks for Query/Set routines.
//  - Add tracing to iofunc.c
//  - Add tracing to file.c
//  - Add support for some fast-paths when querying/setting data.
//  - Verify ShareAccess APIs, XP added some new semantics.
//  - Add support for Fast Dispatch I/O.
//
// Ob:
//  - Fix bug related to Deferred Loading (don't requeue active work item).
//  - Add Directory Lock.
//  - Use Object Type Mutex/Lock.
//
// Ke:
//
//  - FIXES:
//      * Implement invalid opcode handler (fixes some apps from crashing).
//      * Get rid of KiRosPrintAddress and use KiDumpParameterImages instead.
//      * Sanitize some context fields during conversions.
//      * Add DR macro/save and VM macro/save.
//  - FEATURES:
//      * New optimized table-based tick-hashed timer implementation.
//      * New Thread Scheduler based on 2003.
//      * Proper Idle/Initial Thread setup and 2nd stage boot.
//      * Implement KiCallbackReturn, KiGetTickCount, KiRaiseAssertion.
//
// Ex:
//  - Use pushlocks for handle implementation.
//
///////////////////////////////////////////////////////////////////////////////

