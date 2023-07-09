#ifndef _WINNETWK_
#include <winnetwk.h>
#endif // _WINNETWK_

// NOTE: these values can never change. they define the format of IDLIST that
// may be persisted in .lnk files and elsewhere.

#define SHID_JUNCTION           0x80

#define SHID_GROUPMASK          0x70
#define SHID_TYPEMASK           0x7f
#define SHID_INGROUPMASK        0x0f

#define SHID_ROOT               0x10
#define SHID_ROOT_REGITEM       0x1f    // MyDocuments, Internet, etc

#if ((DRIVE_REMOVABLE|DRIVE_FIXED|DRIVE_REMOTE|DRIVE_CDROM|DRIVE_RAMDISK) != 0x07)
#error Definitions of DRIVE_* are changed!
#endif

#define SHID_COMPUTER           0x20
#define SHID_COMPUTER_1         0x21    // free
#define SHID_COMPUTER_REMOVABLE (0x20 | DRIVE_REMOVABLE)  // 2
#define SHID_COMPUTER_FIXED     (0x20 | DRIVE_FIXED)      // 3
#define SHID_COMPUTER_REMOTE    (0x20 | DRIVE_REMOTE)     // 4
#define SHID_COMPUTER_CDROM     (0x20 | DRIVE_CDROM)      // 5
#define SHID_COMPUTER_RAMDISK   (0x20 | DRIVE_RAMDISK)    // 6
#define SHID_COMPUTER_7         0x27    // free
#define SHID_COMPUTER_DRIVE525  0x28    // 5.25 inch floppy disk drive
#define SHID_COMPUTER_DRIVE35   0x29    // 3.5 inch floppy disk drive
#define SHID_COMPUTER_NETDRIVE  0x2a    // Network drive
#define SHID_COMPUTER_NETUNAVAIL 0x2b   // Network drive that is not restored.
#define SHID_COMPUTER_C         0x2c    // free
#define SHID_COMPUTER_D         0x2d    // free
#define SHID_COMPUTER_REGITEM   0x2e    // Controls, Printers, ...
#define SHID_COMPUTER_MISC      0x2f    // Unknown drive type

#define SHID_FS                   0x30  // base simple IDList, we don't generate these anymore
#define SHID_FS_TYPEMASK          0x37
#define SHID_FS_DIRECTORY         0x31  // WINDOWS (a folder)
#define SHID_FS_FILE              0x32  // FOO.TXT (a file)
#define SHID_FS_UNICODE           0x34  // unicode (this is a bitmask)
#define SHID_FS_DIRUNICODE        0x35  // Folder with a unicode name
#define SHID_FS_FILEUNICODE       0x36  // File with a unicode name
#define SHID_FS_COMMONITEM        0x38  // Common item
#define SHID_FS_COMMONDIRECTORY   0x39  // Common directory (ansi)
#define SHID_FS_COMMONFILE        0x3a  // Common file (ansi)
#define SHID_FS_COMMONDIRUNICODE  0x3d  // Common folder with a unicode name
#define SHID_FS_COMMONFILEUNICODE 0x3e  // Common file with a unicode name


#define SHID_NET                0x40
#define SHID_NET_DOMAIN         (SHID_NET | RESOURCEDISPLAYTYPE_DOMAIN)         // 0x41
#define SHID_NET_SERVER         (SHID_NET | RESOURCEDISPLAYTYPE_SERVER)         // 0x42
#define SHID_NET_SHARE          (SHID_NET | RESOURCEDISPLAYTYPE_SHARE)          // 0x43
#define SHID_NET_FILE           (SHID_NET | RESOURCEDISPLAYTYPE_FILE)           // 0x44
#define SHID_NET_GROUP          (SHID_NET | RESOURCEDISPLAYTYPE_GROUP)          // 0x45
#define SHID_NET_NETWORK        (SHID_NET | RESOURCEDISPLAYTYPE_NETWORK)        // 0x46
#define SHID_NET_RESTOFNET      (SHID_NET | RESOURCEDISPLAYTYPE_ROOT)           // 0x47
#define SHID_NET_SHAREADMIN     (SHID_NET | RESOURCEDISPLAYTYPE_SHAREADMIN)     // 0x48
#define SHID_NET_DIRECTORY      (SHID_NET | RESOURCEDISPLAYTYPE_DIRECTORY)      // 0x49
#define SHID_NET_TREE           (SHID_NET | RESOURCEDISPLAYTYPE_TREE)           // 0x4A
#define SHID_NET_NDSCONTAINER   (SHID_NET | RESOURCEDISPLAYTYPE_NDSCONTAINER)   // 0x4B
#define SHID_NET_REGITEM        0x4d    // RegItem in either Entire Net, or the Root
#define SHID_NET_REMOTEREGITEM  0x4e    // Remote Computer items
#define SHID_NET_PRINTER        0x4f    // \\PYREX\LASER1

#ifndef SHID_LOC
// this group is for location items.
// they are defined in inc\shellp.h
#define SHID_LOC                0x50
#define SHID_LOC_TYPEMASK       0x5F
#endif

#define SHID_CONTROLPANEL_REGITEM       0x70
#define SHID_CONTROLPANEL_REGITEM_EX    0x71

#define SIL_GetType(pidl)       (ILIsEmpty(pidl) ? 0 : (pidl)->mkid.abID[0])

