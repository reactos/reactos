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

#ifndef __REACTOS_H
#define __REACTOS_H


///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Loading Functions
//
///////////////////////////////////////////////////////////////////////////////////////
void LoadAndBootReactOS(PUCHAR OperatingSystemName);

///////////////////////////////////////////////////////////////////////////////////////
//
// ReactOS Setup Loader Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID	ReactOSRunSetupLoader(VOID);

///////////////////////////////////////////////////////////////////////////////////////
//
// ARC Path Functions
//
///////////////////////////////////////////////////////////////////////////////////////
BOOL	DissectArcPath(char *ArcPath, char *BootPath, U32* BootDrive, U32* BootPartition);
//BOOL	ConvertBiosDriveToArcName(PUCHAR ArcName, U32 BiosDriveNumber);
//U32		ConvertArcNameToBiosDrive(PUCHAR ArcName);


#endif // defined __REACTOS_H
