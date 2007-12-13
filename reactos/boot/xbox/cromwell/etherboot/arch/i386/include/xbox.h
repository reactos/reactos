#ifndef xbox_h
#define xbox_h

#include "elf.h"
#include "elf_boot.h"

#define	ELF_MAGIC	0x464C457FL	/* e_ident[0:3], little-endian */

/* Most of this things are copied from etherboot.h in the mknbi sources */

struct segoff
{
	unsigned short	offset, segment;
};

struct imgheader
{
	unsigned long	magic;
	unsigned long	length;
	struct segoff	location;
	struct segoff	execaddr;
};

union infoblock
{
	unsigned char		c[512];
	struct imgheader	img;
	Elf32_Ehdr			ehdr;
};

/* These are determined by mknbi.pl. If that changes, so must this. */
enum linuxsegments { S_FIRST = 0, S_PARAMS, S_BOOT, S_SETUP, S_KERNEL,
        S_RAMDISK, S_NOTE, S_END };
#endif
