/* boot.h  -  Read and analyze ia PC/MS-DOS boot sector */

/* Written 1993 by Werner Almesberger */


#ifndef _BOOT_H
#define _BOOT_H

void read_boot(DOS_FS *fs);

/* Reads the boot sector from the currently open device and initializes *FS */

#endif
