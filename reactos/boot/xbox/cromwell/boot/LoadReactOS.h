#ifndef _LOADREACTOS_H_
#define _LOADREACTOS_H_

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

bool ReactOSPresentOnCD(int drive);
void BootReactOSFromCD(int drive);

bool ReactOSPresentOnFATX(FATXPartition *partition);
void BootReactOSFromFATX(void);

bool ReactOSPresentOnNative(int partition);
void BootReactOSFromNative(int partition);

bool ReactOSPresentOnEtherboot(void);
void BootReactOSFromEtherboot(void);

#endif
