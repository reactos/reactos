/**********************************************************************
 * ExtInit.h - Definition for the CShellExtInit, our implementation for
 *             IShellExtInit.
 *
 **********************************************************************/

#if !defined(__EXTINIT_H__)
#define __EXTINIT_H__

class CShellExtInit : public IShellExtInit, public IContextMenu,
   public IShellPropSheetExt
{
public:
   CShellExtInit();
   ~CShellExtInit();
   BOOL bInit();

   // *** IUnknown methods ***

   STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   // *** IShellExtInit methods ***

   STDMETHODIMP Initialize( LPCITEMIDLIST pidlFolder,
                            LPDATAOBJECT lpdobj,
                            HKEY hkeyProgID);

   // ** IContextMenu methods ***

   STDMETHODIMP QueryContextMenu( HMENU hmenu,
                                  UINT indexMenu,
                                  UINT idCmdFirst,
                                  UINT idCmdLast,
                                  UINT uFlags);

   STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

   STDMETHODIMP GetCommandString( UINT_PTR idCmd,
                                  UINT   uFlags,
                                  UINT  *pwReserved,
                                  LPSTR  pszName,
                                  UINT   cchMax);

   // ***IShellPropSheet Ext ***
   //
   STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
   STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);


private:
   // Data for IUnknown
   //
   ULONG          m_cRef;

   // Data for IShellExtInit
   //
   LPDATAOBJECT   m_poData;
};


#endif // __EXTINIT_H__

