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
// | OB, PS, LPC, DBGK, EX => "Code complete". No expected changes until 0.5.0                            | |
// | SE => Not looked at. Interaction with Ps/Io is minimal and currently hacked away. Preserve.          |J|
// | INIT => Boot sequence still needs work in terms of interaction with Ke and CPU features.             |A|
// | KD/KDBG => Laptop has special version of ROS without these components. Commit in branch.             |N|
// | HAL => Needs APC/DPC/IRQL implementation fixed ASAP in terms of interaction with Ke.                 | |
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           | |
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |F|
// | KE => Enable new thread scheduler and ensure it works.                                               |E|
// | KD => Implement KD64 6.0, compatible with WinDBG                                                     |B|
// | FSTUB => Needs IoAssignDriveLetters fixed ASAP but not critical to Ke/Ex. Interacts with Io.         | |
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |M|
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |A|
// | CM => TOTAL REWRITE.                                                                                 |R|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           | |
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |A|
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |P|
// | PNPMGR => TBD                                                                                        |R|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |I|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           |L|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           | |
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |M|
// | MM => TOTAL REWRITE.                                                                                 |A|
// |                                                                                                      |Y|
// \________________________________________________________________________________________________________/
//
