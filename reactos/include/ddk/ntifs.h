#ifndef __INCLUDE_DDK_NTIFS_H
#define __INCLUDE_DDK_NTIFS_H

#if 0
typedef struct
{
   BOOLEAN Replace;
   HANDLE RootDir;
   ULONG FileNameLength;
   WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;
#endif 

#include <ddk/cctypes.h>

#include <ddk/ccfuncs.h>

#endif /* __INCLUDE_DDK_NTIFS_H */
