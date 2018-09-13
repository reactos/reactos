/*
 * twinlist.h - Twin list ADT description.
 */


/* Prototypes
 *************/

/* twinlist.c */

extern HBRFCASE GetTwinListBriefcase(HTWINLIST);
extern ARRAYINDEX GetTwinListCount(HTWINLIST);
extern HTWIN GetTwinFromTwinList(HTWINLIST, ARRAYINDEX);
extern BOOL IsValidHTWINLIST(HTWINLIST);

