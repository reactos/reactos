#ifndef __INCLUDE_DDK_FSTYPES_H
#define __INCLUDE_DDK_FSTYPES_H
/* $Id */

#include <internal/ps.h>

typedef
struct _FILE_LOCK_ANCHOR
{
    LIST_ENTRY GrantedFileLockList;
    LIST_ENTRY PendingFileLockList;

} FILE_LOCK_ANCHOR, *PFILE_LOCK_ANCHOR;

typedef PVOID PNOTIFY_SYNC;


#endif /* __INCLUDE_DDK_FSFUNCS_H */
