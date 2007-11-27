This library deals with Recycle bin.
It is aimed to be compatible with Windows 2000/XP/2003 (at least) on FAT or NTFS volumes.

TODO
- Empty a recycle bin containing directories (v5)
- Set security on recycle bin folder
- Make the library thread-safe

3 levels
- 1: recyclebin.c         : Public C interface
- 2: recyclebin_generic.c : 'System-wide' recycle bin, which knows no implementation detail
- 3: recyclebin_v5.c      : Deals with recycle bins of Windows 2000/XP/2003