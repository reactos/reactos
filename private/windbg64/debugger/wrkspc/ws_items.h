#ifndef __WS_ITEMS_H__
#define __WS_ITEMS_H__



class CItemInterface_WKSP {
public:
    // Never modify this
    const char * const  m_pszRegistryName;

public:
    CItemInterface_WKSP(const char * const pszRegistryName);
    virtual ~CItemInterface_WKSP();

protected:
    void SetRegistryName(const char * const pszRegistryName);

    virtual BOOL Read(const HKEY hkey) const;

public:
    virtual void Save(const HKEY hkey) const;
    virtual void Restore(const HKEY hkey) const;

public:
    virtual DWORD GetRegType() const = 0;
    virtual DWORD CalcSizeOfData() const = 0;
    virtual PVOID GetPtrToData() const = 0;

    virtual BOOL GetValue(PDWORD pdwType, PVOID pvData, PDWORD pcbData) const;

    virtual void Duplicate(const CItemInterface_WKSP & arg);

    virtual BOOL DoDataPointersMatch(PVOID pv) const = 0;
};



template<class T, const DWORD m_dwRegType>
class CItem_WKSP : public CItemInterface_WKSP {
public:
    T * const           m_ptData;

public:
    CItem_WKSP(const char * const pszRegistryName, T * pt)
        : CItemInterface_WKSP(pszRegistryName), m_ptData(pt) {};

public:
    virtual DWORD GetRegType() const { return m_dwRegType; }
    virtual PVOID GetPtrToData() const { return m_ptData; }
    virtual DWORD CalcSizeOfData() const;

    virtual BOOL DoDataPointersMatch(PVOID pv) const { return m_ptData == pv; }
};










//
// Instances of data: DWORD, int, etc.
//
typedef CItem_WKSP<int, REG_DWORD>                  CINT_ITEM_WKSP;
typedef CItem_WKSP<DWORD, REG_DWORD>                CDWORD_ITEM_WKSP;
typedef CItem_WKSP<long, REG_DWORD>                 CLONG_ITEM_WKSP;
typedef CItem_WKSP<BOOL, REG_DWORD>                 CBOOL_ITEM_WKSP;
typedef CItem_WKSP<PSTR, REG_SZ>                    CSZ_ITEM_WKSP;
typedef CItem_WKSP<PSTR, REG_MULTI_SZ>              CMULTI_SZ_ITEM_WKSP;

typedef CItem_WKSP<LOGFONT, REG_BINARY>             CLOGFONT_ITEM_WKSP;
typedef CItem_WKSP<WINDOW_STATE, REG_DWORD>         CWINDOW_STATE_ITEM_WKSP;


#endif
