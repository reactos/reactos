///////////////////////////////////////////////////////////////////////////////
/*  File: extinit.cpp

    Description: Implements IShellExtInit for disk quota shell extensions.



    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "extinit.h"
#include "prshtext.h"
#include "volprop.h"
#include "guidsp.h"

///////////////////////////////////////////////////////////////////////////////
/*  Function: ShellExtInit::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown or
        IShellExtInit interface.  Only IID_IUnknown and
        IID_IShellExtInit are recognized.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NO_ERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    06/25/98    Disabled MMC snapin code.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
ShellExtInit::QueryInterface(
    REFIID riid,
    LPVOID *ppvOut
    )
{
    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    try
    {
        if (IID_IUnknown == riid ||
            IID_IShellExtInit == riid)
        {
            *ppvOut = this;
            ((LPUNKNOWN)*ppvOut)->AddRef();
            hResult = NOERROR;
        }
        else if (IID_IShellPropSheetExt == riid)
        {
            //
            // This can throw OutOfMemory.
            //
            hResult = Create_IShellPropSheetExt(riid, ppvOut);
        }
#ifdef POLICY_MMC_SNAPIN
        else if (IID_ISnapInPropSheetExt == riid)
        {
            hResult = Create_ISnapInPropSheetExt(riid, ppvOut);
        }
#endif // POLICY_MMC_SNAPIN
    }
    catch(CAllocException& e)
    {
        hResult = E_OUTOFMEMORY;
    }
    catch(...)
    {
        hResult = E_UNEXPECTED;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ShellExtInit::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
ShellExtInit::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("ShellExtInit::AddRef, 0x%08X  %d -> %d\n"),
             this, ulReturn - 1, ulReturn));

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ShellExtInit::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
ShellExtInit::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("ShellExtInit::Release, 0x%08X  %d -> %d\n"),
             this, ulReturn + 1, ulReturn));

    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ShellExtInit::Initialize

    Description: Called by the shell to initialize the shell extension.

    Arguments:
        pidlFolder - Pointer to IDL of selected folder.  This NULL for
            property sheet and context menu extensions.

        lpDataObj - Pointer to data object containing list of selected objects.

        hkeyProgID - Registry key for the file object or folder type.

    Returns:
        S_OK    - Success.
        E_FAIL  - Can't initialize extension.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
    06/28/98    Added mounted-volume support.                        BrianAu
                Includes introduction of CVolumeID object.
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
ShellExtInit::Initialize(
    LPCITEMIDLIST pidlFolder,
    LPDATAOBJECT lpDataObj,
    HKEY hkeyProgID)
{
    HRESULT hResult = E_FAIL;
    if (NULL != lpDataObj)
    {
        //
        // First assume it's a normal volume ID (i.e. "C:\").
        // The DataObject will provide CF_HDROP if it is.
        //
        FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM medium;
        bool bMountedVol = false;

        hResult = lpDataObj->GetData(&fe, &medium);
        if (FAILED(hResult))
        {
            //
            // Isn't a normal volume name. Maybe it's a mounted volume.
            // Mounted volume names come in on a different clipboard format
            // so we can treat them differently from normal volume
            // names like "C:\".  A mounted volume name will be the path
            // to the folder hosting the mounted volume.
            // For mounted volumes, the DataObject provides CF "MountedVolume".
            //
            fe.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_MOUNTEDVOLUME);
            hResult     = lpDataObj->GetData(&fe, &medium);
            bMountedVol = SUCCEEDED(hResult);
        }

        if (SUCCEEDED(hResult))
        {
            if (1 == DragQueryFile((HDROP)medium.hGlobal, (DWORD)-1, NULL, 0))
            {
                //
                // Retrieve volume ID string passed in from the shell.
                //
                CString strForParsing; // Used for calling Win32 functions.
                CString strForDisplay; // Used for UI display.
                CString strFSPath;     // Used when an FS path is required.
                DragQueryFile((HDROP)medium.hGlobal,
                              0,
                              strForParsing.GetBuffer(MAX_PATH),
                              MAX_PATH);
                strForParsing.ReleaseBuffer();

                if (!bMountedVol)
                {
                    //
                    // If it's a normal volume name like "C:\", just
                    // use that as the display name and FS Path also.
                    //
                    strFSPath = strForDisplay = strForParsing;
                }
                else
                {
                    //
                    // It's a mounted volume so we need to come up with something
                    // better than "\\?\Volume{ <guid> }\" to display.
                    //
                    // The UI spec says the name shall be like this:
                    //
                    //     <label> (<mounted path>)
                    //
                    TCHAR szMountPtGUID[MAX_PATH] = { TEXT('\0') };
                    GetVolumeNameForVolumeMountPoint(strForParsing,
                                                     szMountPtGUID,
                                                     ARRAYSIZE(szMountPtGUID));

                    TCHAR szLabel[MAX_VOL_LABEL]  = { TEXT('\0') };
                    GetVolumeInformation(szMountPtGUID,
                                         szLabel,
                                         ARRAYSIZE(szLabel),
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         0);
                    //
                    // Format display name as:
                    //
                    // "VOL_LABEL (C:\MountDir)" or
                    // "(C:\MountDir)" if no volume label is available.
                    //
                    // First remove any trailing backslash from the original parsing
                    // string.  It was needed for the call to get the volume mount
                    // point but we don't want to display it in the UI.
                    //
                    if (!strForParsing.IsEmpty())
                    {
                        int iLastBS = strForParsing.Last(TEXT('\\'));
                        if (iLastBS == strForParsing.Length() - 1)
                            strForParsing = strForParsing.SubString(0, iLastBS);
                    }
                    strForDisplay.Format(g_hInstDll,
                                         IDS_FMT_MOUNTEDVOL_DISPLAYNAME,
                                         szLabel,
                                         strForParsing.Cstr());
                    //
                    // Remember the "C:\MountDir" form as the "FSPath".
                    //
                    strFSPath = strForParsing;
                    //
                    // From here on out, the mounted volume GUID string
                    // is used for parsing.
                    //
                    strForParsing = szMountPtGUID;
                }

                //
                // Store the parsing and display name strings in our CVolumeID
                // object for convenient packaging.  This way we can pass around
                // one object and the various parts of the UI can use either the
                // parsable or displayable name as they see fit.  Since the
                // CString objects are reference-counted, all the copying doesn't
                // result in duplication of the actual string contents.
                //
                m_idVolume.SetNames(strForParsing, strForDisplay, strFSPath);

                hResult = S_OK;
            }
            ReleaseStgMedium(&medium);
        }
    }

    return hResult;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ShellExtInit::Create_IShellPropSheetExt

    Description: Creates a shell property sheet extension object and returns
        a pointer to it's IShellPropSheetExt interface.

    Arguments:
        riid - Reference to interface IID.

        ppvOut - Address of interface pointer variable to receive interface
            pointer.

    Returns:
        NO_ERROR        - Success.
        E_FAIL          - Extension initialized with something other
                          than the name of a volume or directory.
                          - OR -
                          The volume or directory doesn't support quotas.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/16/96    Initial creation.                                    BrianAu
    10/07/97    Removed "access denied" and "invalid FS Ver" msgs    BrianAu
                from prop sheet page.  Only display page if
                volume supports quotas and quota control object
                can be initialized.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
ShellExtInit::Create_IShellPropSheetExt(
    REFIID riid,
    LPVOID *ppvOut
    )
{
    HRESULT hResult = E_FAIL;
    DWORD dwFileSysFlags = 0;
    TCHAR szFileSysName[MAX_PATH];

    if (GetVolumeInformation(m_idVolume.ForParsing(), // Volume id str [in]
                             NULL, 0,           // Don't want volume name
                             NULL,              // Don't want serial no.
                             NULL,              // Don't want max comp length.
                             &dwFileSysFlags,   // File system flags.
                             szFileSysName,
                             ARRAYSIZE(szFileSysName)))
    {
        //
        // Only continue if the volume supports quotas.
        //
        if (0 != (dwFileSysFlags & FILE_VOLUME_QUOTAS))
        {
            DiskQuotaPropSheetExt *pSheetExt = NULL;
            try
            {
                pSheetExt = new VolumePropPage;

                //
                // This can throw OutOfMemory.
                //
                hResult = pSheetExt->Initialize(m_idVolume,
                                                IDD_PROPPAGE_VOLQUOTA,
                                                VolumePropPage::DlgProc);
                if (SUCCEEDED(hResult))
                {
                    hResult = pSheetExt->QueryInterface(riid, ppvOut);
                }
            }
            catch(CAllocException& e)
            {
                hResult = E_OUTOFMEMORY;
            }
            catch(...)
            {
                hResult = E_UNEXPECTED;
            }
            if (FAILED(hResult))
            {
                delete pSheetExt;
                *ppvOut = NULL;
            }
        }
    }

    return hResult;
}

#ifdef POLICY_MMC_SNAPIN

HRESULT
ShellExtInit::Create_ISnapInPropSheetExt(
    REFIID riid,
    LPVOID *ppvOut
    )
{
    HRESULT hResult = E_FAIL;
    DiskQuotaPropSheetExt *pSheetExt = NULL;
    try
    {
        pSheetExt = new SnapInVolPropPage;

        hResult = pSheetExt->Initialize(NULL,
                                        IDD_PROPPAGE_POLICY,
                                        VolumePropPage::DlgProc);
        if (SUCCEEDED(hResult))
        {
            hResult = pSheetExt->QueryInterface(riid, ppvOut);
        }
    }
    catch(CAllocException& e)
    {
        hResult = E_OUTOFMEMORY;
    }
    catch(...)
    {
        hResult = E_UNEXPECTED;
    }
    if (FAILED(hResult))
    {
        delete pSheetExt;
        *ppvOut = NULL;
    }
    return hResult;
}

#endif // POLICY_MMC_SNAPIN
