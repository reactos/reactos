#ifndef _INC_DSKQUOTA_EXTINIT_H
#define _INC_DSKQUOTA_EXTINIT_H
///////////////////////////////////////////////////////////////////////////////
/*  File: extinit.h

    Description: Contains declarations for disk quota shell extensions.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef _INC_DSKQUOTA_UTILS_H
#   include "utils.h"
#endif

#ifndef _INC_DSKQUOTA_STRCLASS_H
#   include "strclass.h"
#endif

class ShellExtInit : public IShellExtInit
{
    private:
        LONG      m_cRef;
        CVolumeID m_idVolume; // Contains strings for parsing and display.

        HRESULT Create_IShellPropSheetExt(REFIID riid, LPVOID *ppvOut);
        HRESULT Create_ISnapInPropSheetExt(REFIID riid, LPVOID *ppvOut);

    public:
        ShellExtInit(VOID)
            : m_cRef(0) { }

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
        // IShellExtInit methods.
        //
        STDMETHODIMP
        Initialize(
            LPCITEMIDLIST pidlFolder,
            IDataObject *pDataObj,
            HKEY hkeyProgID);
};



#endif // _INC_DSKQUOTA_EXTINIT_H
