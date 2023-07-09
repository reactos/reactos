/*
 * foldtwin.h - Folder twin ADT description.
 */


/* Prototypes
 *************/

/* foldtwin.c */

extern BOOL CreateFolderPairPtrArray(PHPTRARRAY);
extern void DestroyFolderPairPtrArray(HPTRARRAY);
extern TWINRESULT MyTranslateFolder(HBRFCASE, HPATH, HPATH);
extern BOOL IsValidHFOLDERTWIN(HFOLDERTWIN);

