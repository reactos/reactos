#ifndef _SFVLIST_H_
#define _SFVLIST_H_

#include <shlobj.h>
#include "sfview.h"

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

HRESULT PASCAL RemView_OnGetViews(
    SHELLVIEWID FAR *pvid,
    IEnumSFVViews FAR * FAR *ppObj);

#ifdef __cplusplus
}            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#ifdef __cplusplus

#pragma data_seg("INSTDATA")

class CUnkNoRef
{
public:
    static HRESULT QueryInterface(REFIID riid, LPVOID * ppvObj,
        const IID* const priid[], const LPUNKNOWN ppObj[], REFIID riidHack);
} ;


class CUnknown : public CUnkNoRef
{
public:
    CUnknown() : m_cRef(1) {}
    virtual ~CUnknown() {}

    ULONG AddRef();
    ULONG Release();

private:
    UINT m_cRef;
} ;

class CGenList
{
public:
    CGenList(UINT cbItem) : m_hList(NULL), m_cbItem(cbItem) {}
    ~CGenList() {Empty();}

    LPVOID GetPtr(UINT i)
        {return(i<GetItemCount() ? DSA_GetItemPtr(m_hList, i) : NULL);}

    UINT GetItemCount() {return(m_hList ? DSA_GetItemCount(m_hList) : 0);}

    int Add(LPVOID pv);

    void Empty() {if (m_hList) DSA_Destroy(m_hList); m_hList=NULL;}

protected:
    void Steal(CGenList* pList)
    {
        Empty();
        m_cbItem = pList->m_cbItem;
        m_hList = pList->m_hList;
        pList->m_hList = NULL;
    }

private:
    UINT m_cbItem;
    HDSA m_hList;
} ;


class CViewsList : public CGenList
{
public:
    CViewsList() : CGenList(sizeof(SFVVIEWSDATA*)), m_bGotDef(FALSE) {}
    ~CViewsList() {Empty();}

    SFVVIEWSDATA* GetPtr(UINT i)
    {
        SFVVIEWSDATA** ppViewsData = (SFVVIEWSDATA**)CGenList::GetPtr(i);
        return(ppViewsData ? *ppViewsData : NULL);
    }

    int Add(const SFVVIEWSDATA*pView, BOOL bCopy=TRUE);
    void AddReg(HKEY hkParent, LPCTSTR pszSubKey);
    void AddCLSID(CLSID const* pclsid);

    void SetDef(SHELLVIEWID const *pvid) { m_bGotDef=TRUE; m_vidDef=*pvid; }
    BOOL GetDef(SHELLVIEWID *pvid) { if (m_bGotDef) *pvid=m_vidDef; return(m_bGotDef); }

    void Empty();

    static SFVVIEWSDATA* CopyData(const SFVVIEWSDATA* pData);

private:
    BOOL m_bGotDef;
    SHELLVIEWID m_vidDef;
} ;

class CRegKey
{
public:
    CRegKey(HKEY hkParent, LPCTSTR pszSubKey, BOOL bCreate=FALSE)
    {
        if ((bCreate ? RegCreateKey(hkParent, pszSubKey, &m_hk)
            : RegOpenKey(hkParent, pszSubKey, &m_hk))!=ERROR_SUCCESS)
        {
            m_hk = NULL;
        }
    }
    CRegKey(HKEY hk) { m_hk=hk; }
    ~CRegKey()
    {
        if (m_hk) RegCloseKey(m_hk);
    }

    operator HKEY() const { return(m_hk); }
    operator !() const { return(m_hk==NULL); }

    HRESULT QueryValue(LPCTSTR szSub, LPTSTR pszVal, LONG cb)
        { return(RegQueryValue(m_hk, szSub, pszVal, &cb)); }

private:
    HKEY m_hk;
};

#pragma data_seg()

#endif /* __cplusplus */

#endif // _SFVLIST_H_
