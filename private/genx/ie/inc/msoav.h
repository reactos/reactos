#pragma once

#ifndef _MSOAV_H
#define _MSOAV_H

typedef struct _msoavinfo
{
int cbsize;			//size of this struct
struct {
	ULONG fPath:1;			//when true use pwzFullPath else use lpstg
	ULONG fReadOnlyRequest:1;	//user requests file to be opened read/only
	ULONG fInstalled:1;	//the file at pwzFullPath is an installed file
	ULONG fHttpDownload:1;	//the file at pwzFullPath is a temp file downloaded from http/ftp
	};
HWND hwnd;			//parent window of the Office9 app
union {
	WCHAR *pwzFullPath;	//full path to the file about to be opened
	LPSTORAGE lpstg;	//OLE Storage of the doc about to be opened
	}u;
WCHAR *pwzHostName;	 // Host Office 9 apps name
WCHAR *pwzOrigURL;	 		// URL of the origin of this downloaded file.
}MSOAVINFO;

 // {56FFCC30-D398-11d0-B2AE-00A0C908FA49}
DEFINE_GUID(IID_IOfficeAntiVirus,
0x56ffcc30, 0xd398, 0x11d0, 0xb2, 0xae, 0x0, 0xa0, 0xc9, 0x8, 0xfa, 0x49);

 // {56FFCC31-D398-11d0-B2AE-00A0C908FA49}
DEFINE_GUID(CATID_MSOfficeAntiVirus,
0x56ffcc30, 0xd398, 0x11d0, 0xb2, 0xae, 0x0, 0xa0, 0xc9, 0x8, 0xfa, 0x49);



#undef  INTERFACE
#define INTERFACE  IOfficeAntiVirus
DECLARE_INTERFACE_(IOfficeAntiVirus, IUnknown)
{
    BEGIN_INTERFACE

    // *** IUnknown methods ***

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IOfficeAntiVirus methods ***

	STDMETHOD_(HRESULT, Scan)(THIS_ MSOAVINFO *pmsoavinfo ) PURE;
};

#ifndef AVVENDOR
MSOAPI_(BOOL) MsoFAnyAntiVirus(HMSOINST hmsoinst);
MSOAPI_(BOOL) MsoFDoAntiVirusScan(HMSOINST hmsoinst, MSOAVINFO *msoavinfo);
MSOAPI_(void) MsoFreeMsoavStuff(HMSOINST hmsoinst);
MSOAPI_(BOOL) MsoFDoSecurityLevelDlg(HMSOINST hmsoinst,DWORD msorid, int *pSecurityLevel, 
	BOOL *pfTrustInstalled, HWND hwndParent, BOOL fShowVirusCheckers,
	WCHAR *wzHelpFile, DWORD dwHelpId);

//output of the Enable/disable macro (edm) dialog
#define msoedmEnable	1
#define	msoedmDisable	2
#define	msoedmDontOpen	3

MSOAPI_(int) MsoMsoedmDialog(HMSOINST hmsoinst, BOOL fAppIsActive, BOOL fHasVBMacros, 
	BOOL fHasXLMMacros, void *pvDigSigStore, void *pvMacro, int nAppID, HWND hwnd, 
	const WCHAR *pwtzPath, int iClient, int iSecurityLevel, int *pmsodsv, 
	WCHAR *wzHelpFile, DWORD dwHelpId, HANDLE hFileDLL, BOOL fUserControl);


//Security level
#define	msoslUndefined	0
#define msoslNone   1
#define	msoslMedium	2
#define msoslHigh	3

MSOAPI_(int) MsoMsoslGetSL(HMSOINST hmsoinst);
MSOAPI_(int) MsoMsoslSetSL(DWORD msorid, HMSOINST hmsoinst);

//output of the digital signature verification (dsv)
#define	msodsvNoMacros	0
#define msodsvUnsigned	1
// msodsvPassedTrusted is very unfortunately named because it has nothing to do with
// trust - it just means that the doc is signed and the signature matched. Too late 
// to change the name now so I'm adding a msodsvPassedTrustedCert to mean the doc was
// signed and cert was trusted.
#define	msodsvPassedTrusted	2
#define	msodsvFailed		3
#define	msodsvLowSecurityLevel 4
#define msodsvPassedTrustedCert 5

#endif //!AVVENDOR


#endif // _MSOAV_H
