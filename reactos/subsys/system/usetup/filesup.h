/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: filesup.h,v 1.2 2002/11/23 01:55:27 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/filesup.h
 * PURPOSE:         File support functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __FILESUP_H__
#define __FILESUP_H__

NTSTATUS
CreateDirectory(PWCHAR DirectoryName);

NTSTATUS
SetupCopyFile(PWCHAR SourceFileName,
	      PWCHAR DestinationFileName);


#endif /* __FILESUP_H__ */

/* EOF */
