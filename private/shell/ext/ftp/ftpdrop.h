/*****************************************************************************\
    FILE: ftpdrop.h - IDropTarget interface

    Remarks:

    Note that you cannot create a shortcut on an FTP site.  Although
    there's nothing technically preventing it, it's not done because
    the shortcut won't be of much use on an FTP site.  (It points to
    your local machine, which doesn't help much for people not on the
    same network!)

    If you really want to put a shortcut file on an FTP site, create
    it on the desktop, then drag the shortcut onto the FTP site.

    The default verb for FTP sites is always "Copy".  This is true
    even if an intra-site drag-drop is being done.

    DESCRIPTION:
        DefView will cache the IDropTarget pointer (CFtpDrop) for a shell extension.
    When it calls CFtpDrop::Drop(), the work needs to be done on a background
    thread in order to not block the UI thread.  The problem is that if the user
    does another drag to the same Ftp Window, CFtpDrop::Drop() will be called again.
    For this reasons, CFtpDrop::Drop() cannot have any state after it returns.
    In order to accomplish this with the asynch background thread, we have
    CFtpDrop::Drop() call CDropOperation_Create(), and then CDropOperation->DoOperation().
    And then it will orphan (call Release()) the CDropOperation.  The CDropOperation
    will then destroy itself when the copy is finishes.  This enables subsequent calls
    to CFtpDrop::Drop() to spawn separate CDropOperation objects so each can maintain
    the state for that specifc operation and CFtpDrop remains stateless.
\*****************************************************************************/

#ifndef _FTPDROP_H
#define _FTPDROP_H

#include "util.h"

typedef enum OPS
{                           // Overwrite prompt status
    opsPrompt,              // Prompt each collision
    opsYesToAll,            // Overwrite unconditionally
    opsNoToAll,             // Never overwrite
    opsCancel,              // Stop the operation
} OPS;

#define COHDI_FILESIZE_COUNT            5
#define COHDI_COPY_FILES                6

typedef struct tagCOPYONEHDROPINFO
{
    CFtpFolder * pff;
    LPCWSTR pszFSSource;
    LPCWSTR pszFtpDest;
    LPCWSTR pszDir;
    DWORD dwOperation;                  // COHDI_FILESIZE_COUNT, COHDI_COPY_FILES, etc.
    OPS ops;
    BOOL fIsRoot;
    CMultiLanguageCache * pmlc;
    LPITEMIDLIST pidlServer;
    BOOL fFireChangeNotify;     // Don't fire change notify on BrowserOnly when replacing a file.
    PROGRESSINFO progInfo;
} COPYONEHDROPINFO, * LPCOPYONEHDROPINFO;

typedef HRESULT (*STREAMCOPYPROC)(IStream * pstm, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, LPVOID pv, ULARGE_INTEGER *pqw);

#ifdef UNICODE
#define _EnumOneHdrop          _EnumOneHdropW
#else // UNICODE
#define _EnumOneHdrop          _EnumOneHdropA
#endif // UNICODE


/*****************************************************************************\

    CFtpDrop

    de and pde are rather gnarly.

    pde is where the output drop effect is kept.  A drop handler
    can force the effect to be DROPEFFECT_COPY if it encounters a
    situation that indicates that the source shouldn't be deleted.
    (E.g., if a file is not copied due to an illegal file name.)

    de is where the current effect is kept.  A drop handler
    should consult de to decide whether or not the source
    should be deleted.  (Used by the HDROP handler, because it is
    the drop target's responsibility to delete the source of an
    HDROP if applicable.)

    You should read from de and write to *pde.  Don't
    write to de or read from *pde.

    The overwrite prompting state tells us how to handle
    the case where an incoming file collides with an existing file.

    DESCRIPTION:
        DefView will cache the IDropTarget pointer (CFtpDrop) for a shell extension.
    When it calls CFtpDrop::Drop(), the work needs to be done on a background
    thread in order to not block the UI thread.  The problem is that if the user
    does another drag to the same Ftp Window, CFtpDrop::Drop() will be called again.
    For this reasons, CFtpDrop::Drop() cannot have any state after it returns.
    In order to accomplish this with the asynch background thread, we have
    CFtpDrop::Drop() call CDropOperation_Create(), and then CDropOperation->DoOperation().
    And then it will orphan (call Release()) the CDropOperation.  The CDropOperation
    will then destroy itself when the copy is finishes.  This enables subsequent calls
    to CFtpDrop::Drop() to spawn separate CDropOperation objects so each can maintain
    the state for that specifc operation and CFtpDrop remains stateless.
\*****************************************************************************/
class CFtpDrop          : public IDropTarget
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IDropTarget ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

public:
    CFtpDrop();
    ~CFtpDrop(void);

    // Public Member Functions
    DWORD GetEffectsAvail(IDataObject * pdto);
    STDMETHODIMP EnumHdrop(HGLOBAL hdrop, HGLOBAL hmap, BOOL fCopy);

    static HRESULT _CopyHglobal(IStream * pstm, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, LPVOID pvSrc, ULARGE_INTEGER *pqw);
    static HRESULT CopyStream(IStream * pstm, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, LPVOID pvSrc, ULARGE_INTEGER *pqw);
    

    // Friend Functions
    friend HRESULT CFtpDrop_Create(CFtpFolder * pff, HWND hwnd, CFtpDrop ** ppfm);

protected:
    // Protected Member Variables
    int                     m_cRef;

    CFtpFolder *            m_pff;          // The owner
    CFtpDir *               m_pfd;          // The FtpDir of the owner
    HWND                    m_hwnd;         // The window being drug over

    DWORD                   m_grfks;        // Last grfKeyState seen
    DWORD                   m_grfksAvail;   // Effects available
    DROPEFFECT              m_de;           // Effect being performed
    DROPEFFECT *            m_pde;          // Output effect
    OPS                     m_ops;          // Overwrite prompting state
    int                     m_cobj;         // Number of objects being dropped

    // Private Member Functions
    HRESULT SetEffect(DROPEFFECT * pde);
    HRESULT _InvokePaste(LPCMINVOKECOMMANDINFO pici);
    BOOL _HasData(IDataObject * pdto, FORMATETC * pformatetc) { return (S_OK == pdto->QueryGetData(pformatetc)); };
    DROPEFFECT GetEffect(POINTL pt);
    HRESULT CopyStorage(LPCTSTR pszFile, IStorage * pstgIn);
    HRESULT CopyHdrop(IDataObject * pdto, STGMEDIUM *psm);
    HRESULT CopyAsStream(LPCTSTR pszName, DWORD dwFileAttributes, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, STREAMCOPYPROC pfn, LPVOID pv);
    HRESULT CopyFCont(LPCTSTR pszName, DWORD dwFileAttributes, DWORD dwFlags, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, STGMEDIUM *psm);
    HRESULT CopyFGD(IDataObject * pdto, STGMEDIUM *psm, BOOL fUnicode);
    HRESULT _Copy(IDataObject * pdto);
    HRESULT _CopyOneHdrop(LPCTSTR pszFSSource, LPCTSTR pszFtpDest, IProgressDialog * ppd);

    HRESULT _StartBackgroundInteration(void);
    HRESULT _DoCountIteration(void);
    HRESULT _GetFSSourcePaths(HGLOBAL hdrop, BOOL * pfAnsi);
    HRESULT _GetFtpDestPaths(HGLOBAL hmap, BOOL fAnsi);

private:
    // Private Member Variables
    LPCTSTR                 m_pszzFSSource;         // Paths
    LPCTSTR                 m_pszzFtpDest;              // Map


    HRESULT _CalcSizeOneHdrop(LPCWSTR pszFSSource, LPCWSTR pszFtpDest, IProgressDialog * ppd);
    HRESULT _GetFileDescriptor(LONG nIndex, LPFILEGROUPDESCRIPTORW pfgdW, LPFILEGROUPDESCRIPTORA pfgdA, BOOL fUnicode, LPFILEDESCRIPTOR pfd);
    HRESULT _CreateFGDDirectory(LPFILEDESCRIPTOR pfd);
    BOOL _IsFTPOperationAllowed(IDataObject * pdto);
    CFtpDir * _GetRelativePidl(LPCWSTR pszFullPath, DWORD dwFileAttributes, DWORD dwFileSizeHigh, DWORD dwFileSizeLow, LPITEMIDLIST * ppidl);
};

#endif // _FTPDROP_H
