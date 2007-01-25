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
// Global:
//  - TODO: Complete the list of bufxies
//  - Fix process reference count leak.
//  - Fix atapi.sys loading one more time at each boot.
//  - Fix LiveCD.
//
///////////////////////////////////////////////////////////////////////////////

// REACTOS GUIDANCE PLAN
//  ________________________________________________________________________________________________________
// /                                                                                                        \
// | OB, PS, LPC, DBGK, EX, INIT => "Code complete". No expected changes until 0.5.0                      | |
// | SE => Not looked at. Interaction with Ps/Io is minimal and currently hacked away. Preserve.          |J|
// | KD/KDBG => Laptop has special version of ROS without these components. Commit in branch.             |A|
// | HAL => Needs APC/DPC/IRQL implementation fixed ASAP in terms of interaction with Ke.                 |N|
// | ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||      ||           | |
// | \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/      \/           |F|
// | BUGFIXES BUGFIXES BUGFIXES BUGFIXES BUGFIXES BUGFIXES BUGFIXES BUGFIXES BUGFIXES BUGFIXES BUGFIXES   |E|
// | KE => Enable new thread scheduler and ensure it works.                                               |B|
// | KD => Implement KD64 6.0, compatible with WinDBG                                                     | |
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
