#ifndef _WINNETWK_
#include <winnetwk.h>
#endif // _WINNETWK_

#define SHID_JUNCTION           0x80

#define SHID_GROUPMASK          0x70
#define SHID_TYPEMASK           0x7f
#define SHID_INGROUPMASK        0x0f

#define SHID_ROOT               0x10
#define SHID_ROOT_REGITEM       0x1f    // Mail

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

#define SHID_FS                 0x30
#define SHID_FS_TYPEMASK        0x3F
#define SHID_FS_DIRECTORY       0x31    // CHICAGO
#define SHID_FS_FILE            0x32    // FOO.TXT
#define SHID_FS_UNICODE         0x34    // Is it unicode? (this is a bitmask)
#define SHID_FS_DIRUNICODE      0x35    // Folder with a unicode name
#define SHID_FS_FILEUNICODE     0x36    // File with a unicode name

#define SHID_NET                0x40    
#define SHID_NET_DOMAIN         (SHID_NET | RESOURCEDISPLAYTYPE_DOMAIN)
#define SHID_NET_SERVER         (SHID_NET | RESOURCEDISPLAYTYPE_SERVER)
#define SHID_NET_SHARE          (SHID_NET | RESOURCEDISPLAYTYPE_SHARE)
#define SHID_NET_FILE           (SHID_NET | RESOURCEDISPLAYTYPE_FILE)
#define SHID_NET_GROUP          (SHID_NET | RESOURCEDISPLAYTYPE_GROUP)
#define SHID_NET_NETWORK        (SHID_NET | RESOURCEDISPLAYTYPE_NETWORK)
#define SHID_NET_RESTOFNET      (SHID_NET | RESOURCEDISPLAYTYPE_ROOT)
#define SHID_NET_SHAREADMIN     (SHID_NET | RESOURCEDISPLAYTYPE_SHAREADMIN)
#define SHID_NET_DIRECTORY      (SHID_NET | RESOURCEDISPLAYTYPE_DIRECTORY)
#define SHID_NET_TREE           (SHID_NET | RESOURCEDISPLAYTYPE_TREE)
#define SHID_NET_REGITEM        0x4e    // Remote Computer items
#define SHID_NET_PRINTER        0x4f    // \\PYREX\LASER1

#define SIL_GetType(pidl)       (ILIsEmpty(pidl) ? 0 : (pidl)->mkid.abID[0])
#define FS_IsValidID(pidl)      ((SIL_GetType(pidl) & SHID_GROUPMASK) == SHID_FS)
#define NET_IsValidID(pidl)     ((SIL_GetType(pidl) & SHID_GROUPMASK) == SHID_NET)

typedef struct _ICONMAP // icmp
{
    UINT        uType;                  // SHID_ type
    UINT        indexResource;          // Resource index (of SHELL232.DLL)
} ICONMAP, FAR* LPICONMAP;

UINT SILGetIconIndex(LPCITEMIDLIST pidl, const ICONMAP aicmp[], UINT cmax);

#pragma pack(1)
typedef struct _IDNETRESOURCE   // idn
{
        WORD    cb;
        BYTE    bFlags;         // Display type in low nibble
        BYTE    uType;
        BYTE    uUsage;         // Usage in low nibble, More Flags in high nibble
        CHAR    szNetResName[1];
        // char szProvider[*] - If NET_HASPROVIDER bit is set
        // char szComment[*]  - If NET_HASCOMMENT bit is set.
        // WCHAR szNetResNameWide[*] - If NET_UNICODE bit it set.
        // WCHAR szProviderWide[*]   - If NET_UNICODE and NET_HASPROVIDER
        // WCHAR szCommentWide[*]    - If NET_UNICODE and NET_HASCOMMENT
} IDNETRESOURCE, *LPIDNETRESOURCE;
typedef const IDNETRESOURCE *LPCIDNETRESOURCE;
#pragma pack()

//===========================================================================
// CNetwork: Some private macro
//===========================================================================

#define NET_DISPLAYNAMEOFFSET           ((UINT)((LPIDNETRESOURCE)0)->szNetResName)
#define NET_GetFlags(pidnRel)           ((pidnRel)->bFlags)
#define NET_GetDisplayType(pidnRel)     ((pidnRel)->bFlags & 0x0f)
#define NET_GetType(pidnRel)            ((pidnRel)->uType)
#define NET_GetUsage(pidnRel)           ((pidnRel)->uUsage & 0x0f)

// Define some Flags that are on high nibble of uUsage byte
#define NET_HASPROVIDER                 0x80    // Has own copy of provider
#define NET_HASCOMMENT                  0x40    // Has comment field in pidl
#define NET_REMOTEFLD                   0x20    // Is remote folder
#define NET_UNICODE                     0x10    // Has unicode names
#define NET_FHasComment(pidnRel)        ((pidnRel)->uUsage & NET_HASCOMMENT)
#define NET_FHasProvider(pidnRel)        ((pidnRel)->uUsage & NET_HASPROVIDER)
#define NET_IsRemoteFld(pidnRel)        ((pidnRel)->uUsage & NET_REMOTEFLD)
#define NET_IsUnicode(pidnRel)          ((pidnRel)->uUsage & NET_UNICODE)
