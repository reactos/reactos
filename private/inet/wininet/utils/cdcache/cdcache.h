#define IDM_NEW            100
#define IDM_OPEN           101
#define IDM_SAVE           102
#define IDM_SAVEAS         103
#define IDM_PRINT          104
#define IDM_PRINTSETUP     105
#define IDM_EXIT           106
#define IDM_UNDO           200
#define IDM_CUT            201
#define IDM_COPY           202
#define IDM_PASTE          203
#define IDM_LINK           204
#define IDM_LINKS          205
#define IDM_HELPCONTENTS   300
#define IDM_HELPSEARCH     301
#define IDM_HELPHELP       302
#define IDM_ABOUT          303
#define IDM_HELPTOPICS     304
#define IDM_INSTALL        305
#define IDM_UNINSTALL	   306
#define IDM_SUCCESS		   307
#define IDM_SUCCESS_REMOVE 308
#define IDM_HEADER		   309
#define IDM_FAILED	310
#define IDM_FAILED_REMOVE	311
#define IDM_NEEDIE4WININET 312
#define IDM_ERR_IE4REQFORUNINSTALL	313

#define IDD_MAINAPP			1000
#define IDC_LIST			1001
#define IDC_HEADER			1002
#define IDC_QUESTION		1003

#define IDC_STATIC -1

#define DLG_VERFIRST        400
#define IDC_COMPANY			DLG_VERFIRST
#define IDC_FILEDESC       	DLG_VERFIRST+1
#define IDC_PRODVER         DLG_VERFIRST+2
#define IDC_COPYRIGHT       DLG_VERFIRST+3
#define IDC_OSVERSION       DLG_VERFIRST+4
#define IDC_TRADEMARK       DLG_VERFIRST+5
#define DLG_VERLAST         DLG_VERFIRST+5

#define IDC_LABEL           DLG_VERLAST+1

#define ID_COMPANY			1
#define ID_INFNAME			2
#define ID_APPNAME			3
#define ID_CMDLINE			4

// ============ VALUES ============
#define INI_YES			_T("Yes")
#define INI_NO                 _T("No")
#define INI_TRUE               _T("TRUE")
#define INI_FALSE              _T("FALSE")
#define INI_ON                 _T("ON")
#define INI_OFF                _T("OFF")

typedef struct _INTERNET_CACHE_CONTAINER_INFO_MAX {
    DWORD dwCacheVersion;       // version of software
    LPSTR lpszName;             // embedded pointer to the container name string.
    LPSTR lpszCachePrefix;      // embedded pointer to the container URL prefix
	LPSTR lpszPrefixMap;		// embedded pointer to the container data location
	DWORD dwKBCacheLimit;
	DWORD dwContainerType;
	DWORD dwOptions;
} INTERNET_CACHE_CONTAINER_INFO_MAX, * LPINTERNET_CACHE_CONTAINER_INFO_MAX;


/////////////////////////////////////////////////////////////////////////////
// class CWaitCursor

class CWaitCursor
{
// Construction/Destruction
public:
	CWaitCursor()
	{
		m_hWait = LoadCursor(NULL, IDC_WAIT);
		m_hSave = SetCursor(m_hWait);
	};

	~CWaitCursor()
	{
		SetCursor(m_hSave);	
	};

	HCURSOR	m_hSave;
	HCURSOR m_hWait;

// Operations
public:
	void Restore()
	{
		SetCursor(m_hSave);
	};
};
