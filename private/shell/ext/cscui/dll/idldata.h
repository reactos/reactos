//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       idldata.h
//
//--------------------------------------------------------------------------

#ifndef _INC_SHELL_IDLDATA_H
#define _INC_SHELL_IDLDATA_H

#include "fmtetc.h"

//
// Clipboard Format for IDLData object.
//
#define ICFHDROP                        0
#define ICFFILENAME                     1
#define ICFNETRESOURCE                  2
#define ICFFILECONTENTS                 3
#define ICFFILEGROUPDESCRIPTORA         4
#define ICFFILENAMEMAPW                 5
#define ICFFILENAMEMAP                  6
#define ICFHIDA                         7
#define ICFOFFSETS                      8
#define ICFPRINTERFRIENDLYNAME          9
#define ICFPRIVATESHELLDATA             10
#define ICFHTML                         11
#define ICFFILENAMEW                    12
#define ICFFILEGROUPDESCRIPTORW         13
#define ICFPREFERREDDROPEFFECT          14
#define ICFPERFORMEDDROPEFFECT          15
#define ICFLOGICALPERFORMEDDROPEFFECT   16
#define ICFSHELLURL                     17
#define ICFINDRAGLOOP                   18
#define ICF_DRAGCONTEXT                 19
#define ICF_TARGETCLSID                 20
#define ICF_MAX                         21

#define g_cfNetResource                 CIDLData::m_rgcfGlobal[ICFNETRESOURCE]
#define g_cfHIDA                        CIDLData::m_rgcfGlobal[ICFHIDA]
#define g_cfOFFSETS                     CIDLData::m_rgcfGlobal[ICFOFFSETS]
#define g_cfPrinterFriendlyName         CIDLData::m_rgcfGlobal[ICFPRINTERFRIENDLYNAME]
#define g_cfFileName                    CIDLData::m_rgcfGlobal[ICFFILENAME]
#define g_cfFileContents                CIDLData::m_rgcfGlobal[ICFFILECONTENTS]
#define g_cfFileGroupDescriptorA        CIDLData::m_rgcfGlobal[ICFFILEGROUPDESCRIPTORA]
#define g_cfFileGroupDescriptorW        CIDLData::m_rgcfGlobal[ICFFILEGROUPDESCRIPTORW]
#define g_cfFileNameMapW                CIDLData::m_rgcfGlobal[ICFFILENAMEMAPW]
#define g_cfFileNameMapA                CIDLData::m_rgcfGlobal[ICFFILENAMEMAP]
#define g_cfPrivateShellData            CIDLData::m_rgcfGlobal[ICFPRIVATESHELLDATA]
#define g_cfHTML                        CIDLData::m_rgcfGlobal[ICFHTML]
#define g_cfFileNameW                   CIDLData::m_rgcfGlobal[ICFFILENAMEW]
#define g_cfPreferredDropEffect         CIDLData::m_rgcfGlobal[ICFPREFERREDDROPEFFECT]
#define g_cfPerformedDropEffect         CIDLData::m_rgcfGlobal[ICFPERFORMEDDROPEFFECT]
#define g_cfLogicalPerformedDropEffect  CIDLData::m_rgcfGlobal[ICFLOGICALPERFORMEDDROPEFFECT]
#define g_cfShellURL                    CIDLData::m_rgcfGlobal[ICFSHELLURL]
#define g_cfInDragLoop                  CIDLData::m_rgcfGlobal[ICFINDRAGLOOP]
#define g_cfDragContext                 CIDLData::m_rgcfGlobal[ICF_DRAGCONTEXT]
#define g_cfTargetCLSID                 CIDLData::m_rgcfGlobal[ICF_TARGETCLSID]

// Most places will only generate one so minimize the number of changes in the code (bad idea!)
#ifdef UNICODE
#define g_cfFileNameMap         g_cfFileNameMapW
#else
#define g_cfFileNameMap         g_cfFileNameMapA
#endif

class CIDLData : public IDataObject
{
    public:
        CIDLData(LPCITEMIDLIST pidlFolder, 
                 UINT cidl, 
                 LPCITEMIDLIST *apidl, 
                 IShellFolder *psfOwner = NULL,
                 IDataObject *pdtInner = NULL);

        virtual ~CIDLData(void);

        //
        // IUnknown methods.
        //
        STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        //
        // IDataObject methods.
        //
        STDMETHODIMP GetData(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
        STDMETHODIMP GetDataHere(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
        STDMETHODIMP QueryGetData(FORMATETC *pFmtEtc);
        STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pFmtEtcIn, FORMATETC *pFmtEtcOut);
        STDMETHODIMP SetData(FORMATETC *pFmtEtc, STGMEDIUM *pstm, BOOL fRelease);
        STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum);
        STDMETHODIMP DAdvise(FORMATETC *pFmtEtc, DWORD grfAdv, LPADVISESINK pAdvSink, DWORD *pdwConnection);
        STDMETHODIMP DUnadvise(DWORD dwConnection);
        STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *ppEnum);

        static HRESULT CreateInstance(IDataObject **ppOut,
                                      LPCITEMIDLIST pidlFolder,
                                      UINT cidl,
                                      LPCITEMIDLIST *apidl,
                                      IShellFolder *psfOwner = NULL,
                                      IDataObject *pdtInner = NULL);

        static HRESULT CreateInstance(CIDLData **ppOut,
                                      LPCITEMIDLIST pidlFolder,
                                      UINT cidl,
                                      LPCITEMIDLIST *apidl,
                                      IShellFolder *psfOwner = NULL,
                                      IDataObject *pdtInner = NULL);

        void InitializeClipboardFormats(void);

        HRESULT Clone(UINT *acf, UINT ccf, IDataObject **ppdtobjOut);

        HRESULT CloneForMoveCopy(IDataObject **ppdtobjOut);

        BOOL IsOurs(IDataObject *pdtobj) const;

        HRESULT CtorResult(void) const
            { return m_hrCtor; }

        virtual IShellFolder *GetFolder(void) const;

    protected:
        //
        // These are defined for compatibility with the original shell code.
        //
        enum { MAX_FORMATS = ICF_MAX };

        LONG          m_cRef;
        IShellFolder *m_psfOwner;
        DWORD         m_dwOwnerData;
        HRESULT       m_hrCtor;
        IDataObject  *m_pdtobjInner;
        FORMATETC     m_rgFmtEtc[MAX_FORMATS];
        STGMEDIUM     m_rgMedium[MAX_FORMATS];
        bool          m_bEnumFormatCalled;

        static CLIPFORMAT m_rgcfGlobal[ICF_MAX];
        static const LARGE_INTEGER m_LargeIntZero;

        virtual HRESULT ProvideFormats(CEnumFormatEtc *pEnumFormatEtc);

    private:
        typedef HGLOBAL HIDA;
        HIDA HIDA_Create(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl);

        //
        // Prevent copy.
        //
        CIDLData(const CIDLData& rhs);
        CIDLData& operator = (const CIDLData& rhs);
};

#endif _INC_SHELL_IDLDATA_H

