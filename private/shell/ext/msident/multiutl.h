
#ifndef __MULTIUTL_H
#define __MULTIUTL_H

#include <assert.h>
#include "objidl.h"
#include <pstore.h>
#include "multiusr.h"

#ifndef Assert
#ifdef DEBUG
#define Assert(a)		assert(a)
#define SideAssert(a)	Assert(a)
#define AssertSz(a, sz) Assert(a)
#define ASSERT_MSGA     1 ? (void)0 : (void)
#else	// DEBUG
#define ASSERT_MSGA     1 ? (void)0 : (void)
#define Assert(a)
#define SideAssert(a)	(a)
#define AssertSz(a, sz) 
#endif	// DEBUG, else

#endif

#ifdef UNICODE
#define FIsSpace            FIsSpaceW
#else
#define FIsSpace            FIsSpaceA
#endif


// Context-sensitive Help utility.
typedef struct _tagHELPMAP
    {
    DWORD   id; 
    DWORD   hid;
    } HELPMAP, *LPHELPMAP;

BOOL OnContextHelp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, HELPMAP const * rgCtxMap);


ULONG UlStripWhitespace(LPTSTR lpsz, BOOL fLeading, BOOL fTrailing, ULONG *pcb);
void    EncodeUserPassword(TCHAR *lpszPwd, ULONG *cb);
void    DecodeUserPassword(TCHAR *lpszPwd, ULONG *cb);
STDAPI WriteIdentityPassword(GUID *puidIdentity, PASSWORD_STORE  *pPwdStore);
STDAPI  ReadIdentityPassword(GUID *puidIdentity, PASSWORD_STORE  *pPwdStore);
STDAPI  CreatePStore(IPStore **ppIPStore);
STDAPI  ReleasePStore(IPStore *pIPStore);



// --------------------------------------------------------------------------------
// SafeRelease - Releases an object and sets the object to NULL
// --------------------------------------------------------------------------------
#define SafeRelease(_object) \
    if (_object) { \
        (_object)->Release(); \
        (_object) = NULL; \
    } else

// --------------------------------------------------------------------------------
// Memory Utility Functions
// --------------------------------------------------------------------------------
extern IMalloc *g_pMalloc;

// --------------------------------------------------------------------------------
// SafeMemFree
// --------------------------------------------------------------------------------
#ifndef SafeMemFree
#ifdef __cplusplus
#define SafeMemFree(_pv) \
    if (_pv) { \
        g_pMalloc->Free(_pv); \
        _pv = NULL; \
    } else
#else
#define SafeMemFree(_pv) \
    if (_pv) { \
        g_pMalloc->lpVtbl->Free(g_pMalloc, _pv); \
        _pv = NULL; \
    } else
#endif // __cplusplus
#endif // SafeMemFree

// --------------------------------------------------------------------------------
// MemFree
// --------------------------------------------------------------------------------
#define MemFree(_pv)        g_pMalloc->Free(_pv)
#define ReleaseMem(_pv)     MemFree(_pv)

// --------------------------------------------------------------------------------
// Memory Allocation Functions
// --------------------------------------------------------------------------------
VOID       MemInit();
VOID       MemUnInit();

LPVOID     ZeroAllocate(DWORD cbSize);
BOOL       MemAlloc(LPVOID* ppv, ULONG cb);
BOOL       MemRealloc(LPVOID *ppv, ULONG cbNew);

// --------------------------------------------------------------------------------
// TraceCall(_pszFunc)
// -------------------------------------------------------------------------------
#ifdef DEBUG
EXTERN_C void DebugStrf(LPTSTR lpszFormat, ...);
#endif

#if defined(DEBUG)
#define TraceCall(_pszFunc) DebugStrf("%s\r\n", _pszFunc)
#else
#define TraceCall(_pszFunc)
#endif

// --------------------------------------------------------------------------------
// GUID <-> Ascii string functions
// --------------------------------------------------------------------------------
HRESULT GUIDFromAString(TCHAR *lpsz, GUID *puid);
int     AStringFromGUID(GUID *rguid,  TCHAR *lpsz, int cch);

// --------------------------------------------------------------------------------
// Declarations for overloading global new and delete.
// --------------------------------------------------------------------------------

void * __cdecl operator new(size_t size);
void __cdecl operator delete(void *ptr) throw();

typedef enum 
{
    NS_NONE = 0,
    NS_PRE_NOTIFY,
    NS_NOTIFIED
} NOTIFICATION_STATE;

typedef struct tagUNKLIST_ENTRY
{
    HWND        hwnd;
    DWORD       dwThreadId;
    DWORD       dwCookie;
    BYTE        bState;
    IUnknown   *punk;
} UNKLIST_ENTRY;

class CNotifierList
{
public:
                            CNotifierList();
    virtual                 ~CNotifierList();

    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);

    inline  DWORD           GetLength(void)      {return m_count;}
            HRESULT         Add(IUnknown *punk, DWORD *pdwCookie);
            HRESULT         RemoveCookie(DWORD dwCookie);
            HRESULT         Remove(int iIndex);
            HRESULT         GetAtIndex(int iIndex, IUnknown **ppunk);  
            HRESULT         CreateNotifyWindow();
            HRESULT         ReleaseWindow();
            HRESULT         SendNotification(UINT msg, DWORD dwType);
            HRESULT         PreNotify();
            int             GetNextNotify();
private:
    ULONG               m_cRef;
    int                 m_count;
    int                 m_ptrCount;
    DWORD               m_nextCookie;
    UNKLIST_ENTRY      *m_entries;      
    CRITICAL_SECTION    m_rCritSect;
};



#endif  //__MULTIUTL_H
