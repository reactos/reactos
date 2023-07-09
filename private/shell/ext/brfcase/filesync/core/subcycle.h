/*
 * subcycle.h - Subtree cycle detection routines description.
 */


/* Prototypes
 *************/

/* subcycle.c */

extern void BeginTranslateFolder(PFOLDERPAIR);
extern void EndTranslateFolder(PFOLDERPAIR);
extern TWINRESULT CheckForSubtreeCycles(HPTRARRAY, HPATH, HPATH, HSTRING);

