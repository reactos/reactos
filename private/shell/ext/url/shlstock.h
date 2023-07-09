/*
 * shlstock.h - Stock Shell header file.
 */


#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */

#ifdef WINNT_ENV    /* shlapip */
#include <shlapip.h>
#endif

/* Types
 ********/

/* interfaces */

DECLARE_STANDARD_TYPES(IExtractIcon);
DECLARE_STANDARD_TYPES(INewShortcutHook);
DECLARE_STANDARD_TYPES(IShellExecuteHook);
DECLARE_STANDARD_TYPES(IShellLink);
DECLARE_STANDARD_TYPES(IShellExtInit);
DECLARE_STANDARD_TYPES(IShellPropSheetExt);

/* structures */

DECLARE_STANDARD_TYPES(DROPFILES);
DECLARE_STANDARD_TYPES(FILEDESCRIPTOR);
DECLARE_STANDARD_TYPES(FILEGROUPDESCRIPTOR);
DECLARE_STANDARD_TYPES(PROPSHEETPAGE);
DECLARE_STANDARD_TYPES(SHELLEXECUTEINFO);
DECLARE_STANDARD_TYPES_U(ITEMIDLIST);  /* WINNT: RISC care about alignment: */
/* WINNT: LPITEMIDLIST+LPCITEMIDLIST declared UNALIGNED in sdk\inc\shlobj.h */

/* flags */

typedef enum _shellexecute_mask_flags
{
   ALL_SHELLEXECUTE_MASK_FLAGS = 0x001FFFFF
}
SHELLEXECUTE_MASK_FLAGS;


#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

