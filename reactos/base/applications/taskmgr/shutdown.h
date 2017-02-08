/*
 *  ReactOS Task Manager
 *
 *  shutdown.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *                2011         Mário Kacmár /Mario Kacmar/ aka Kario (kario@szm.sk)
 *                2014         Robert Naumann  <gonzomdx@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
