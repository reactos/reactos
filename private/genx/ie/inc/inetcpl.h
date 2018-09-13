//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1998               **
//*********************************************************************

#ifndef _INETCPLP_H_
#define _INETCPLP_H_

// property sheet page IDs
#define INET_PAGE_GENERAL       0x00000001
#define INET_PAGE_CONNECTION    0x00000002
#define INET_PAGE_PLACES        0x00000004      // OBSOLETE: IE40 users! DO NOT use this ID
#define INET_PAGE_PROGRAMS      0x00000008
#define INET_PAGE_SECURITY_OLD  0x00000010      // OBSOLETE: IE40 users! DO NOT use this ID
#define INET_PAGE_ADVANCED      0x00000020
#define INET_PAGE_PRINT         0x00000040      // OBSOLETE: IE40 users! DO NOT use this ID
#define INET_PAGE_CONTENT       0x00000080
#define INET_PAGE_SECURITY      0x00000100
#define INET_PAGE_ASSOC         0x00000200      // UNIX Assocations
#define INET_PAGE_ALIAS         0x00000400      // UNIX Aliases


//  restrict flags
#define R_MULTIMEDIA    0x00000001              // OBSOLETE: IE40 users! DO NOT use this ID
#define R_COLORS        0x00000002              // colors section of the Colors Dialog
#define R_LINKS         0x00000004              // links section of the Colors Dialog
#define R_TOOLBARS      0x00000008              // OBSOLETE: IE40 users! DO NOT use this ID
#define R_FONTS         0x00000010              // Fonts Dialog
#define R_DIALING       0x00000020              // Connection section of Connection tab (incl Settings subdialog)
#define R_PROXYSERVER   0x00000040              // Proxy server section of Connection tab (incl Advanced subdialog)
#define R_CUSTOMIZE     0x00000080              // Homepage section of General tab
#define R_HISTORY       0x00000100              // History section of General tab
#define R_MAILANDNEWS   0x00000200              // Messaging section of Programs tab
#define R_VIEWERS       0x00000400              // OBSOLETE: IE40 users! DO NOT use this ID
#define R_RATINGS       0x00000800              // Ratings section of Content tab
#define R_CERTIFICATES  0x00001000              // Certificates section of Content tab
#define R_ACTIVECONTENT 0x00002000              // OBSOLETE: IE40 users! DO NOT use this ID
#define R_WARNINGS      0x00004000              // OBSOLETE: IE40 users! DO NOT use this ID
#define R_CACHE         0x00008000              // Temporary Internet Files section of General Tab (incl Settings subdialog)
#define R_CRYPTOGRAPHY  0x00010000              // OBSOLETE: IE40 users! DO NOT use this ID
#define R_PLACESDEFAULT 0x00020000              // OBSOLETE: IE40 users! DO NOT use this ID
#define R_OTHER         0x00040000              // OBSOLETE: IE40 users! DO NOT use this ID
#define R_CHECKBROWSER  0x00080000              // "IE should check if default browser" checkbox on Programs tab
#define R_LANGUAGES     0x00100000              // Languages Dialog off of the General tab
#define R_ACCESSIBILITY 0x00200000              // Accessibility Dialog off of the General tab
#define R_SECURITY_HKLM_ONLY 0x00400000         // Security tab settings (everything is read only)
#define R_SECURITY_CHANGE_SETTINGS 0x00800000   // Security tab settings (can't change security level for a zone)
#define R_SECURITY_CHANGE_SITES 0x01000000      // Security tab settings (disable everything on Add sites)
#define R_PROFILES      0x02000000              // Profile Asst. section of Content tab
#define R_WALLET        0x04000000              // MS Wallet section of Content tab
#define R_CONNECTION_WIZARD 0x08000000          // Connection wizard button on Connection tab
#define R_AUTOCONFIG    0x10000000              // Auto config section of Programs tab
#define R_ADVANCED      0x20000000              // Entire Advanced tab (including "Restore Defaults")
#define R_CAL_CONTACT   0x40000000              // Personal Info section of Programs tab

#define STR_INETCPL TEXT("inetcpl.cpl") // LoadLibrary() with this string

// structure to pass info to the control panel
typedef struct {
    UINT cbSize;                    // size of the structure
    DWORD dwFlags;                  // enabled page flags (remove pages)
    LPSTR pszCurrentURL;            // the current URL (NULL=none)
    DWORD dwRestrictMask;           // disable sections of the control panel
    DWORD dwRestrictFlags;          // masking for the above
} IEPROPPAGEINFO, *LPIEPROPPAGEINFO;

// GetProcAddress() with this string
#define STR_ADDINTERNETPROPSHEETS "AddInternetPropertySheets"

typedef HRESULT (STDMETHODCALLTYPE * PFNADDINTERNETPROPERTYSHEETS)(
    LPFNADDPROPSHEETPAGE pfnAddPage,   // add PS callback function
    LPARAM lparam,                     // pointer to prop. sheet header
    PUINT pucRefCount,                 // reference counter (NULL if not used)
    LPFNPSPCALLBACK pfnCallback        // PS-to-be-added's callback function (NULL if not used);
);


// GetProcAddress() with this string
#define STR_ADDINTERNETPROPSHEETSEX "AddInternetPropertySheetsEx"

typedef HRESULT (STDMETHODCALLTYPE * PFNADDINTERNETPROPERTYSHEETSEX)(
    LPFNADDPROPSHEETPAGE pfnAddPage, // add PS callback function
    LPARAM lparam,                   // pointer to prop. sheet header
    PUINT pucRefCount,               // reference counter (NULL if not used)
    LPFNPSPCALLBACK pfnCallback,     // PS-to-be-added's callback function (NULL if not used)
    LPIEPROPPAGEINFO piepi           // structure to pass info to control panel
);

STDAPI_(INT_PTR) OpenFontsDialog(HWND hDlg, LPCSTR lpszKeyPath);
STDAPI_(BOOL) LaunchSecurityDialogEx(HWND hDlg, DWORD dwZone, DWORD dwFlags);

#define STR_LAUNCHSECURITYDIALOGEX TEXT("LaunchSecurityDialogEx")

// Flags understood by LaunchSecurityDialog
typedef enum {
    LSDFLAG_DEFAULT    = 0x00000000,
    LSDFLAG_NOADDSITES = 0x00000001,
    LSDFLAG_FORCEUI    = 0x00000002
} LSDFLAG;

typedef BOOL (STDMETHODCALLTYPE * PFNLAUNCHSECURITYDIALOGEX)(
    HWND        hDlg,    // Parent Window
    DWORD       dwZone,  // Initial Zone to display, as defined in urlmon
    DWORD       dwFlags // Initialization flags: or'd combination of LSD_FLAGS
);

#endif

