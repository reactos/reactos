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
// Ke1:
//  - Implement Privileged Instruction Handler in Umode GPF.
//
// Ex:
//  - Use pushlocks for handle implementation.
//
// Ke2:
//  - Dispatcher Rewrite (DPCs-Timers-Waits).
//
// Hal:
//  - Use APC and DPC Interrupt Dispatchers.
//  - CMOS Initialization and CMOS Spinlock.
//
// Fstub:
//  - Implement IoAssignDriveLetters using mount manager support.
//
// Kd:
//  - Implement KD Kernel Debugging and WinDBG support.
//
///////////////////////////////////////////////////////////////////////////////

// REACTOS GUIDANCE PLAN
//  ________________________________________________________________________________________________________
// /                                                                                                        \
// | OB, PS, LPC, DBGK, IO => Almost entirely fixed interaction with Ke/Ex.                               | |
// | SE => Not looked at. Interaction with Ps/Io is minimal and currently hacked away. Preserve.          |J|
// | EX => Needs re-visiting (in trunk). Do callbacks/push locks for interaction with Ps.                 |A|
// | KD/KDBG => Laptop has special version of ROS without these components. Commit in branch.             |N|
// | INIT => Boot sequence still needs work in terms of interaction with Ke and CPU features.             | |
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |F|
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |E|
// | HAL => Needs APC/DPC/IRQL implementation fixed ASAP in terms of interaction with Ke.                 |B|
// | FSTUB => Needs IoAssignDriveLetters fixed ASAP but not critical to Ke/Ex. Interacts with Io.         | |
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |M|
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |A|
// | CM => TOTAL REWRITE.                                                                                 |R|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           | |
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |A|
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |P|
// | KE => Timer Rewrite + Thread Scheduler Rewrite.                                                      |R|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |I|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |L|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           | |
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |M|
// | MM => TOTAL REWRITE.                                                                                 |A|
// |                                                                                                      |Y|
// \________________________________________________________________________________________________________/
//
