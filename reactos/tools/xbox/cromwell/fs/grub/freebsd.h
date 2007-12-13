
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001  Free Software Foundation, Inc.
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

/* command-line parameter defines */
#define RB_ASKNAME      0x01	/* ask for file name to reboot from */
#define RB_SINGLE       0x02	/* reboot to single user only */
#define RB_NOSYNC       0x04	/* dont sync before reboot */
#define RB_HALT         0x08	/* don't reboot, just halt */
#define RB_INITNAME     0x10	/* name given for /etc/init (unused) */
#define RB_DFLTROOT     0x20	/* use compiled-in rootdev */
#define RB_KDB          0x40	/* give control to kernel debugger */
#define RB_RDONLY       0x80	/* mount root fs read-only */
#define RB_DUMP         0x100	/* dump kernel memory before reboot */
#define RB_MINIROOT     0x200	/* mini-root present in memory at boot time */
#define RB_CONFIG       0x400	/* invoke user configuration routing */
#define RB_VERBOSE      0x800	/* print all potentially useful info */
#define RB_SERIAL       0x1000	/* user serial port as console */
#define RB_CDROM        0x2000	/* use cdrom as root */
#define RB_GDB		0x8000	/* use GDB remote debugger instead of DDB */
#define RB_MUTE		0x10000	/* Come up with the console muted */
#define RB_MULTIPLE	0x20000000	/* Use multiple consoles */

#define RB_BOOTINFO     0x80000000	/* have `struct bootinfo *' arg */

/*
 * Constants for converting boot-style device number to type,
 * adaptor (uba, mba, etc), unit number and partition number.
 * Type (== major device number) is in the low byte
 * for backward compatibility.  Except for that of the "magic
 * number", each mask applies to the shifted value.
 * Format:
 *       (4) (4) (4) (4)  (8)     (8)
 *      --------------------------------
 *      |MA | AD| CT| UN| PART  | TYPE |
 *      --------------------------------
 */
#define B_ADAPTORSHIFT          24
#define B_CONTROLLERSHIFT       20
#define B_UNITSHIFT             16
#define B_PARTITIONSHIFT        8
#define B_TYPESHIFT             0

#define B_DEVMAGIC      ((unsigned long)0xa0000000)

#define MAKEBOOTDEV(type, adaptor, controller, unit, partition) \
        (((type) << B_TYPESHIFT) | ((adaptor) << B_ADAPTORSHIFT) | \
        ((controller) << B_CONTROLLERSHIFT) | ((unit) << B_UNITSHIFT) | \
        ((partition) << B_PARTITIONSHIFT) | B_DEVMAGIC)


/* Only change the version number if you break compatibility. */
#define BOOTINFO_VERSION        1

#define N_BIOS_GEOM             8

/*
 * A zero bootinfo field often means that there is no info available.
 * Flags are used to indicate the validity of fields where zero is a
 * normal value.
 */
struct bootinfo
  {
    unsigned int bi_version;
    unsigned char *bi_kernelname;
    struct nfs_diskless *bi_nfs_diskless;
    /* End of fields that are always present. */
#define bi_endcommon            bi_n_bios_used
    unsigned int bi_n_bios_used;
    unsigned long bi_bios_geom[N_BIOS_GEOM];
    unsigned int bi_size;
    unsigned char bi_memsizes_valid;
    unsigned char bi_bios_dev;
    unsigned char bi_pad[2];
    unsigned long bi_basemem;
    unsigned long bi_extmem;
    unsigned long bi_symtab;
    unsigned long bi_esymtab;
  };
