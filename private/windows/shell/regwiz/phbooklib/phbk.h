#ifndef _PHBK
#define _PHBK

#include "ccsv.h"
#include "debug.h"

#define DllExportH extern "C" HRESULT WINAPI __stdcall
//#define DllExportH extern "C" __declspec(dllexport) HRESULT WINAPI

#if !defined(WIN16)
#define MsgBox(m,s) MessageBox(g_hWndMain,GetSz(m),GetSz(IDS_TITLE),s)
#endif

#define cbAreaCode	6			// maximum number of characters in an area code, not including \0
#define cbCity 19				// maximum number of chars in city name, not including \0
#define cbAccessNumber 15		// maximum number of chars in phone number, not including \0
#define cbStateName 31 			// maximum number of chars in state name, not including \0
								// check this against state.pbk delivered by mktg
#define cbBaudRate 6			// maximum number of chars in a baud rate, not including \0
#if defined(WIN16)
#define cbDataCenter 12			// max length of data center string
#else
#define cbDataCenter (MAX_PATH+1)			// max length of data center string
#endif
#define NO_AREA_CODE 0xFFFFFFFF

#define PHONE_ENTRY_ALLOC_SIZE	500
#define INF_SUFFIX ".ISP"
#define INF_APP_NAME "ISP INFO"
#define INF_PHONE_BOOK "PhoneBookFile"
#define INF_DEFAULT	"SPAM SPAM SPAM SPAM SPAM SPAM EGGS AND SPAM"
#define STATE_FILENAME "STATE.ICW"
#define FILE_NAME_800950 "800950.DAT"
#define TEMP_BUFFER_LENGTH 1024
#define MAX_INFCODE 9

#define TYPE_SIGNUP_TOLLFREE	0x83
#define TYPE_SIGNUP_TOLL		0x82
#define TYPE_REGULAR_USAGE		0x42

#define MASK_SIGNUP_TOLLFREE	0xB3
#define MASK_SIGNUP_TOLL		0xB3
#define MASK_REGULAR_USAGE		0x73

// 8/13/96 jmazner for Normandy bug #4597
// ported from core\client\phbk 10/15/96
#define MASK_TOLLFREE_BIT			0x01	// Bit #1: 1=tollfree, 0=charge
#define TYPE_SET_TOLLFREE			0x01	// usage: type |= TYPE_SET_TOLLFREE
// want TYPE_SET_TOLL to be a DWORD to match pSuggestInfo->fType
#define TYPE_SET_TOLL				~((DWORD)TYPE_SET_TOLLFREE)    // usage: type &= TYPE_SET_TOLL


#define clineMaxATT	16
#define NXXMin		200
#define NXXMax		999
#define cbgrbitNXX	((NXXMax + 1 - NXXMin) / 8)

// Phone number select dialog flags
//

#define FREETEXT_SELECTION_METHOD  0x00000001
#define PHONELIST_SELECTION_METHOD 0x00000002
#define AUTODIAL_IN_PROGRESS       0x00000004
#define DIALERR_IN_PROGRESS        0x00000008

typedef struct
{
	DWORD	dwIndex;								// index number
	BYTE	bFlipFactor;							// for auto-pick
	DWORD	fType;									// phone number type
	WORD	wStateID;								// state ID
	DWORD	dwCountryID;							// TAPI country ID
	DWORD	dwAreaCode;								// area code or NO_AREA_CODE if none
	DWORD	dwConnectSpeedMin;						// minimum baud rate
	DWORD	dwConnectSpeedMax;						// maximum baud rate
	char	szCity[cbCity + sizeof('\0')];			// city name
	char	szAccessNumber[cbAccessNumber + sizeof('\0')];	// access number
	char	szDataCenter[cbDataCenter + sizeof('\0')];				// data center access string
	char	szAreaCode[cbAreaCode + sizeof('\0')];					//Keep the actual area code string around.
} ACCESSENTRY, far *PACCESSENTRY; 	// ae

typedef struct {
	DWORD dwCountryID;								// country ID that this state occurred in
	PACCESSENTRY paeFirst;							// pointer to first access entry for this state
	char szStateName[cbStateName + sizeof('\0')];	// state name
} STATE, far *LPSTATE;

typedef struct tagIDLOOKUPELEMENT {
	DWORD dwID;
	LPLINECOUNTRYENTRY pLCE;
	PACCESSENTRY pFirstAE;
} IDLOOKUPELEMENT, far *LPIDLOOKUPELEMENT;

typedef struct tagCNTRYNAMELOOKUPELEMENT {
	LPSTR psCountryName;
	DWORD dwNameSize;
	LPLINECOUNTRYENTRY pLCE;
} CNTRYNAMELOOKUPELEMENT, far *LPCNTRYNAMELOOKUPELEMENT;

typedef struct tagIDXLOOKUPELEMENT {
	DWORD dwIndex;
	PACCESSENTRY pAE;
} IDXLOOKUPELEMENT,far *LPIDXLOOKUPELEMENT;

typedef struct tagSUGGESTIONINFO
{
	DWORD	dwCountryID;
	DWORD	wAreaCode;
	DWORD	wExchange;
	WORD	wNumber;
	DWORD	fType;  // 9/6/96 jmazner  Normandy
	DWORD	bMask;  // make this struct look like the one in %msnroot%\core\client\phbk\phbk.h
	PACCESSENTRY *rgpAccessEntry;
} SUGGESTINFO, far *PSUGGESTINFO;

typedef struct tagNPABlock
{
	WORD wAreaCode;
	BYTE grbitNXX [cbgrbitNXX];
} NPABLOCK, far *LPNPABLOCK;



class CPhoneBook
{
	//friend HRESULT DllExport PhoneBookLoad(LPCSTR pszISPCode, DWORD_PTR *pdwPhoneID);
	//friend class CDialog;
	
	// 1/9/96  jmazner Normandy #13185
	//friend class CAccessNumDlg;
	
	friend class CSelectNumDlg;

public:
	void far * operator new( size_t cb ) { return GlobalAlloc(GPTR,cb); };
	void operator delete( void far * p ) {GlobalFree(p); };

	CPhoneBook();
	~CPhoneBook();

	HRESULT Init(LPCSTR pszISPCode);
	HRESULT Merge(LPCSTR pszChangeFilename);
	HRESULT Suggest(PSUGGESTINFO pSuggest);
	HRESULT GetCanonical(PACCESSENTRY pAE, LPSTR psOut);

private:
	PACCESSENTRY			m_rgPhoneBookEntry;
	HANDLE					m_hPhoneBookEntry;	
	DWORD					m_cPhoneBookEntries;
	LPLINECOUNTRYENTRY		m_rgLineCountryEntry;
	LPLINECOUNTRYLIST 		m_pLineCountryList;
	LPIDLOOKUPELEMENT		m_rgIDLookUp;
	LPCNTRYNAMELOOKUPELEMENT m_rgNameLookUp;
	LPSTATE					m_rgState;
	DWORD					m_cStates;
	BOOL              m_bScriptingAvailable;
	char					m_szINFFile[MAX_PATH];
	char					m_szINFCode[MAX_INFCODE];
	char					m_szPhoneBook[MAX_PATH];
	char                    m_szICWDirectoryPath[MAX_PATH];
	// Added on 05/13/97  by Suresh
	// To store the ICW directory Path as it is required by RegWiz
	//
	BOOL ReadPhoneBookDW(DWORD far *pdw, CCSVFile far *pcCSVFile);
	BOOL ReadPhoneBookW(WORD far *pw, CCSVFile far *pcCSVFile);
	BOOL ReadPhoneBookSZ(LPSTR psz, DWORD dwSize, CCSVFile far *pcCSVFile);
	BOOL ReadPhoneBookB(BYTE far *pb, CCSVFile far *pcCSVFile);
	HRESULT ReadOneLine(PACCESSENTRY pAccessEntry, CCSVFile far *pcCSVFile);
	BOOL FixUpFromRealloc(PACCESSENTRY paeOld, PACCESSENTRY paeNew);

};

#ifdef __cplusplus
extern "C" {
#endif
extern HINSTANCE g_hInstDll;	// instance for this DLL
extern HWND g_hWndMain;
#ifdef __cplusplus
}
#endif
#endif // _PHBK

