/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     System Shutdown.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2011 Mário Kacmár /Mario Kacmar/ aka Kario <kario@szm.sk>
 *              Copyright 2014 Robert Naumann <gonzomdx@gmail.com>
 */

#pragma once

VOID
ShutDown_StandBy(VOID);

VOID
ShutDown_Hibernate(VOID);

VOID
ShutDown_PowerOff(VOID);

VOID
ShutDown_Reboot(VOID);

VOID
ShutDown_LogOffUser(VOID);

VOID
ShutDown_SwitchUser(VOID);

VOID
ShutDown_LockComputer(VOID);

VOID
ShutDown_Disconnect(VOID);

VOID
ShutDown_EjectComputer(VOID);
