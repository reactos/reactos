/*
 *  ReactOS
 *
 *  regdump.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#ifndef __REGDUMP_H__
#define __REGDUMP_H__

#ifdef __cplusplus
extern "C" {
#endif


extern HANDLE OutputHandle;
extern HANDLE InputHandle;

DWORD GetInput(char* Buffer, int buflen);
//void dprintf(char* fmt, ...);
int regdump(int argc, char* argv[]);
BOOL ProcessCmdLine(LPSTR lpCmdLine);


#ifdef __cplusplus
};
#endif

#endif // __REGDUMP_H__
