#include "shellprv.h"

/*
  this caches a mapping between hwnd's and ishell browsers

  this facilitates finding the ishellbrowser and telling it to 
  browse to something.. that browser may or may not decide to
  then create a new window, but if it does, it can then pass along
  history information.
  
  
  currently this is kept per instance.  at this point, it would help us
  to make the cache global because we don't marshal the 
  IShellBrowser::BrowseToObject calls anyways...  when this changes, we 
  should also make the cache system global.

 */

HDSA g_hdsaSB = NULL;

typedef struct tabSBCacheItem {
    HWND hwnd;
    LPSHELLBROWSER psb;
} SBCACHEITEM, *PSBCACHEITEM;

enum {
    SBCF_ADD,
    SBCF_REMOVE,
    SBCF_FIND,
} ;

int SBFind(HWND hwnd , LPSHELLBROWSER *ppsb)
{
    int i = -1;
    PSBCACHEITEM pci;
    LPSHELLBROWSER psb = NULL;
    
    if (g_hdsaSB) {
        ENTERCRITICAL;
        
        // walk the dsa looking for the hwnd
        for (i = DSA_GetItemCount(g_hdsaSB) - 1; i >= 0; i--) {
            pci = DSA_GetItemPtr(g_hdsaSB, i);
            if (pci->hwnd == hwnd) {
                psb = pci->psb;
                break;
            }
        }
        LEAVECRITICAL;
    }
    
    if (ppsb) 
        *ppsb = psb;
    
    return i;
}

LPSHELLBROWSER SHSBCache(UINT uOperation, HWND hwnd, LPSHELLBROWSER psb)
{
    PSBCACHEITEM pci;
    int i;
    switch (uOperation)
    {
    case SBCF_ADD:
        ENTERCRITICAL;
        if (!g_hdsaSB) {
            g_hdsaSB = DSA_Create(SIZEOF(SBCACHEITEM), 1);
        }
        LEAVECRITICAL;
        
        if (!g_hdsaSB)
            return NULL;
        
        ENTERCRITICAL;
        i = SBFind(hwnd, NULL);
        if (i == -1) {
            SBCACHEITEM ci;
            ci.hwnd = hwnd;
            ci.psb = psb;
            i = DSA_AppendItem(g_hdsaSB, &ci);
            if (i == -1) {
                // failed to add.
                psb = NULL;
            }
        } else {
            // was already in there.. make sure things are correct
            pci = DSA_GetItemPtr(g_hdsaSB, i);
            ASSERT(pci->hwnd == hwnd);
            ASSERT(pci->psb == psb);
            pci->psb = psb;
        }
        LEAVECRITICAL;
        break;
        
    case SBCF_REMOVE:
        ENTERCRITICAL;
        i = SBFind(hwnd, &psb);
        if (i != -1) {
            DSA_DeleteItem(g_hdsaSB, i);
        }
        LEAVECRITICAL;
        break;
        
    case SBCF_FIND:
        i = SBFind(hwnd, &psb);
        break;
    }
    
    return psb;
}

