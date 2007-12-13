#ifndef _LOADLINUX_H_
#define _LOADLINUX_H_

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

bool LinuxPresentOnCD(int drive);
void BootLinuxFromCD(int drive);

bool LinuxPresentOnFATX(FATXPartition *partition);
void BootLinuxFromFATX(void);

bool LinuxPresentOnNative(int partition);
void BootLinuxFromNative(int partition);

bool LinuxPresentOnEtherboot(void);
void BootLinuxFromEtherboot(void);

#endif
