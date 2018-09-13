#ifndef _INC_DSKQUOTA_PRSHTEXT_H
#define _INC_DSKQUOTA_PRSHTEXT_H
///////////////////////////////////////////////////////////////////////////////
/*  File: prshtext.h

    Description: DSKQUOTA property sheet extention declaration.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    06/25/98    Disabled snapin code with #ifdef POLICY_MMC_SNAPIN.  BrianAu
                Switching to ADM-file approach to entering policy
                data.  Keeping snapin code available in case
                we decide to switch back at a later time.
    06/27/98    Added support for mounted volumes.                   BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_H
#   include "dskquota.h"
#endif

#ifndef _INC_DSKQUOTA_POLICY_H
#   include "policy.h"
#endif

//
// Base class for all DiskQuotaControl property sheet extensions.
//
class DiskQuotaPropSheetExt : public IShellPropSheetExt
{
    private:
        LONG               m_cRef;
        DWORD              m_dwDlgTemplateID;
        DLGPROC            m_lpfnDlgProc;

        static UINT CALLBACK 
        DiskQuotaPropSheetExt::PropSheetPageCallback(
            HWND hwnd,	
            UINT uMsg,	
            LPPROPSHEETPAGE ppsp);

#ifdef POLICY_MMC_SNAPIN
        //
        // This base class can't create a disk quota policy object.
        // Defer to the policy prop page derived class.
        //
        virtual HRESULT CreateDiskQuotaPolicyObject(IDiskQuotaPolicy **ppOut)
            { return E_NOINTERFACE; }
#endif

        //
        // Prevent copying.
        //
        DiskQuotaPropSheetExt(const DiskQuotaPropSheetExt&);
        DiskQuotaPropSheetExt& operator = (const DiskQuotaPropSheetExt&);

    protected:
        CVolumeID          m_idVolume;
        HPROPSHEETPAGE     m_hPage;
        PDISKQUOTA_CONTROL m_pQuotaControl;
        INT                m_cOleInitialized;

        //
        // Subclasses can act on these notifications if they wish.
        // These are called from PropSheetPageCallback().
        //
        virtual UINT OnPropSheetPageCreate(LPPROPSHEETPAGE ppsp) 
            { return 1; }
        virtual VOID OnPropSheetPageRelease(LPPROPSHEETPAGE ppsp) { }

        HRESULT GetQuotaController(IDiskQuotaControl **ppqc);


    public:
        DiskQuotaPropSheetExt(VOID);
  
        //
        // Need to call subclass destructor when Release() 
        // destroys "this".
        //
        virtual ~DiskQuotaPropSheetExt(VOID);

        HRESULT Initialize(const CVolumeID& idVolume, 
                           DWORD dwDlgTemplateID,
                           DLGPROC lpfnDlgProc);

        //
        // IUnknown methods.
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID, 
            LPVOID *);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // IShellPropSheetInit methods.
        //
        STDMETHODIMP
        AddPages(
            LPFNADDPROPSHEETPAGE lpfnAddPage,
            LPARAM lParam);

        STDMETHODIMP
        ReplacePage(
            UINT uPageID,
            LPFNADDPROPSHEETPAGE lpfnAddPage,
            LPARAM lParam)
                { return E_NOTIMPL; }
};

#endif // _INC_DSKQUOTA_PRSHTEXT_H

