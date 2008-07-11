======================
 About Ext2Fsd
======================

The Ext2Fsd project is an ext2 file system driver for WinNT/2000/XP.

It's a free software and everyone can distribute and modify it under
GNU GPL.

======================
 Author & Homepage
======================

Matt<mattwu@163.com>
http://ext2.yeah.net


======================
 Credits 
======================

Here, I owe all my thanks to Bo Branten<bosse@acc.umu.se> for his
project romfs and his great contribution of the free version ntifs.h

Everyone can get to him at http://www.acc.umu.se/~bosse/.


======================
 Revision history
======================

38, V0.46:   2008-05-24

Modifications from V0.46:

    1, ext3 journal check and replay implemented. If the journal is
       not empty ext2fsd will replay the journal and make the file
       system consistent as an ext2 file system.
    2, flexible-inode-size supported. recent Linux are using 256-byte
       inode that fails 0.45 and before to show all the files.
    3, FIXME: 2 minor issues that mislead EditPlus. EditPlus is always
       trying to open any file with directory_only flag set to judge
       whether the target is a directory or file, when the file isn't
       a directory, the open request should be denied. But Ext2Fsd 0.45
       and before doesn't. Another issue is that ext2 file time on disk
       has different precision against windows (1 second vs 100 nano
       second), which causes EditPlus thinks the file is being changed.
    4, FIXME: a severe bug (likely happen on win2k system) which cause
       dirty caches missed and slow down the whole system. 
    5, many other minor changes: bulk block allocation/release, possible
       inode allocation dead-loop when all inodes are used out, Ext2Mgr
       win2k support, other performance improvements.

37, V0.45:   2008-02-19

Modifications from V0.44:

    1, set hidden attribute for all files starting with "."
    2, update cache window size when writing to file end
    3, reaper resources allocated during file creation

36, V0.44:   2008-02-09

Modifications from V0.43:

    1, Ext2Fsd: LastWriteTime/LastAccessTime update
    2, Ext2Mgr: Show driver/app versions in About dialog

35, V0.43:   2008-02-01

Modifications from V0.42:

    1, Ext2Fsd: hidden/system attribute handling
    2, Ext2Mgr: Added manifest to cope with UAC

34, V0.42:   2008-01-26

Features implemented:
    1, Mountpoint auto mount/removal supported for removable disks

33, V0.41:   2008-01-25

Modifications from V0.40:

Bugs/Problems Fixed:

    1,  Some files couldn't be shown in explorer with utf8 codepage
    2,  System always pops "device is busy" when removing USB disks

Features implemented:

    1, Ext2Fsd: Updated codepage encodings to latest Linux kernel
    2, Ext2Mgr: Added Chinese translation which is in high demand
                Enhanced mountpoints management for removable devices

32, V0.40:   2008-01-13

Modifications from V0.39

Bugs/Problems Fixed:

    1,  Directory inode isn't freed after directory deletion
    2,  Retrieval pointers implemented for extents mapping
    3,  Correct the handling of STATUS_CANT_WAIT. Vista's Cache
        Manager often fails us on PingRead when copying bunch of
        files/directories from ext2 volumes, and thus it results
        in zero-data-content in copied files
    4,  Optimize space allocation to minimize fragments

31, V0.39:   2008-01-09

Modifications from V0.38

  Bugs Fixed:

    1, Disk space isn't released after deleting big files larger than
       (BLOCK_SIZE * (12 + BLOCK_SIZE/4 + BLOCK_SIZE * BLOCK_SIZE / 16))
       That's 4,299,210,752 in case BLOCK_SIZE is 4096.

30, V0.38:   2008-01-04

Modifications from V0.37

  Bugs Fixed:

     1, File block extents management improved to decrease CPU usage
     2, Re-queue request when cache manager can't prepare pages in time
     3, Possible deletion of it's hardlink entry when removing a file
     4, Prohibit to create file with same names to dead symlinks
     5, Wrong inode type in directory entry for symbol links
     6, Possible memory leak when failed to overwrite file
     7, Memory leak in Ext2FreeMcb, it's counted but not freed.
     8, Possible loss of inode/dentry update: Ext2WriteVolume might
        miss the dirty windows with SECTOR_SIZE aligned borders
     9, SpinLock related routines shouldn't be resident in paged zone
    10, Possible lost of files in directory listing

29, V0.37:   2007-12-25

  Bugs Fixed:

      1, files >=4G couldn't be copy&paste to ext2 volume, caused by a hardcode
         limit in Ext2CreateFile
      2, failure of "No enough memory" when renaming files under directories
         it likely happens when renaming files under subdirectory with Samba
         or Linux kernel CIFS.
      3, possible deadlock in renaming with simultaneous access on the same file
      4, possible wrong entry (hardlinks) deletion instead of itself: it's
         rare to happen but possible when renaming, moving or deleting files. 
      5, entry management routines enhancement 
      6, 64k-block support

28, V0.36:   2007-12-22

  Bugs Fixed:

      1, BSOD caused by symlink pointing to itself or others
      2, bug in new inode allocation algorithm 
      3, "." and ".." are treated as separate inodes
      4, remove unnecessary local variables to reduce stack
         overflow chances especially for nested symlinks
      5, overall optimizations on block allocation/release/mapping 
      6, non-consistent Mcb reference issue (broken by symlinks)
      7, memory leaks of BLOCK_DATA when expanding file
      8, buffer overflow in Ext2Printf only for checked build
      9, bug in handling exception of working cdroms ejection

  Features Newly Implemented:

      1, remove the execute bit ('x') for all newly created files 
      2, enhance memory reaper mechanism of all Xcbs (Fcb/Mcb)
      3, big block support (up to 32k) 
      4, statistics of memory allocation / irp handling

  Ext2 Volume Manager:

      1, crash with RAW disk (non-partitioned)
      2, wrong message box poped when setting service property
      3, more partition types recognized
      4, support memory/irp statistics
      5, keyboard handling, as convenient as using mouse

27, V0.35:   2007-12-02

   Bugs Fixed:

      1, Corrected change notification message for renaming
      2, e2fsck i_size issue: Ext2SetInformation doesn't free
         extra blocks when shrinking FileEndOfFileInformation
      3, BSOD with symlink since it's treated as a file

   Features Newly Implemented:

      1, Symbolic link support on most operations but creating 
      2, Property settings per volume, more flexible
      3, Hiding files with specified prefix and suffix
      4, Different implements on driver letter assignment
      5, CDROM support to mount CD/DVD burned in EXT2 format
      6, "Move and Replace (overwrite)" support
      7, Install Ext2Mgr as a service

26, V0.31A:  2007-01-06

    Bugs/Problems Fixed:

      1, Disable the partition type changing, which causes linux could not boot
      2, Added the test-signature to AMD64 drivers for vista system

25, V0.31:   2006-11-06

   Bugs Fixed:

      1, Wrong ERESOURCE referenced in Ext2DeleteFile
      2, Stale path name referenced for directory notification
      3, Codepage names inconsistent between Ext2Fsd and Ext2Mgr

   Features Newly Implemented:

      1, Vista supported 

24, V0.30:   2006-10-21

   Main Bugs Fixed:

      1, Incorrect removable media support
      2, Wrong file size, timestamps reporting
      3, Some minor memory leaks 
      4, Improper usage of ERESOURCE locking
      5, Wrong handling of some special I/O
      6, Incorrect codepage loading sequence
      7, Mke2fs is much improved

   Main Features Newly Supported:

      1, Network sharing, Oplocks handling
      2, Ext2 Volume Manager (codepage, attributes)

23, V0.25:   2005-07-27

    Bug Fixed:

      1, File allocation blocks are not freed when being deleted
      2, Other minor modifications on the program logics ...

    Features Newly Supported:

      1, Floppy support implemented. 
      2, Win64 (X86 64) support.


22, V0.24:   2005-03-28

    Bug Fixed:

      1, Codepage support errors fixed.
      2, Ext2LookupFileName: should fail when it's not a real path.
      3, Ext2FastIoQueryNetworkOpenInfo: Fcb not initialized.

    Features Newly Supported:

      1, Sparse file support. 
      2, Setup and config tools by Jared Breland.


21, V0.23:   2005-01-09

    Bug Fixed:

      1, Ext2ReadSync: Thread stack is paged out, which causes "Event" is invalid.
      2, Ext2InitializeVcb: Ext2Global->McbList is referred before initializing.

    Features Newly Supported:

      1, Multi codepages supported
        In registry, there's a new item named "CodePage", just change it to the
        codepage name you want to use. The codepage name is the same to linux
        system.

        Here's the codepages list: 

        big5    cp1251  cp1255  cp437   cp737   cp775   cp850   cp852   cp855
        cp857   cp860   cp861   cp862   cp863   cp864   cp865   cp866   cp869
        cp874   cp949   cp950   euc_jp  euc_kr  iso8859_1       iso8859_13 
        iso8859_14  iso8859_15  iso8859_2   iso8859_3   iso8859_4   iso8859_5
        iso8859_6   iso8859_7   iso8859_8   iso8859_9   koi8_r  koi8_ru koi8_u
        sjis    tis_620 cp936   gb2312  utf8

        If you don't specify it or make a wrong page type, system default OEM
        codepage will be used.


20, V0.22b:  2004-11-05

    Bugs Fixed:
      1, Deleting and copying files from/to a directory frequently corrupts
         the directory. E2fsck will report "directory corrupted". This bug
         is cuased by Ext2AddEntry.

19, V0.22a:  2004-10-28

    Bugs Fixed:
      1, Fsck error reports: "inode is in use, but has dtime set." for
         deleted directories. This bug caused by non-zero i_links_count
         of deleted directory inode.
      2, When many files (Normally > 255 files, reported by Zhoujingg)
         are deleted, the dir entry may cross blocks, which will cause
         ext2/ext3 driver could not correctly read the directory entries.

18, V0.22:  2004-10-04

    Bugs Fixed:
      1, Mcb part overflows the stack
      2, Delay-writing errors: cache lost
      3, Bad file type for newly created files

    Features Newly Supported:
      1, Optimize the Mcb management part and the function of Ext2SaveGroup
      2, Big files (> 4G) accessing

17, V0.21:  2004-06-09

    Bugs Fixed:
      1, Ext2Flush does not complete the IRP for READ_ONLY mode.
      2, Delayed close tries to refer completed Irps.
      3, Directory content does NOT ends with zero-inode entry. Ext2ScanDir
         and Ext2QueryDirectory do a wrong way to enum entries. Thanks to 
         "Bomb" <bomb_hero@163.com>
      4, Ext2MountVolume still returns STATUS_SUCCESS for non-ext2 volumes.
      5, Ext2SetRenameInfo always treats "Rename" as "Move"

    Features Newly Supported:
      1, Various sector sizes support from 512 to 4096.
      2, Compatible for windows nt 4.0.
      3, Removable disk supported (CDROM is to be supported in next version.)
      4, Volumes umounting works (Force dismount supported)...
      5, Floppy partially support (NOT complete yet) ...
      6, Add CheckingBitmap / Ext3ForceWriting parameter key in the registry
      7, Quick format supported (directly access on mounted ext2 volumes.)

16, V0.20:  2003-12-26
      1, Merge Petr Borsodi's  patches for several problems.
      2, Unicode/OEM characters support.?
      3, Change notifications is supported.
      4, Totally Optimize and upgrade.
      5, ......

15, V0.10A: 2003-01-14
      1, FIXME: Deadlock caused by CcPurgeCacheSection for readonly volumes

14, V0.10:  2003-01-03
      1, Merge xjaguar's patch: Ext2NewInode and Ext2NewBlock fail on one
         group ext2 system.
      2, Merge Bo Branten's patch: Sync modifications of Romfs.
      3, FIXME: Ext2Flush causes crash when shutting down. Thanks Petr Borsodi
         for his clues.
      4, FIXME: Crash when playing media files (mpg, wav ...)
      5, ......

13, V0.09A: 2002-08-03
      1, Add mcb feature to keep cache coherence between volume and file
         streams 
      2, Skip "AdvacneOnly" when setting FileEndOfFileInformation.
      3, Add file stream dirty flag in order to flush it when cleanup.
      4, Some corrections of lock operations
      5, Fix volume size reporting in Ext2QueryVolumeInformation

12, V0.09: 2002-07-25
      1, Fix some minor bugs
      2, Redesign some internal architectures
      3, Add writing support (Ext3 is not supported yet.)
           1) File write.c: writing functions 
           2) File flush.c: do flushing
           3) File shutdown.c: prepare for system shutdown 
           4) Many modifications in create.c/fileinfo.c/volinfo.c ...

11, V0.08: 2002-05-28
      1, Fix page-fault bsod when reading bad-size inodes
      2, Fix a minor bug in Ext2GetBlock
      3, Add binary-tree-mechanism to store the info of opened files

10, V0.07A: 2002-05-17
      1, Merge Boose's patch: make ext2fsd cooperating with his ntifs.h
         (verion r36) and winxp ddk.
      2, Fix wild-cases matching in Ext2QueryDirectory

 9, V0.07: 2002-05-14
      1, FIXME: only some minor bugs
      2, Add access protection of some sharable members
      3, Add booting start support
      4, Change the driver's name to ext2fsd.sys

 8, V0.06: 2002-03-12
      1, FIXME: BSOD when playing wav/mpeg/avi with IE embedded media player.
      2, Some optimizations in Ext2QueryDirectionary and Ext2ScanDir.

 7, V0.05: 2002-03-01
      1, FIXME: Ext2Create: Ext2ScanDir returns wrong fcb.
      2, Support file executing now.

 6, V0.04: 2002-02-20
      1, Add caching support. 

 5, V0.03: 2002-02
      (It's an internal version, not released.)
      1, Keep up with romfs: add byte-range lock and directory change
         notification.

 4, V0.02: 2002-02-07
      1, Add large ext2 partition support. 

 3, V0.01: 2002-01-26
      1, Make it public at http://ext2.yeah.net.

 2, 2001-10
      1, Non-public pre-release of ext2fsd was born.

 1, 2001-08
      1, I began the journey of file system driver developing with romfs


======================
 Bugs
======================

(Nobody can expect that ext2fsd is bug-free.)
Any bugs, please mail to me.


======================
 How to install
======================

1, Build this project, and ext2fsd.sys will be created.
2, Copy ext2fsd.sys to %system32%\drivers.
3, Import the ext2fsd.reg into your system registry.
4, After rebooting your machine, run "net start ext2fsd" at a dos shell.
5, Use mount/unmount tools to mount or unmount a disk partition.
6, You can unload ext2fsd from system with unload tool.

After version 0.10, these is an easy way:

1, Just run "Setup.bat" in "Setup" subdir for the first time. Then just
   run "net start ext2fsd" in a dos shell to start the driver.

      After v0.23, you need specify your system version:

        setup 2k: to install ext2fsd for windows 200
        setup xp: to install ext2fsd for windows xp


2, Then you can mount the ext2 partitions without rebooting.


======================
 How to uninstall
======================

1, Just run uninstall.bat in the Ext2Fsd.zip package.
   .OR.
2, Use the "Add/Remove Programs" in "Control Panel", click the item of
   "linux ext2 file system driver" to remove the program.
   .OR.
3, Manually remove the ext2fsd projects files. 
   a) Remove the registry: HKLM\...\Ext2Fsd & Uninstall ...
   b) Remove Mount.exe/Mke2fs.exe from $(SystemRoot)\system32
   c) Remove Ext2Fsd.sys from $(SystemRoot)\system32\drivers
   d) Remove ext2fsd.inf from $(SystemRoot)\inf


======================
 Writing support
======================

Two ways are optional to enable writing support.

1, Change the value of "Parameters\WritingSupport" in ext2fsd.reg to 1, then
   import the reg file into system registry.
   Or modify the registry directly. Like this:

   -------------------------------------------------------------------------
   [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Ext2Fsd\Parameters]
   "WritingSupport"=dword:00000001
   -------------------------------------------------------------------------

   Notes: Reboot is need.

   Or

2, Change the definition of EXT2_READ_ONLY to 0, and recompile ext2fsd, then
   overwrite the original one. (See line 27 in ext2fs.h)


Please refer FAQ.txt for more information.
