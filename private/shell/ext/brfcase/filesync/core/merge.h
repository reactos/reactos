/*
 * merge.h - File merge handler module description.
 */


/* Prototypes
 *************/

/* merge.c */

extern void BeginMerge(void);
extern void EndMerge(void);
extern HRESULT MergeHandler(PRECNODE, RECSTATUSPROC, LPARAM, DWORD, HWND, HWND, PRECNODE *);
extern HRESULT MyCreateFileMoniker(LPCTSTR, LPCTSTR, PIMoniker *);
extern void ReleaseIUnknowns(ULONG, PIUnknown *);
extern HRESULT OLECopy(PRECNODE, PCCLSID, RECSTATUSPROC, LPARAM, DWORD, HWND, HWND);

