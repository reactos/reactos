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
//
// Ob:
//  - Fix bug related to Deferred Loading (don't requeue active work item).
//  - Add Directory Lock.
//
// Fstub:
//  - Implement IoAssignDriveLetters using mount manager support.
//
// Ke:
//  - Figure out why the DPC stack doesn't really work.
//  - Fix SEH/Page Fault + Exceptions!? Weird exception bugs!
//  - New optimized table-based tick-hashed timer implementation.
//  - New Thread Scheduler based on 2003.
//  - Implement KiCallbackReturn, KiGetTickCount, KiRaiseAssertion.
//
// Hal:
//  - New IRQL Implementation.
//  - CMOS Initialization and CMOS Spinlock.
//  - Report resource usage to kernel (HalReportResourceUsage).
//
// Lpc:
//  - Activate new NTLPC and delete old implementation.
//  - Figure out why LPC-processes won't die anymore.
//
// Ex:
//  - Implement Generic Callback mechanism.
//  - Use pushlocks for handle implementation.
//
// Kd:
//  - Implement KD Kernel Debugging and WinDBG support.
//
// Native:
//  - Rewrite loader.
//  - Make smss NT-compatible.
//
///////////////////////////////////////////////////////////////////////////////

