#ifndef __INCLUDE_DDK_FSTYPES_H
#define __INCLUDE_DDK_FSTYPES_H
/* $Id: fstypes.h,v 1.3 2001/09/07 21:37:47 ea Exp $ */


typedef
struct _FILE_LOCK_ANCHOR
{
    LIST_ENTRY GrantedFileLockList;
    LIST_ENTRY PendingFileLockList;

} FILE_LOCK_ANCHOR, *PFILE_LOCK_ANCHOR;

typedef PVOID PNOTIFY_SYNC;


#endif /* __INCLUDE_DDK_FSFUNCS_H */
