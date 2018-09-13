#ifndef _YNLIST_H
#define _YNLIST_H

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


typedef struct {

    LPTSTR pszzList;            // Double NULL terminated list of directories
    UINT  cbAlloc;              // Space allocated to list, in BYTEs
    UINT  cchUsed;              // Space used in list, in CHARacters
    BOOL  fEverythingInList;    // TRUE if everything is considered on the list

} DIRLIST, *PDIRLIST;

typedef struct {

    DIRLIST dlYes;              // List of YES directories
    DIRLIST dlNo;               // List of NO directories

} YNLIST, *PYNLIST;

void CreateYesNoList(PYNLIST pynl);
void DestroyYesNoList(PYNLIST pynl);
BOOL IsInYesList(PYNLIST pynl, LPCTSTR szItem);
BOOL IsInNoList(PYNLIST pynl, LPCTSTR szItem);
void AddToYesList(PYNLIST pynl, LPCTSTR szItem);
void AddToNoList(PYNLIST pynl, LPCTSTR szItem);
void SetYesToAll(PYNLIST pynl);

#ifdef __cplusplus
};
#endif  /* __cplusplus */

#endif  // _YNLIST_H
