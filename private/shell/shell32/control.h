#ifdef __cplusplus
extern "C" {
#endif

#include <cpl.h>
#include "shell32p.h"

extern TCHAR const c_szCPLCache[];
extern TCHAR const c_szCPLData[];

// Structures used to enumerate CPLs.
//
typedef struct
{
    HDSA    haminst;        // MINST for each loaded dll
    
    HDSA    hamiModule;     // Array of MODULEINFOs of modules in system
    int     cModules;       // size of hamiModule
    
    LPBYTE  pRegCPLBuffer;  // Buffer for hRegCPLs (read from registry)
    HDPA    hRegCPLs;       // Array of RegCPLInfo structs from registry
    int     cRegCPLs;       // size of hRegCPLs
    BOOL    fRegCPLChanged; // TRUE iff hRegCPLs changed
} ControlData, *PControlData;

typedef struct
{
    LPTSTR  pszModule;      // Name of .cpl module
    LPTSTR  pszModuleName;  // points into pszModule to the name sans path
    
    BOOL    flags;          // MI_ flags defined below
    
    FILETIME ftCreationTime;// WIN32_FIND_DATA.ftCreationTime
    DWORD   nFileSizeHigh;  // WIN32_FIND_DATA.nFileSizeHigh
    DWORD   nFileSizeLow;   // WIN32_FIND_DATA.nFileSizeLow
} MODULEINFO, *PMODULEINFO;
// flags:
#define MI_FIND_FILE  1 // WIN32_FIND_FILE info filled in
#define MI_REG_ENUM   2 // Module already enumerated thru registry
#define MI_CPL_LOADED 4 // CPLD_InitModule called for this module

typedef struct tagRegCPLInfo
{
    UINT    cbSize;         // We write the first cbSize bytes of this
    // structure to the registry.  This saves about
    // 250 bytes per structure in the registry.
    BOOL    flags;
    
    // what file does this CPL come from?
    //      UINT    oFileName;      // file name // always 0, so don't need it
    FILETIME ftCreationTime;// WIN32_FIND_DATA.ftCreationTime
    DWORD   nFileSizeHigh;  // WIN32_FIND_DATA.nFileSizeHigh
    DWORD   nFileSizeLow;   // WIN32_FIND_DATA.nFileSizeLow
    
    // what's the display info for this CPL?
    int     idIcon;
    UINT    oName;          // (icon title) short name
    UINT    oInfo;          // (details view) description
    
    // buffer for information
    TCHAR   buf[MAX_PATH +  // oFileName
        32 +                // oName
        64];                // oInfo
} RegCPLInfo;
typedef RegCPLInfo * PRegCPLInfo;
// flags:
#define REGCPL_FROMREG  0x0001  // this RegCPLInfo was loaded from the registry
                                // (used to optimize reading from registry)
// helper defines:
#define REGCPL_FILENAME(pRegCPLInfo) ((pRegCPLInfo)->buf)
#define REGCPL_CPLNAME(pRegCPLInfo) (&((pRegCPLInfo)->buf[(pRegCPLInfo)->oName]))
#define REGCPL_CPLINFO(pRegCPLInfo) (&((pRegCPLInfo)->buf[(pRegCPLInfo)->oInfo]))

// Information about control modules and individual controls
//
typedef struct // cpli
{
    int     idControl;      // control index
    HICON   hIcon;          // handle of icon
    int     idIcon;         // ID of the icon (used for links)
    LPTSTR  pszName;        // ptr to name string
    LPTSTR  pszInfo;        // ptr to info string
    LPTSTR  pszHelpFile;    // help file
    LONG_PTR lData;         // user supplied data
    DWORD   dwContext;      // help context
} CPLITEM, *LPCPLITEM;

typedef struct // minst
{
    BOOL        fIs16bit;
#ifdef WX86
    BOOL        fIsX86Dll;      // TRUE if dll is an x86 dll
#endif
    HINSTANCE   hinst;          // either a 16 or 32 bit HINSTANCE (fIs16bit)
    DWORD       idOwner;        // process id of owner (system unique)
    HANDLE      hOwner;         // keeps id valid (against reuse)
} MINST;

typedef struct // cplm
{
    int             cRef;
    MINST           minst;
    TCHAR           szModule[MAXPATHLEN];
    union
    {
        FARPROC16   lpfnCPL16;      // minst.fIs16bit=TRUE
        APPLET_PROC lpfnCPL32;      // minst.fIs16bit=FALSE
        FARPROC     lpfnCPL;        // for opaque operation
    };
    HDSA            hacpli;         // array of CPLITEM structs
} CPLMODULE, *PCPLMODULE, *LPCPLMODULE;

//
//  These values are larger than the NEWCPLINFO structure because some
//  languages (German, for example) can't fit "Scanners and Cameras" into
//  just 32 characters of NEWCPLINFO.szName, and even in English,
//  you can't fit very much helptext into only 64 characters of
//  NEWCPLINFO.szInfo.  (The Network applet writes a small novel for its
//  helptext.)
//
#define MAX_CCH_CPLNAME     MAX_PATH    // Arbitrary
#define MAX_CCH_CPLINFO     512         // Arbitrary


LRESULT CPL_CallEntry(LPCPLMODULE, HWND, UINT, LPARAM, LPARAM);

void CPL_StripAmpersand(LPTSTR szBuffer);
BOOL CPL_Init(HINSTANCE hinst);
int _FindCPLModuleByName(LPCTSTR pszModule);

LPCPLMODULE CPL_LoadCPLModule(LPCTSTR szModule);
int CPL_FreeCPLModule(LPCPLMODULE pcplm);

void CPLD_Destroy(PControlData lpData);
BOOL CPLD_GetModules(PControlData lpData);
void CPLD_GetRegModules(PControlData lpData);
int CPLD_InitModule(PControlData lpData, int nModule, MINST *lphModule);
void CPLD_AddControlToReg(PControlData lpData, const MINST * pminst, int nControl);
void CPLD_FlushRegModules(PControlData lpData);

STDAPI_(int) MakeCPLCommandLine(LPCTSTR pszModule, LPCTSTR pszName, LPTSTR pszCommandLine, DWORD cchCommandLine);

HRESULT CALLBACK CControls_DFMCallBackBG(LPSHELLFOLDER psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);
IShellFolderViewCB* Controls_CreateSFVCB(IShellFolder* psf, LPCITEMIDLIST pidl);

LPCPLMODULE FindCPLModule(const MINST * pminst);

#ifdef __cplusplus
};
#endif
