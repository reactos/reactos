/*
 *  ReactOS Task Manager
 *
 *  optnmenu.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
//
// Menu item handlers for the options menu.
//

#ifndef __OPTNMENU_H
#define __OPTNMENU_H

void TaskManager_OnOptionsAlwaysOnTop(void);
void TaskManager_OnOptionsMinimizeOnUse(void);
void TaskManager_OnOptionsHideWhenMinimized(void);
void TaskManager_OnOptionsShow16BitTasks(void);

#endif // __OPTNMENU_H
