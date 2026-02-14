/*
 * PROJECT:     FreeLoader
 * LICENSE:     Dual-licensed:
 *              LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 *              MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     sys/stat.h S_IF* flags used for stat/inode.st_mode, and S_IS*
 *              convenience macros, used by *nix-based file-systems (EXTn, Btrfs, ...)
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * REFERENCE:   Based on definitions from MinGW-w64, (u)CRT, schily/stat.h, etc.
 */

#pragma once


/*
 * Device/File types (st_mode)
 */
#define _S_IFMT     0xF000 // File type mask
#define _S_IFIFO    0x1000 // FIFO buffer (Named pipe)
#define _S_IFCHR    0x2000 // Character special
// #define _S_IFMPC    0x3000 // Multiplexed Character special
#define _S_IFDIR    0x4000 // Directory
// #define _S_IFNAM    0x5000 // XENIX Named special
#define _S_IFBLK    0x6000 // Block special
// #define _S_IFMPB    0x7000 // Multiplexed Block special
#define _S_IFREG    0x8000 // Regular file

#define _S_IFCTG    0x9000 // Contiguous file
#define _S_IFCMP    0x9000 // Compressed VxFS
#define _S_IFNWK    0x9000 // Network Special (HP-UX)

#define _S_IFLNK    0xA000 // Symbolic link
#define _S_IFSHAD   0xB000 // Solaris shadow inode
#define _S_IFSOCK   0xC000 // UNIX domain socket
#define _S_IFDOOR   0xD000 // Solaris DOOR
#define _S_IFPORT   0xE000 // Solaris event port
#define _S_IFWHT    0xE000 // BSD whiteout
#define _S_IFEVC    0xF000 // UNOS eventcount

#define _S_ISTYPE(mode, mask)  (((mode) & _S_IFMT) == (mask))

#define _S_ISFIFO(m)    _S_ISTYPE(m, _S_IFIFO) // (((m) & _S_IFMT) == _S_IFIFO)
#define _S_ISCHR(m)     _S_ISTYPE(m, _S_IFCHR) // (((m) & _S_IFMT) == _S_IFCHR)
// #define _S_ISMPC(m)     _S_ISTYPE(m, _S_IFMPC) // (((m) & _S_IFMT) == _S_IFMPC)
#define _S_ISDIR(m)     _S_ISTYPE(m, _S_IFDIR) // (((m) & _S_IFMT) == _S_IFDIR)
// #define _S_ISNAM(m)     _S_ISTYPE(m, _S_IFNAM) // (((m) & _S_IFMT) == _S_IFNAM)
#define _S_ISBLK(m)     _S_ISTYPE(m, _S_IFBLK) // (((m) & _S_IFMT) == _S_IFBLK)
// #define _S_ISMPB(m)     _S_ISTYPE(m, _S_IFMPB) // (((m) & _S_IFMT) == _S_IFMPB)
#define _S_ISREG(m)     _S_ISTYPE(m, _S_IFREG) // (((m) & _S_IFMT) == _S_IFREG)

#define _S_ISCTG(m)     _S_ISTYPE(m, _S_IFCTG) // (((m) & _S_IFMT) == _S_IFCTG)
#define _S_ISCMP(m)     _S_ISTYPE(m, _S_IFCMP) // (((m) & _S_IFMT) == _S_IFCMP)
#define _S_ISNWK(m)     _S_ISTYPE(m, _S_IFNWK) // (((m) & _S_IFMT) == _S_IFNWK)

#define _S_ISLNK(m)     _S_ISTYPE(m, _S_IFLNK ) // (((m) & _S_IFMT) == _S_IFLNK )
#define _S_ISSHAD(m)    _S_ISTYPE(m, _S_IFSHAD) // (((m) & _S_IFMT) == _S_IFSHAD)
#define _S_ISSOCK(m)    _S_ISTYPE(m, _S_IFSOCK) // (((m) & _S_IFMT) == _S_IFSOCK)
#define _S_ISDOOR(m)    _S_ISTYPE(m, _S_IFDOOR) // (((m) & _S_IFMT) == _S_IFDOOR)
#define _S_ISPORT(m)    _S_ISTYPE(m, _S_IFPORT) // (((m) & _S_IFMT) == _S_IFPORT)
#define _S_ISWHT(m)     _S_ISTYPE(m, _S_IFWHT ) // (((m) & _S_IFMT) == _S_IFWHT )
#define _S_ISEVC(m)     _S_ISTYPE(m, _S_IFEVC ) // (((m) & _S_IFMT) == _S_IFEVC )

/*
 * Major/Minor of device (st_rdev)
 */
#define S_INSEM     0x01 // XENIX IFNAM semaphore
#define S_INSHD     0x02 // XENIX IFNAM shared memory object
#define S_TYPEISSEM(_stbuf) (S_ISNAM((_stbuf)->st_mode) && \
                             (_stbuf)->st_rdev == S_INSEM)
#define S_TYPEISSHM(_stbuf) (S_ISNAM((_stbuf)->st_mode) && \
                             (_stbuf)->st_rdev == S_INSHD)
// #define S_TYPEISMQ(_stbuf) (...) // Test for message queue
// #define S_TYPEISTMO(_stbuf) (...) // Test for typed memory object


/*
 * Mode permission bits (st_mode)
 *
 * S_IREAD/S_IWRITE/S_IEXEC is only available on UNIX V.7 but not on POSIX.
 * UNIX V.7 has only S_IREAD/S_IWRITE/S_IEXEC and S_ISUID/S_ISGID/S_ISVTX.
 * S_ISUID/S_ISGID/S_ISVTX is available on UNIX V.7 and POSIX.
 */
#define _S_IREAD    0x0100 // Read permission, owner
#define _S_IWRITE   0x0080 // Write permission, owner
#define _S_IEXEC    0x0040 // Execute/search permission, owner

#define _S_IRUSR    _S_IREAD        // -r-------- // Read permission, owner
#define _S_IWUSR    _S_IWRITE       // --w------- // Write permission, owner
#define _S_IXUSR    _S_IEXEC        // ---x------ // Execute/search permission, owner
#define _S_IRWXU    (_S_IRUSR | _S_IWUSR | _S_IXUSR) // Read, write, execute/search by owner

#define _S_IRGRP    (_S_IRUSR >> 3) // ----r----- // Read permission, group
#define _S_IWGRP    (_S_IWUSR >> 3) // -----w---- // Write permission, group
#define _S_IXGRP    (_S_IXUSR >> 3) // ------x--- // Execute/search permission, group
#define _S_IRWXG    (_S_IRGRP | _S_IWGRP | _S_IXGRP) // Read, write, execute/search by group

#define _S_IROTH    (_S_IRUSR >> 6) // -------r-- // Read permission, others
#define _S_IWOTH    (_S_IWUSR >> 6) // --------w- // Write permission, others
#define _S_IXOTH    (_S_IXUSR >> 6) // ---------x // Execute/search permission, others
#define _S_IRWXO    (_S_IROTH | _S_IWOTH | _S_IXOTH)   // Read, write, execute/search by others

#define _S_ISUID    0x0800 // Set-user-ID on execution
#define _S_ISGID    0x0400 // Set-group-ID on execution
#define _S_ISVTX    0x0200 // Sticky bit: on directories, restricted deletion flag


/* COMPATIBILITY DEFINITIONS *************************************************/

// #if !defined(NO_OLDNAMES) || (defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES)

/*
 * Device/File types (st_mode)
 */
#define S_IFMT      _S_IFMT
#define S_IFIFO     _S_IFIFO
#define S_IFCHR     _S_IFCHR
// #define S_IFMPC     _S_IFMPC
#define S_IFDIR     _S_IFDIR
// #define S_IFNAM     _S_IFNAM
#define S_IFBLK     _S_IFBLK
// #define S_IFMPB     _S_IFMPB
#define S_IFREG     _S_IFREG

#define S_IFCTG     _S_IFCTG
#define S_IFCMP     _S_IFCMP
#define S_IFNWK     _S_IFNWK

#define S_IFLNK     _S_IFLNK
#define S_IFSHAD    _S_IFSHAD
#define S_IFSOCK    _S_IFSOCK
#define S_IFDOOR    _S_IFDOOR
#define S_IFPORT    _S_IFPORT
#define S_IFWHT     _S_IFWHT
#define S_IFEVC     _S_IFEVC

#define S_ISTYPE(mode, mask)  _S_ISTYPE(mode, mask)

#define S_ISFIFO(m)     _S_ISFIFO(m)
#define S_ISCHR(m)      _S_ISCHR(m)
// #define S_ISMPC(m)      _S_ISMPC(m)
#define S_ISDIR(m)      _S_ISDIR(m)
// #define S_ISNAM(m)      _S_ISNAM(m)
#define S_ISBLK(m)      _S_ISBLK(m)
// #define S_ISMPB(m)      _S_ISMPB(m)
#define S_ISREG(m)      _S_ISREG(m)

#define S_ISCTG(m)      _S_ISCTG(m)
#define S_ISCMP(m)      _S_ISCMP(m)
#define S_ISNWK(m)      _S_ISNWK(m)

#define S_ISLNK(m)      _S_ISLNK(m)
#define S_ISSHAD(m)     _S_ISSHAD(m)
#define S_ISSOCK(m)     _S_ISSOCK(m)
#define S_ISDOOR(m)     _S_ISDOOR(m)
#define S_ISPORT(m)     _S_ISPORT(m)
#define S_ISWHT(m)      _S_ISWHT(m)
#define S_ISEVC(m)      _S_ISEVC(m)


/*
 * Mode permission bits (st_mode)
 */
#define S_IREAD     _S_IREAD
#define S_IWRITE    _S_IWRITE
#define S_IEXEC     _S_IEXEC

#define S_IRUSR     _S_IRUSR
#define S_IWUSR     _S_IWUSR
#define S_IXUSR     _S_IXUSR
#define S_IRWXU     _S_IRWXU

#define S_IRGRP     _S_IRGRP
#define S_IWGRP     _S_IWGRP
#define S_IXGRP     _S_IXGRP
#define S_IRWXG     _S_IRWXG

#define S_IROTH     _S_IROTH
#define S_IWOTH     _S_IWOTH
#define S_IXOTH     _S_IXOTH
#define S_IRWXO     _S_IRWXO

#define S_ISUID     _S_ISUID
#define S_ISGID     _S_ISGID
#define S_ISVTX     _S_ISVTX

// #endif // !NO_OLDNAMES || _CRT_INTERNAL_NONSTDC_NAMES
