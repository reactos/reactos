/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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

#ifndef __OPTIONS_H
#define __OPTIONS_H

void	DoOptionsMenu(void);
void	DoDiskOptionsMenu(void);
void	DoBootOptionsMenu(int BootDriveNum, char *BootDriveText);
void	DoBootPartitionOptionsMenu(int BootDriveNum);
int		RunOptionsMenu(char OptionsMenuItems[][80], int OptionsMenuItemCount, int nOptionSelected, char *OptionsMenuTitle);
void	InitOptionsMenu(int *nOptionsMenuBoxLeft, int *nOptionsMenuBoxTop, int *nOptionsMenuBoxRight, int *nOptionsMenuBoxBottom, int OptionsMenuItemCount);
void	DrawOptionsMenu(char OptionsMenuItems[][80], int OptionsMenuItemCount, int nOptionSelected, char *OptionsMenuTitle, int nOptionsMenuBoxLeft, int nOptionsMenuBoxTop, int nOptionsMenuBoxRight, int nOptionsMenuBoxBottom);

#endif // #defined __OPTIONS_H
