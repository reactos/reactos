///////////////////////////////////////////////////////////////////////////////
/*  File: extricon.cpp

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
    03/04/99    Added explicit support for IExtractIconW and         BrianAu
                IExtractIconA.  Was previously only supporting
                IExtractIconW implicitely through UNICODE build.
*/
///////////////////////////////////////////////////////////////////////////////
#include "priv.h"

#include "dbutl.h"
#include "globals.h"
#include "fontext.h"
#include "resource.h"
#include "extricon.h"


#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//
// TrueType/OpenType table tag values.
// Note that MAKETAG macro is defined in winuserp.h
//
static const DWORD TAG_DSIGTABLE = MAKETAG('D','S','I','G');
static const DWORD TAG_CFFTABLE  = MAKETAG('C','F','F',' ');
//
// Required TrueType tables.  This is per the TrueType
// specification at http://www.microsoft.com/typography/tt/ttf_spec
//
static const DWORD TAG_NAMETABLE = MAKETAG('n','a','m','e');
static const DWORD TAG_CMAPTABLE = MAKETAG('c','m','a','p');
static const DWORD TAG_HEADTABLE = MAKETAG('h','e','a','d');
static const DWORD TAG_HHEATABLE = MAKETAG('h','h','e','a');
static const DWORD TAG_HMTXTABLE = MAKETAG('h','m','t','x');
static const DWORD TAG_OS2TABLE  = MAKETAG('O','S','/','2');
static const DWORD TAG_POSTTABLE = MAKETAG('p','o','s','t');
static const DWORD TAG_GLYFTABLE = MAKETAG('g','l','y','f');
static const DWORD TAG_LOCATABLE = MAKETAG('l','o','c','a');
static const DWORD TAG_MAXPTABLE = MAKETAG('m','a','x','p');
//
// "ttcf" isn't really a table.  It's a tag found at the front of 
// a TTC (TrueType Collection) font file.  Treating it like a table
// tag fits well with this scheme.
//
static const DWORD TAG_TTCFILE   = MAKETAG('t','t','c','f');

//
// Helper to swap bytes in a word.
//
inline WORD
SWAP2B(WORD x) 
{
    return ((x << 8) | HIBYTE(x));
}

//
// Template of a TrueType file header.
//
struct TrueTypeFileHdr {
  DWORD dwVersion;
  WORD  uNumTables;
  WORD  uSearchRange;
  WORD  uEntrySelector;
  WORD  uRangeShift;
};

//
// Template of a TrueType table header.
//
struct TrueTypeTableHdr {
  DWORD dwTag;
  DWORD dwCheckSum;
  DWORD dwOffset;
  DWORD dwLength;
};


//-----------------------------------------------------------------------------
// CFontIconHandler
//-----------------------------------------------------------------------------
//
// Path to FONTEXT.DLL.  Only one instance required.
//
TCHAR CFontIconHandler::m_szFontExtDll[];

//
// Initialize the font icon handler object.  This is the object created
// to implement IExtractIcon.  Internally, it creates a type-specific
// handler to handle font file type-specific issues.
//
CFontIconHandler::CFontIconHandler(
    VOID
    ) : m_cRef(0),
        m_pHandler(NULL)
{
    m_szFileName[0] = TEXT('\0');
    //
    // Save the path to FONTEXT.DLL to return in GetIconLocation.
    // This is a static string that should only be initialized once.
    //
    if (TEXT('\0') == m_szFontExtDll[0])
    {
        HINSTANCE hModule = GetModuleHandle(TEXT("FONTEXT.DLL"));
        if (NULL != hModule)
        {
            GetModuleFileName(hModule, m_szFontExtDll, ARRAYSIZE(m_szFontExtDll));
        }
    }

    //
    // Keep DLL in memory as long as this object needs it.
    // Must be done at the end of the ctor in case something in the ctor throws
    // an exception.  The dtor is not called on a partially constructed
    // object.
    //
    InterlockedIncrement(&g_cRefThisDll);
}

CFontIconHandler::~CFontIconHandler(
    VOID
    )
{
    delete m_pHandler;
    //
    // DLL no longer required for this object.
    //
    InterlockedDecrement(&g_cRefThisDll);
}


STDMETHODIMP 
CFontIconHandler::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = NO_ERROR;

    if (NULL != ppvOut)
    {
        *ppvOut = NULL;

        //
        // Explicit casts required because we use multiple inheritance.
        //
        if (IID_IUnknown == riid || IID_IExtractIconW == riid)
        {
            *ppvOut = static_cast<IExtractIconW *>(this);
        }
        else if (IID_IExtractIconA == riid)
        {
            *ppvOut = static_cast<IExtractIconA *>(this);
        }
        else if (IID_IPersistFile == riid)
        {
            *ppvOut = static_cast<IPersistFile *>(this);
        }

        if (NULL == *ppvOut)
            hResult = E_NOINTERFACE;
        else
            ((LPUNKNOWN)*ppvOut)->AddRef();
    }
    else
        hResult = E_POINTER;

    return hResult;
}


STDMETHODIMP_(ULONG) 
CFontIconHandler::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


STDMETHODIMP_(ULONG) 
CFontIconHandler::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}


//
// Implementation of IPersist::GetClassID
//
STDMETHODIMP 
CFontIconHandler::GetClassID(
    CLSID *pClassID
    )
{
    *pClassID = CLSID_FontExt;
    return S_OK;
}


//
// Implementation of IPersistFile::IsDirty
//
STDMETHODIMP 
CFontIconHandler::IsDirty(
    VOID
    )
{
    return E_NOTIMPL;
}

//
//
// Implementation of IPersistFile::Load
//
// This is called by the shell before IExtractIcon::GetIconLocation.
// It gives the extension a chance to save the file name.
//
STDMETHODIMP 
CFontIconHandler::Load(
    LPCOLESTR pszFileName,
    DWORD dwMode            // unused.
    )
{
    HRESULT hr = NOERROR;
    //
    // Save the name of the font file so that the IExtractIcon
    // functions know what file to work with.
    //
#ifdef UNICODE
    lstrcpyn(m_szFileName, pszFileName, ARRAYSIZE(m_szFileName));
#else
    WideCharToMultiByte(CP_ACP, 
                        0, 
                        pszFileName, 
                        -1, 
                        m_szFileName, 
                        ARRAYSIZE(m_szFileName), 
                        NULL, NULL);
#endif
    //
    // Delete any existing type-specific handler.
    //
    delete m_pHandler;
    m_pHandler = NULL;
 
    //
    // Create a new type-specific handler.
    //
    if (NULL == (m_pHandler = IconHandler::Create(m_szFileName)))
        hr = E_FAIL;

    return hr;
}


//
// Implementation of IPersistFile::Save
//
STDMETHODIMP 
CFontIconHandler::Save(
    LPCOLESTR pszFileName,
    BOOL fRemember
    )
{
    return E_NOTIMPL;
}


//
// Implementation of IPersistFile::SaveCompleted
//
STDMETHODIMP 
CFontIconHandler::SaveCompleted(
    LPCOLESTR pszFileName
    )
{
    return E_NOTIMPL;
}


//
// Implementation of IPersistFile::GetCurFile
//
STDMETHODIMP 
CFontIconHandler::GetCurFile(
    LPOLESTR *ppszFileName
    )
{
    return E_NOTIMPL;
}


//
// Implementation of IExtractIconW::Extract
//
STDMETHODIMP 
CFontIconHandler::Extract(
    LPCWSTR pszFileNameW,   // unused
    UINT niconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize          // unused
    )
{
    HICON hiconLarge;
    HICON hiconSmall;

    HRESULT hr = GetIcons(niconIndex, &hiconLarge, &hiconSmall);
    if (SUCCEEDED(hr))
    {
        if (NULL != phiconLarge)
            *phiconLarge = CopyIcon(hiconLarge);
        if (NULL != phiconSmall)
            *phiconSmall = CopyIcon(hiconSmall);
    }

    return SUCCEEDED(hr) ? NO_ERROR     // Use these icons.
                         : S_FALSE;     // Caller must load icons.
}



//
// Implementation of IExtractIconW::GetIconLocation
//
STDMETHODIMP 
CFontIconHandler::GetIconLocation(
    UINT uFlags,        // unused
    LPWSTR pszIconFileW,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags
    )
{
    HRESULT hr      = S_FALSE;
    INT iIconIndex  = GetIconIndex();

    if (-1 != iIconIndex)
    {
        //
        // This is a special case for internal font folder use.
        // Normally, the shell always gives us a pointer to a destination
        // for the path to FONTEXT.DLL.  Since we also use this icon
        // handler in the font folder itself, that code only needs to know
        // the icon index (it already knows the icon is in fontext.dll).
        // This test allows the font folder code to pass NULL and skip
        // the unnecessary string copy.
        // 
        if (NULL != pszIconFileW)
        {
#ifdef UNICODE
            lstrcpynW(pszIconFileW, m_szFontExtDll, cchMax);
#else
            MultiByteToWideChar(CP_ACP,
                                0,
                                m_szFontExtDll,
                                -1,
                                pszIconFileW,
                                cchMax);
#endif
        }
        *pwFlags = GIL_PERINSTANCE;
        *piIndex = iIconIndex;
        hr       = S_OK;
    }

    return hr;
}


//
// Implementation of IExtractIconA::Extract
//
STDMETHODIMP 
CFontIconHandler::Extract(
    LPCSTR pszFileNameA,
    UINT niconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize          // unused
    )
{
    WCHAR szFileNameW[MAX_PATH * 2] = {0};
    MultiByteToWideChar(CP_ACP,
                        0,
                        pszFileNameA,
                        -1,
                        szFileNameW,
                        ARRAYSIZE(szFileNameW));
                            
    return Extract(szFileNameW, niconIndex, phiconLarge, phiconSmall, nIconSize);
}



//
// Implementation of IExtractIconA::GetIconLocation
//
STDMETHODIMP 
CFontIconHandler::GetIconLocation(
    UINT uFlags,        // unused
    LPSTR pszIconFileA,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags
    )
{
    //
    // Call the wide-char version then convert the result to ansi.
    //
    WCHAR szIconFileW[MAX_PATH * 2] = {0};
    HRESULT hr = GetIconLocation(uFlags, 
                                 szIconFileW, 
                                 ARRAYSIZE(szIconFileW), 
                                 piIndex, 
                                 pwFlags);
    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP, 
                            0,
                            szIconFileW,
                            -1,
                            pszIconFileA,
                            cchMax,
                            NULL,
                            NULL);
    }
    return hr;
}


//
// Retrieve the icon index for the font file loaded in Load().
//
INT
CFontIconHandler::GetIconIndex(
    VOID
    )
{
    INT iIconIndex = -1;
    if (NULL != m_pHandler)
    {
        //
        // Call the type-specific icon handler to get the index.
        //
        iIconIndex = m_pHandler->GetIconIndex(m_szFileName);
    }
    return iIconIndex;
}


//
// Retrieve the icon handles for a given icon index.
//
HRESULT
CFontIconHandler::GetIcons(
    UINT iIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall
    )
{
    HRESULT hr = E_FAIL;
    if (NULL != m_pHandler)
    {
        //
        // Call the type-specific icon handler to get the icons.
        //
        hr = m_pHandler->GetIcons(iIconIndex, phiconLarge, phiconSmall);
    }
    return hr;
}


//
// Create a new type-specific icon handler based on the file name extension.
//
IconHandler*
IconHandler::Create(
    LPCTSTR pszFile
    )
{
    IconHandler *pIconHandler = NULL;
    LPCTSTR pszFileExt = pszFile + lstrlen(pszFile) - 3;
    if (pszFileExt > pszFile)
    {
        //
        // Do some quick checks of the first character in the extension before
        // making a call out to lstrcmpi.  Should help perf just a bit.
        //
        bool bCreateHandler = false;
        switch(*pszFileExt)
        {
            case TEXT('t'):
            case TEXT('T'):
                bCreateHandler = (0 == lstrcmpi(pszFileExt, TEXT("TTF")) ||
                                  0 == lstrcmpi(pszFileExt, TEXT("TTC")));
                break;

            case TEXT('O'):
            case TEXT('o'):
                bCreateHandler = (0 == lstrcmpi(pszFileExt, TEXT("OTF")));
                break;

            default:
                break;
        }
        if (bCreateHandler)
        {
            //
            // Filename has either TTF, TTC or OTF extension.
            //
            DWORD dwTables = 0;
            if (TrueTypeIconHandler::GetFileTables(pszFile, &dwTables))
            {
                //
                // Only require the "open type" tables which are a proper subset
                // of the required "true type" tables.
                //
                DWORD dwReqdTables = TrueTypeIconHandler::RequiredOpenTypeTables();
                if (dwReqdTables == (dwTables & dwReqdTables))
                {
                    //
                    // File is a valid TrueType file.
                    //
                    pIconHandler = new TrueTypeIconHandler(dwTables);
                }
            }
        }
    }
    //
    // If new font types are added later, here's where you'll create
    // the handler.
    //

    return pIconHandler;
}

//-----------------------------------------------------------------------------
// TrueTypeIconHandler
//-----------------------------------------------------------------------------
//
// Initialize the OpenType icon handler.
//
TrueTypeIconHandler::TrueTypeIconHandler(
    DWORD dwTables
    ) : m_dwTables(dwTables)
{
    ZeroMemory(m_rghIcons, sizeof(m_rghIcons));
}


TrueTypeIconHandler::~TrueTypeIconHandler(
    void
    )
{
    for (int i = 0; i < ARRAYSIZE(m_rghIcons); i++)
    {
        if (NULL != m_rghIcons[i])
            DestroyIcon(m_rghIcons[i]);
    }
}


//
// Get the icon index for icons to represent a particular TrueType font
// file.  This is where all of the icon identification logic is.
//
INT 
TrueTypeIconHandler::GetIconIndex(
    LPCTSTR pszFile
    )
{
    INT iIconIndex = IDI_TTF;
    if (TABLE_CFF & m_dwTables)
    {
        iIconIndex = IDI_OTFp;
    }
    else if (TABLE_DSIG & m_dwTables)
    {
        iIconIndex = IDI_OTFt;
    }
    else if (TABLE_TTCF & m_dwTables)
    {
        iIconIndex = IDI_TTC;
    }
    return iIconIndex;
}


//
// Retrieve the large and small icons based on the icon index (ID).
//
HRESULT
TrueTypeIconHandler::GetIcons(
    UINT iIconIndex, 
    HICON *phiconLarge, 
    HICON *phiconSmall
    )
{
    HRESULT hr = NO_ERROR;
    int iSmall = -1;
    int iLarge = -1;

    switch(iIconIndex)
    {
        case IDI_TTF:
            iLarge = iICON_LARGE_TT;
            iSmall = iICON_SMALL_TT;
            break;

        case IDI_OTFt:
            iLarge = iICON_LARGE_OTt;
            iSmall = iICON_SMALL_OTt;
            break;

        case IDI_OTFp:
            iLarge = iICON_LARGE_OTp;
            iSmall = iICON_SMALL_OTp;
            break;

        case IDI_TTC:
            iLarge = iICON_LARGE_TTC;
            iSmall = iICON_SMALL_TTC;
            break;

        default:
            hr = E_FAIL;
            break;
    }

    if (-1 != iLarge)
    {
        *phiconLarge = GetIcon(iLarge);
        *phiconSmall = GetIcon(iSmall);
    }

    return hr;
}


//
// Retrieve the icon's handle.  If the icon isn't yet loaded we 
// load it here.  Once it's loaded it stays loaded until the handler
// object is destroyed.  This way we only load icons on demand.
//
HICON
TrueTypeIconHandler::GetIcon(
    int iIcon
    )
{
    HICON hicon = NULL;

    if (0 <= iIcon && ARRAYSIZE(m_rghIcons) > iIcon)
    {
        if (NULL == m_rghIcons[iIcon])
        {
            //
            // Icon hasn't been loaded yet.  Load it.
            //
            // These must be kept in the same order as the iICON_XXXXX enumeration.
            //
            static const struct
            {
                UINT idIcon;
                int  cxcyIcon;

            } rgIconInfo[] = { { IDI_TTF,     32 }, // iICON_LARGE_TT
                               { IDI_TTF,     16 }, // iICON_SMALL_TT
                               { IDI_OTFt,    32 }, // iICON_LARGE_OTt
                               { IDI_OTFt,    16 }, // iICON_SMALL_OTt
                               { IDI_OTFp,    32 }, // iICON_LARGE_OTp
                               { IDI_OTFp,    16 }, // iICON_SMALL_OTp
                               { IDI_TTC,     32 }, // iICON_LARGE_TTC
                               { IDI_TTC,     16 }  // iICON_SMALL_TTC
                             };

            m_rghIcons[iIcon] = (HICON)LoadImage(g_hInst, 
                                     MAKEINTRESOURCE(rgIconInfo[iIcon].idIcon),
                                     IMAGE_ICON,
                                     rgIconInfo[iIcon].cxcyIcon,
                                     rgIconInfo[iIcon].cxcyIcon,
                                     0);
        }
        hicon = m_rghIcons[iIcon];
    }
    return hicon;
}


//
// Determine the index (ID) of the icon for a given TrueType icon file.
// This can also be used by the OpenType handler since TrueType and OpenType
// font files have the same table format.
//
// NOTE: This code does not handle LZ-compressed files like other similar
//       code in the font folder.  The reason is that this code needs to 
//       interrogate only TTF and OTF files which are not compressed.  The
//       font folder must also deal with .TT_ (compressed) files that sometimes
//       come on distribution media.  This icon handler is not required to 
//       display special icons for .TT_ files.  The performance penalty 
//       incurred by using LZxxxxx functions instead of directly mapping
//       files into memory would be significant. [brianau - 6/13/97]
//
BOOL
TrueTypeIconHandler::GetFileTables(
    LPCTSTR pszFile,
    LPDWORD pfTables
    )
{
    *pfTables = 0;

    IconHandler::MappedFile file;

    __try
    {
        //
        // Assumes pszFile points to a TTF or OTF file name (fully qualified).
        //
        if (SUCCEEDED(file.Open(pszFile)))
        {
            LPBYTE pbBase = file.Base();

            TrueTypeFileHdr  *pFileHdr  = (TrueTypeFileHdr *)pbBase;
            if (TAG_TTCFILE == pFileHdr->dwVersion)
            {
                //
                // This icon handler is only interested in what icon is required.
                // Since TTC files have only one icon we don't care about any of the
                // table information.  What we have is all we need.  So basically,
                // if the file has a TTC extension and 'ttcf' is the first 4 bytes
                // in the file, we'll display a TTC icon.
                //
                *pfTables |= (TABLE_TTCF | TrueTypeIconHandler::RequiredTrueTypeTables());
            }
            else
            {
                TrueTypeTableHdr *pTableHdr = (TrueTypeTableHdr *)(pbBase + sizeof(*pFileHdr));
                INT cTables                 = SWAP2B(pFileHdr->uNumTables);

                //
                // Do a sanity check on the table count.
                // This is the same check used in bValidateTrueType (pfiles.cpp).
                //
                if ((0x7FFFF / sizeof(TrueTypeTableHdr)) > cTables)
                {
                    //
                    // Scan the table headers looking for identifying table tags.
                    //
                    for (INT i = 0; i < cTables; i++, pTableHdr++)
                    {
    /*
                        //
                        // Uncomment this to see the tags for each table.
                        //
                        DEBUGMSG((DM_ERROR, TEXT("Table[%d] tag = 0x%08X \"%c%c%c%c\""), 
                               i, pTableHdr->dwTag,
                               pTableHdr->dwTag  & 0x000000FF,
                               (pTableHdr->dwTag & 0x0000FF00) >> 8,
                               (pTableHdr->dwTag & 0x00FF0000) >> 16,
                               (pTableHdr->dwTag & 0xFF000000) >> 24));
    */
                        switch(pTableHdr->dwTag)
                        {
                            case TAG_DSIGTABLE: *pfTables |= TABLE_DSIG; break;
                            case TAG_CFFTABLE:  *pfTables |= TABLE_CFF;  break;
                            case TAG_NAMETABLE: *pfTables |= TABLE_NAME; break;
                            case TAG_CMAPTABLE: *pfTables |= TABLE_CMAP; break;
                            case TAG_HEADTABLE: *pfTables |= TABLE_HEAD; break;
                            case TAG_HHEATABLE: *pfTables |= TABLE_HHEA; break;
                            case TAG_HMTXTABLE: *pfTables |= TABLE_HMTX; break;
                            case TAG_OS2TABLE:  *pfTables |= TABLE_OS2;  break;
                            case TAG_POSTTABLE: *pfTables |= TABLE_POST; break;
                            case TAG_GLYFTABLE: *pfTables |= TABLE_GLYF; break;
                            case TAG_LOCATABLE: *pfTables |= TABLE_LOCA; break;
                            case TAG_MAXPTABLE: *pfTables |= TABLE_MAXP; break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
    }
    __except(FilterGetFileTablesException(GetExceptionCode()))
    {
        //
        // Something in reading the mapped file caused an exception.
        // Probably opened a file that isn't really a font file and it
        // had a bogus table count number.  This can cause us to read 
        // beyond the file mapping.  If this happens, we just set the
        // contents of the table flags return value to 0 indicating that
        // we didn't find any tables in the file.
        //
        *pfTables = 0;    
        DEBUGMSG((DM_ERROR, 
                  TEXT("FONTEXT: Exception occurred reading file %s"), 
                  pszFile));
    }
    return (0 != *pfTables);
}


//
// GetFileTable's response to an exception depends upon the exception
// For debugger-initiated exceptions, continue the search for a handler so
// that the debugger can handle the exception.
// For all others, execute the handler code.
//
INT
TrueTypeIconHandler::FilterGetFileTablesException(
    INT nException
    )
{
    DEBUGMSG((DM_ERROR, TEXT("FONTEXT: Exception Filter: nException = 0x%08X"), nException));
    if (STATUS_SINGLE_STEP == nException ||
        STATUS_BREAKPOINT == nException)
    {
        //
        // Exception generated by debugger.  
        //
        return EXCEPTION_CONTINUE_SEARCH;
    }
    else
    {
        //
        // Exception generated by processing the mapped file.
        //
        return EXCEPTION_EXECUTE_HANDLER;
    }
}


//-----------------------------------------------------------------------------
// IconHandler::MappedFile
//
// A simple encapsulation of opening a mapped file in memory.
// The file is opened with READ access only.
// Client calls Base() to retrieve the base pointer of the mapped file.
//-----------------------------------------------------------------------------
IconHandler::MappedFile::~MappedFile(
    VOID
    )
{
    Close();
}


//
// Close the file mapping and the file.
//
VOID
IconHandler::MappedFile::Close(
    VOID
    )
{
    if (NULL != m_pbBase)
    {
        UnmapViewOfFile(m_pbBase);
        m_pbBase = NULL;
    }
    if (INVALID_HANDLE_VALUE != m_hFileMapping)
    {
        CloseHandle(m_hFileMapping);
        m_hFileMapping = INVALID_HANDLE_VALUE;
    }
    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}


//
// Open the file.  Caller retrieves the base pointer through the
// Base() member function.
//
HRESULT
IconHandler::MappedFile::Open(
    LPCTSTR pszFile
    )
{
    HRESULT hr = NO_ERROR;

    m_hFile = CreateFile(pszFile, 
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL);

    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        if ((m_hFileMapping = CreateFileMapping(m_hFile,
                                                NULL,
                                                PAGE_READONLY,
                                                0,
                                                0,
                                                NULL)) != NULL)
        {
            m_pbBase = (LPBYTE)MapViewOfFile(m_hFileMapping,
                                             FILE_MAP_READ,
                                             0,
                                             0,
                                             0);
            if (NULL == m_pbBase)
            {
                hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NULL, GetLastError());
                DEBUGMSG((DM_ERROR, 
                          TEXT("FONTEXT: Error 0x%08X mapping view of OTF file %s"), 
                          hr, pszFile));
            }
        }
        else
        {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NULL, GetLastError());
            DEBUGMSG((DM_ERROR, 
                      TEXT("FONTEXT: Error 0x%08X creating mapping for OTF file %s"), 
                      hr, pszFile));
        }
    }
    else
    {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NULL, GetLastError());
        DEBUGMSG((DM_ERROR, 
                  TEXT("FONTEXT: Error 0x%08X opening OTF file %s"), 
                  hr, pszFile));
    }
    return hr;
}
