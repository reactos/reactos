#ifndef __EXTRICON_H__
#define __EXTRICON_H__
///////////////////////////////////////////////////////////////////////////////
/*  File: extricon.h

    Description: Contains implementation of IExtractIcon for the font folder.
        This code provides icon identification for both TrueType and OpenType
        font files.  The logic used is as follows:
        
            TrueType(1)  DSIG?   CFF?    Icon
            ------------ ------- ------- -----------
            yes          no      no      TT
            yes          no      yes     OTp
            yes          yes     no      OTt
            yes          yes     yes     OTp

        (1) Files must contain required TrueType tables to be considered
            a TrueType font file.

        This icon handler is used by both the shell and the font folder
        to display TrueType and OpenType font icons.  It is designed to be
        easily extensible if support for dynamic icon identification is
        required in other fonts.

        Classes (indentation denotes inheritance):

            CFontIconHandler
            IconHandler
                TrueTypeIconHandler
               

        NOTE:  The design is sort of in a state of limbo right now.  Originally
               the idea was to support two types of OpenType icons along with
               the conventional TrueType and raster font icons.  The OpenType
               icons were OTt and OTp with the 't' and 'p' meaning "TrueType"
               and "PostScript".  Later we decided to only show the icons as
               "OT" without the subscript 't' or 'p'.  The code still distinguishes
               the difference but we just use the same "OT" icon for both the
               OTt and OTp conditions.  Make sense?  Anyway, This OTt and OTp
               stuff may come back at a later date (GDI guys haven't decided)
               so I'm leaving that code in place. [brianau - 4/7/98]

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/13/97    Initial creation.                                    BrianAu
    04/08/98    Removed OpenTypeIconHandler and folded it into       BrianAu
                TrueTypeIconHandler.  There's no need for the 
                separation.  Also added detection of "required"
                TrueType tables.
*/
///////////////////////////////////////////////////////////////////////////////

//
// Pure virtual base class for all types of icon handlers.
//
class IconHandler
{
    public:
        //
        // Simple encapsulation of a mapped file for opening font files.
        //
        class MappedFile
        {
            public:
                MappedFile(VOID)
                    : m_hFile(INVALID_HANDLE_VALUE),
                      m_hFileMapping(INVALID_HANDLE_VALUE),
                      m_pbBase(NULL) { }

                ~MappedFile(VOID);
                HRESULT Open(LPCTSTR pszFile);
                VOID Close(VOID);

                LPBYTE Base(VOID)
                    { return m_pbBase; }

            private:
                HANDLE m_hFile;
                HANDLE m_hFileMapping;
                LPBYTE m_pbBase;

                //
                // Prevent copy.
                //
                MappedFile(const MappedFile& rhs);
                MappedFile& operator = (const MappedFile& rhs);
        };

        virtual ~IconHandler(VOID) { };
        //
        // Derived classes implement this to retrieve the index (ID) of the 
        // desired icon in fontext.dll.
        //
        virtual INT GetIconIndex(LPCTSTR pszFileName) = 0;
        //
        // Derived classes implement this to retrieve the large and small
        // icons corresponding to an index.  The index should be one returned
        // from GetIconIndex().
        //
        virtual HRESULT GetIcons(UINT iIconIndex, HICON *phiconLarge, HICON *phiconSmall) = 0;
        //
        // This static function creates an icon handler object of the proper
        // derived type for the font file extension specified in pszFileExt.
        //
        static IconHandler *Create(LPCTSTR pszFile);
};

/*
//
// This is a template for creating a new type of icon handler for 
// other types of font files. 
//
// To create a new handler:
// 1. Create new handler class from template below.
// 2. Provide implementations for GetIconIndex and GetIcons.
// 3. Load icons in constructor.  See OpenTypeIconHandler as an example.
// 4. Modify IconHandler::Create() to instantiate the new handler type.
//
class XXXXIconHandler : public IconHandler
{
    public:
        XXXXIconHandler(VOID);
        virtual INT GetIconIndex(LPCTSTR pszFileName);
        virtual HRESULT GetIcons(UINT iIconIndex, HICON *phiconLarge, HICON *phiconSmall);

    private:
        static HICON m_hiconLarge;
        static HICON m_hiconSmall;
};
*/    


//
// Icon handler for TrueType font files.
//
class TrueTypeIconHandler : public IconHandler
{
    public:
        TrueTypeIconHandler(DWORD dwTables);
        ~TrueTypeIconHandler(void);

        virtual INT GetIconIndex(LPCTSTR pszFileName);
        virtual HRESULT GetIcons(UINT iIconIndex, HICON *phiconLarge, HICON *phiconSmall);
        //
        // Scans TTF or OTF file identifying tables.
        // Used by TrueTypeIconHandler and any subclasses.
        //
        static BOOL GetFileTables(LPCTSTR pszFile, LPDWORD pfTables);

        enum TABLES { 
                         //
                         // These are the only tables we're interested in.
                         //
                         TABLE_CFF  = 0x00000001,
                         TABLE_DSIG = 0x00000002,
                         TABLE_HEAD = 0x00000004,
                         TABLE_NAME = 0x00000008,
                         TABLE_CMAP = 0x00000010,
                         TABLE_HHEA = 0x00000020,
                         TABLE_HMTX = 0x00000040,
                         TABLE_OS2  = 0x00000080,
                         TABLE_POST = 0x00000100,
                         TABLE_GLYF = 0x00000200,
                         TABLE_MAXP = 0x00000400,
                         TABLE_LOCA = 0x00000800,
                         TABLE_TTCF = 0x00001000  // this is a pseudo table.
                    };

        static DWORD RequiredOpenTypeTables(void)
            { return (TABLE_CMAP |
                      TABLE_HEAD |
                      TABLE_HHEA |
                      TABLE_HMTX |
                      TABLE_MAXP |
                      TABLE_NAME |
                      TABLE_POST |
                      TABLE_OS2); }

        static DWORD RequiredTrueTypeTables(void)
            { return (RequiredOpenTypeTables() |
                      TABLE_GLYF |
                      TABLE_LOCA); }

    protected:
        enum eIcons {iICON_LARGE_TT,
                     iICON_SMALL_TT,
                     iICON_LARGE_OTt,
                     iICON_SMALL_OTt,
                     iICON_LARGE_OTp,
                     iICON_SMALL_OTp,
                     iICON_LARGE_TTC,
                     iICON_SMALL_TTC,
                     MAX_ICONS };

        DWORD m_dwTables;
        HICON m_rghIcons[MAX_ICONS]; // Array of icon handles.

    private:
        static INT FilterGetFileTablesException(INT nException);
        HICON GetIcon(int iIcon);
};


//
// Declaration for the DLL's icon handler.
// This is the object that is instantiated whenever a client asks CLSID_FontExt 
// for IID_IExtractIcon or IID_IPersistFile.
//
class CFontIconHandler : public IExtractIconW, 
                         public IExtractIconA, 
                         public IPersistFile
{
    public:
        CFontIconHandler(VOID);
        ~CFontIconHandler(VOID);

        //
        // IUnknown methods.
        //
        STDMETHODIMP
        QueryInterface(
            REFIID riid,
            LPVOID *ppvOut);

        STDMETHODIMP_(ULONG)
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG)
        Release(
            VOID);

        //
        // IExtractIconW methods.
        //
        STDMETHODIMP Extract(
            LPCWSTR pszFileW,
            UINT nIconIndex,
            HICON *phiconLarge,
            HICON *phiconSmall,
            UINT nIconSize);

        STDMETHODIMP GetIconLocation(
            UINT uFlags,
            LPWSTR szIconFileW,
            UINT cchMax,
            int *piIndex,
            UINT *pwFlags);

        //
        // IExtractIconA methods.
        //
        STDMETHODIMP Extract(
            LPCSTR pszFileA,
            UINT nIconIndex,
            HICON *phiconLarge,
            HICON *phiconSmall,
            UINT nIconSize);

        STDMETHODIMP GetIconLocation(
            UINT uFlags,
            LPSTR szIconFileA,
            UINT cchMax,
            int *piIndex,
            UINT *pwFlags);

        //
        // IPersist methods.
        //
        STDMETHODIMP GetClassID(
            CLSID *pClassID);

        //
        // IPersistFile methods.
        //
        STDMETHODIMP IsDirty(
            VOID);

        STDMETHODIMP Load(
            LPCOLESTR pszFileName,
            DWORD dwMode);

        STDMETHODIMP Save(
            LPCOLESTR pszFileName,
            BOOL fRemember);

        STDMETHODIMP SaveCompleted(
            LPCOLESTR pszFileName);

        STDMETHODIMP GetCurFile(
            LPOLESTR *ppszFileName);

    private:
        LONG         m_cRef;
        TCHAR        m_szFileName[MAX_PATH];    // Name of icon file.
        IconHandler *m_pHandler;                // Ptr to type-specific handler.
        static TCHAR m_szFontExtDll[MAX_PATH];  // Path to FONTEXT.DLL

        INT GetIconIndex(VOID);
        HRESULT GetIcons(UINT iIconIndex, HICON *phiconLarge, HICON *phiconSmall);
};

#endif // __EXTRICON_H__

