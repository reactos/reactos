#ifndef __LINK_H__
#define __LINK_H__

// Typdefs
#define EXP_SZ_LINK_SIG                0xA0000001
#define EXP_TRACKER_SIG                0xA0000003
#define EXP_SPECIAL_FOLDER_SIG         0xA0000005
#define EXP_SZ_ICON_SIG                0xA0000007

typedef struct
{
    DWORD       cbSize;             // Size of this extra data block
    DWORD       dwSignature;        // signature of this extra data block
} EXP_HEADER, *LPEXP_HEADER;

typedef struct 
{
    DWORD       cbSize;             // Size of this extra data block
    DWORD       dwSignature;        // signature of this extra data block
    DWORD       idSpecialFolder;        // special folder id this link points into
    DWORD       cbOffset;               // ofset into pidl from SLDF_HAS_ID_LIST for child
} EXP_SPECIAL_FOLDER, *LPEXP_SPECIAL_FOLDER;


#ifdef WINNT
typedef struct 
{
    DWORD       cbSize;             // Size of this extra data block
    DWORD       dwSignature;        // signature of this extra data block
    BYTE        abTracker[ 1 ];         //
} EXP_TRACKER, *LPEXP_TRACKER;
#endif


typedef struct 
{
    DWORD       cbSize;             // Size of this extra data block
    DWORD       dwSignature;        // signature of this extra data block
    CHAR        szTarget[ MAX_PATH ];   // ANSI target name w/EXP_SZ in it
    WCHAR       swzTarget[ MAX_PATH ];  // UNICODE target name w/EXP_SZ in it
} EXP_SZ_LINK;
typedef EXP_SZ_LINK *LPEXP_SZ_LINK;

#endif //__LINK_H__

