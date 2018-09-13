#include "shprv.h"

#include <dskmaint.h>
#include <prsht.h>
#include <regstr.h>
#include <winerror.h>

#include <memory.h>
#include "util.h"
#include "format.h"
#include <help.h>

#define g_hInstance g_hinst
static  char g_szNULL[] = "";   // c_szNull


#define szTextMax (256+MAX_PATH)
#define szBufMax 256
#define VERBOSE

#ifndef DEBUG
#ifdef VERBOSE
#undef VERBOSE
#endif
#endif

#ifdef VERBOSE
#define DUMP(a,b) {char szT[200];wsprintf(szT,a"\r\n",b);OutputDebugString(szT);}
#else
#define DUMP(a,b)
#endif //VERBOSE



#ifdef DEBUG
#define CHECKHRES(a) {if (!SUCCEEDED(hres)) {DebugHr(hres);goto a;}}
#else
#define CHECKHRES(a) {if (!SUCCEEDED(hres)) {goto a;}}
#endif

#ifdef DEBUG
#define CHECKSIZE(a,b) {if(sizeof(a)!=cb) {Assert(0);goto b;}}
#else
#define CHECKSIZE(a,b) {if (sizeof(a) != cb) {goto b;}}
#endif

#ifdef VERBOSE
#define MESSAGE(a) {OutputDebugString(a "\r\n");}
#else
#define MESSAGE(a) 
#endif 

static DWORD FORMATSEG aIdsMaster[]={IDC_SE_OK,IDH_OK,
				     IDC_SE_CANCEL,IDH_CANCEL,
				     IDC_SE_TXT,0xFFFFFFFFL,
				     IDC_SE_BUT1,0xFFFFFFFFL,
				     IDC_SE_BUT2,0xFFFFFFFFL,
				     IDC_SE_BUT3,0xFFFFFFFFL,
				     IDC_SE_BUT4,0xFFFFFFFFL,
				     IDC_SE_HELP,0xFFFFFFFFL,
				     IDC_SE_MOREINFO,0xFFFFFFFFL,
				     0,0};

static DWORD FORMATSEG MIaIds[]={IDC_SE_OK,IDH_OK,
				 IDC_SE_CANCEL,0xFFFFFFFFL,
				 IDC_SE_TXT,0xFFFFFFFFL,
				 IDC_SE_BUT1,0xFFFFFFFFL,
				 IDC_SE_BUT2,0xFFFFFFFFL,
				 IDC_SE_BUT3,0xFFFFFFFFL,
				 IDC_SE_BUT4,0xFFFFFFFFL,
				 IDC_SE_HELP,0xFFFFFFFFL,
				 0,0};

static DWORD FORMATSEG	XLrgRet[6]={MAKELONG(0,ERETMKCPY),
				    MAKELONG(0,ERETDELALL),
				    MAKELONG(0,ERETTNCALL),
				    MAKELONG(0xFFFF,ERETSVONED),
				    MAKELONG(0xFFFF,ERETSVONET),
				    MAKELONG(0,ERETIGN)};

static DWORD FORMATSEG	XLaIds[]={IDC_SE_OK,IDH_WINDISK_OK_FOR_ERRORS,
				  IDC_SE_CANCEL,IDH_WINDISK_CANCEL_FOR_ERRORS,
				  IDC_XL_BUT1,IDH_WINDISK_FATERRXLNK_COPY,
				  IDC_XL_BUT1+1,IDH_WINDISK_FATERRXLNK_DELETE,
				  IDC_XL_BUT1+2,IDH_WINDISK_FATERRXLNK_TRUNCATE_ALL,
				  IDC_XL_BUT1+3,IDH_WINDISK_FATERRXLNK_KEEP_SEL_DEL_OTH,
				  IDC_XL_BUT1+4,IDH_WINDISK_FATERRXLNK_KEEP_SEL_TRUNC_OTH,
				  IDC_XL_BUT1+5,IDH_WINDISK_FATERRXLNK_IGNORE,
				  IDC_SE_HELP,0xFFFFFFFFL,
				  IDC_XL_TXT1,0xFFFFFFFFL,
				  IDC_XL_TXT2,0xFFFFFFFFL,
				  IDC_XL_LIST,0xFFFFFFFFL,
				  0,0};

static DWORD FORMATSEG	XLrgRetAlt[6]={MAKELONG(0,ERETMKCPY),
				       MAKELONG(0,ERETDELALL),
				       MAKELONG(0,ERETIGN),
				       MAKELONG(0,ERETIGN),
				       MAKELONG(0,ERETIGN),
				       MAKELONG(0,ERETIGN)};

static DWORD FORMATSEG	XLaIdsAlt[]={IDC_SE_OK,IDH_WINDISK_OK_FOR_ERRORS,
				     IDC_SE_CANCEL,IDH_WINDISK_CANCEL_FOR_ERRORS,
				     IDC_XL_BUT1,IDH_WINDISK_DDERRXLSQZ_COPY,
				     IDC_XL_BUT1+1,IDH_WINDISK_DDERRXLSQZ_DELETE,
				     IDC_XL_BUT1+2,IDH_WINDISK_DDERRXLSQZ_IGNORE,
				     IDC_XL_BUT1+3,0xFFFFFFFFL,
				     IDC_XL_BUT1+4,0xFFFFFFFFL,
				     IDC_XL_BUT1+5,0xFFFFFFFFL,
				     IDC_SE_HELP,0xFFFFFFFFL,
				     IDC_XL_TXT1,0xFFFFFFFFL,
				     IDC_XL_TXT2,0xFFFFFFFFL,
				     IDC_XL_LIST,0xFFFFFFFFL,
				     0,0};

typedef struct ei_ {  //Error info
	WORD		idBase;
	WORD		cButtons;
	WORD		rgBut[4];
	WORD		AltrgBut[4];
	WORD		rgRet[4];
	DWORD		rgHelp[5];
	DWORD		AltrgHelp[5];
	BOOL		(*pfnStuff)(HWND,LPMYCHKINFOSTRUCT);
	UINT		IsHelpButton;
	UINT		IsAltHelpButton;
	DWORD		HelpButtonID;
	DWORD		AltHelpButtonID;
	BOOL		IsMoreInfoButton;
	DWORD		MoreInfoButtonHID;
	BOOL		IsMultiError;
	BOOL		DelButFilDir;
	UINT		DelButIDDir;
	DWORD		DelButDirHID;
	BOOL		FixButFilDir;
	UINT		FixButIDDir;
	DWORD		FixButDirHID;
	BOOL		WarnCantDel;
	UINT		DelButtIndx;
	UINT		CantDelTstFlag;
	BOOL		WarnCantFix;
	UINT		FixButtIndx;
	UINT		CantFixTstFlag;
	BOOL		CantFixTstIsRev;
	BOOL		OkIsContinue;
	BOOL		IsNoOk;
} EI, FAR* LPEI;

#ifdef OPK2
#define RGEIMax 57
#else
#define RGEIMax 53
#endif

#define ISNOHELPB	 0
#define ISHELPBALWAYS	 1
#define ISHELPBIFCANTDEL 2

// A template array entry:
//
// {ISTR for strings in the string table, # of radiobuttons
//	   {Strings for radio buttons ...},
//	   {Values to return for the corresponding button}
//	   {Help ID's for the message text, and then the buttons}
// }
//
// To add stuff to the log, call SEAddToLog(String,string)
// The strings are concatenated and stuck on the end of the log

static EI FORMATSEG rgEI[RGEIMax] =
{
	{ISTR_FATERRDIR,3,
		{ISTR_SE_REPAIR,ISTR_SE_DIRDEL,ISTR_SE_IGNORE,0},
		{ISTR_SE_REPAIR,ISTR_SE_DIRDEL,ISTR_SE_IGNORE,0},
		{ERETAFIX,ERETDELDIR,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRDIR_REPAIR,
		 IDH_WINDISK_ISTR_FATERRDIR_DELETE,
		 IDH_WINDISK_ISTR_FATERRDIR_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRDIR_REPAIR,
		 IDH_WINDISK_ISTR_FATERRDIR_DELETE,
		 IDH_WINDISK_ISTR_FATERRDIR_IGNORE,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK,			    // HelpButtonID
		IDH_SCANDISK,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		TRUE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		TRUE,				    // WarnCantDel
		1,				    // DelButtIndx
		ERRCANTDEL,			    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATLSTCLUS,3,
		{BUTT1(ISTR_FATLSTCLUS),BUTT2(ISTR_FATLSTCLUS),ISTR_SE_IGNORE,0},
		{BUTT1(ISTR_FATLSTCLUS),BUTT2(ISTR_FATLSTCLUS),ISTR_SE_IGNORE,0},
		{ERETFREE,ERETMKFILS,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRLSTCLUS_DISCARD,
		 IDH_WINDISK_FATERRLSTCLUS_CONVERT,
		 IDH_WINDISK_FATERRLSTCLUS_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRLSTCLUS_DISCARD,
		 IDH_WINDISK_FATERRLSTCLUS_CONVERT,
		 IDH_WINDISK_FATERRLSTCLUS_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		TRUE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATCIRCC,3,
		{ISTR_SE_FILTRNC,ISTR_SE_FILDEL,ISTR_SE_IGNORE,0},
		{ISTR_SE_FILTRNC,ISTR_SE_FILDEL,ISTR_SE_IGNORE,0},
		{ERETAFIX,ERETDELALL,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRCIRCC_TRUNCATE,
		 IDH_WINDISK_ISTR_FATERRCIRCC_DELETE,
		 IDH_WINDISK_ISTR_FATERRCIRCC_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRCIRCC_TRUNCATE,
		 IDH_WINDISK_ISTR_FATERRCIRCC_DELETE,
		 IDH_WINDISK_ISTR_FATERRCIRCC_IGNORE,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK,			    // HelpButtonID
		IDH_SCANDISK,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		TRUE,				    // DelButFilDir
		ISTR_SE_DIRDEL, 		    // DelButIDDir
		IDH_WINDISK_ISTR_FATERRCIRCC_DELETE, // DelButDirHID
		TRUE,				    // FixButFilDir
		ISTR_SE_DIRTRNC,		    // FixButIDDir
		IDH_WINDISK_ISTR_FATERRCIRCC_TRUNCATE, // FixButDirHID
		TRUE,				    // WarnCantDel
		1,				    // DelButtIndx
		ERRCANTDEL,			    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0,				    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATINVCLUS,3,
		{ISTR_SE_FILTRNC,ISTR_SE_FILDEL,ISTR_SE_IGNORE,0},
		{ISTR_SE_FILTRNC,ISTR_SE_FILDEL,ISTR_SE_IGNORE,0},
		{ERETAFIX,ERETDELALL,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRINVCLUS_TRUNCATE,
		 IDH_WINDISK_ISTR_FATERRINVCLUS_DELETE,
		 IDH_WINDISK_ISTR_FATERRINVCLUS_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRINVCLUS_TRUNCATE,
		 IDH_WINDISK_ISTR_FATERRINVCLUS_DELETE,
		 IDH_WINDISK_ISTR_FATERRINVCLUS_IGNORE,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK,			    // HelpButtonID
		IDH_SCANDISK,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		TRUE,				    // DelButFilDir
		ISTR_SE_DIRDEL, 		    // DelButIDDir
		IDH_WINDISK_ISTR_FATERRINVCLUS_DELETE, // DelButDirHID
		TRUE,				    // FixButFilDir
		ISTR_SE_DIRTRNC,		    // FixButIDDir
		IDH_WINDISK_ISTR_FATERRINVCLUS_TRUNCATE, // FixButDirHID
		TRUE,				    // WarnCantDel
		1,				    // DelButtIndx
		ERRCANTDEL,			    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0,				    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATRESVAL,2,
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRRESVAL_REPAIR,
		 IDH_WINDISK_FATERRRESVAL_DONT_REPAIR,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRRESVAL_REPAIR,
		 IDH_WINDISK_FATERRRESVAL_DONT_REPAIR,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATFMISMAT,2,
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRMISMAT_REPAIR,
		 IDH_WINDISK_FATERRMISMAT_DONT_REPAIR,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRMISMAT_REPAIR,
		 IDH_WINDISK_FATERRMISMAT_DONT_REPAIR,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATERRFILE,3,
		{ISTR_SE_REPAIR,ISTR_SE_FILDEL,ISTR_SE_IGNORE,0},
		{ISTR_SE_REPAIR,ISTR_SE_FILDEL,ISTR_SE_IGNORE,0},
		{ERETAFIX,ERETDELALL,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRFILE_REPAIR,
		 IDH_WINDISK_FATERRFILE_DELETE_FILE,
		 IDH_WINDISK_FATERRFILE_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRFILE_REPAIR,
		 IDH_WINDISK_FATERRFILE_DELETE_FILE,
		 IDH_WINDISK_FATERRFILE_IGNORE,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK,			    // HelpButtonID
		IDH_SCANDISK,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		TRUE,				    // IsMultiError
		TRUE,				    // DelButFilDir
		ISTR_SE_DIRDEL, 		    // DelButIDDir
		IDH_WINDISK_FATERRFILE_DELETE_FOLDER, // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		TRUE,				    // WarnCantDel
		1,				    // DelButtIndx
		ERRCANTDEL,			    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATERRVOLLAB,3,
		{BUTT1(ISTR_FATERRVOLLAB),BUTT2(ISTR_FATERRVOLLAB),ISTR_SE_IGNORE,0},
		{ALTBUTT1(ISTR_FATERRVOLLAB),ALTBUTT2(ISTR_FATERRVOLLAB),ISTR_SE_IGNORE,0},
		{ERETAFIX,ERETDELALL,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRVOLLAB_REPAIR_ISFRST_SET,
		 IDH_WINDISK_ISTR_FATERRVOLLAB_DELETE_ISFRST_SET,
		 IDH_WINDISK_ISTR_FATERRVOLLAB_IGNORE_ISFRST_SET,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRVOLLAB_REPAIR_ISFRST_NOTSET,
		 IDH_WINDISK_ISTR_FATERRVOLLAB_DELETE_ISFRST_NOTSET,
		 IDH_WINDISK_ISTR_FATERRVOLLAB_IGNORE_ISFRST_NOTSET,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATERRMXPLENL,3,
		{BUTT1(ISTR_FATERRMXPLENL),ISTR_SE_FILDEL,ISTR_SE_IGNORE,0},
		{ALTBUTT1(ISTR_FATERRMXPLENL),ISTR_SE_DIRDEL,ISTR_SE_IGNORE,0},
		{ERETMVFIL,ERETDELALL,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRMXPLEN_REPAIR_FILE,
		 IDH_WINDISK_FATERRMXPLEN_DELETE_FILE,
		 IDH_WINDISK_FATERRMXPLEN_IGNORE_FILE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRMXPLEN_REPAIR_FOLDER,
		 IDH_WINDISK_FATERRMXPLEN_DELETE_FOLDER,
		 IDH_WINDISK_FATERRMXPLEN_IGNORE_FOLDER,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK,			    // HelpButtonID
		IDH_SCANDISK,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		TRUE,				    // WarnCantDel
		1,				    // DelButtIndx
		ERRCANTDEL,			    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		0,				    // FixButtIndx
		ERRCANTDEL,			    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATERRMXPLENS,3,
		{ISTR_SE_IGNORE,BUTT1(ISTR_FATERRMXPLENL),ISTR_SE_FILDEL,0},
		{ISTR_SE_IGNORE,ALTBUTT1(ISTR_FATERRMXPLENL),ISTR_SE_DIRDEL,0},
		{ERETIGN,ERETMVFIL,ERETDELALL,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRMXPLEN_IGNORE_FILE,
		 IDH_WINDISK_FATERRMXPLEN_REPAIR_FILE,
		 IDH_WINDISK_FATERRMXPLEN_DELETE_FILE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRMXPLEN_IGNORE_FOLDER,
		 IDH_WINDISK_FATERRMXPLEN_REPAIR_FOLDER,
		 IDH_WINDISK_FATERRMXPLEN_DELETE_FOLDER,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK,			    // HelpButtonID
		IDH_SCANDISK,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		TRUE,				    // WarnCantDel
		2,				    // DelButtIndx
		ERRCANTDEL,			    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		1,				    // FixButtIndx
		ERRCANTDEL,			    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATERRCDLIMIT,3,
		{ISTR_SE_IGNORE,ALTBUTT1(ISTR_FATERRMXPLENL),ISTR_SE_DIRDEL,0},
		{ISTR_SE_IGNORE,ALTBUTT1(ISTR_FATERRMXPLENL),ISTR_SE_DIRDEL,0},
		{ERETIGN,ERETMVDIR,ERETDELDIR,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRCDLIMIT_IGNORE,
		 IDH_WINDISK_FATERRCDLIMIT_REPAIR,
		 IDH_WINDISK_FATERRCDLIMIT_DELETE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRCDLIMIT_IGNORE,
		 IDH_WINDISK_FATERRCDLIMIT_REPAIR,
		 IDH_WINDISK_FATERRCDLIMIT_DELETE,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK,			    // HelpButtonID
		IDH_SCANDISK,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		TRUE,				    // WarnCantDel
		2,				    // DelButtIndx
		ERRCANTDEL,			    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		1,				    // FixButtIndx
		ERRCANTDEL,			    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRSIZE1,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN2,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISHELPBALWAYS,			    // IsHelpButton
		ISHELPBALWAYS,			    // IsAltHelpButton
		IDH_COMPRESS_CORRECT_SIZE,	    // HelpButtonID
		IDH_COMPRESS_CORRECT_SIZE,	    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		TRUE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRFRAG,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN2,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISHELPBALWAYS,			    // IsHelpButton
		ISHELPBALWAYS,			    // IsAltHelpButton
		IDH_UTILITIES_DEFRAG_DISK_ERROR,    // HelpButtonID
		IDH_UTILITIES_DEFRAG_DISK_ERROR,    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		TRUE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRALIGN,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN2,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISHELPBALWAYS,			    // IsHelpButton
		ISHELPBALWAYS,			    // IsAltHelpButton
		IDH_COMPRESS_CORRECT_RATIO,	    // HelpButtonID
		IDH_COMPRESS_CORRECT_RATIO,	    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		TRUE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRNOXLCHK,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN2,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRUNSUP,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN2,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRCVFNM,2,
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRCVFNM_REPAIR,
		 IDH_WINDISK_DDERRCVFNM_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRCVFNM_REPAIR,
		 IDH_WINDISK_DDERRCVFNM_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRSIG,2,
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRSIG_REPAIR,
		 IDH_WINDISK_DDERRSIG_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRSIG_REPAIR,
		 IDH_WINDISK_DDERRSIG_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRBOOT,2,
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRBOOT_REPAIR,
		 IDH_WINDISK_DDERRBOOT_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRBOOT_REPAIR,
		 IDH_WINDISK_DDERRBOOT_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRMDBPB,2,
		{BUTT1(ISTR_DDERRMDBPB),ISTR_SE_IGNORE,0,0},
		{BUTT1(ISTR_DDERRMDBPB),ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDEMDBPB_REPAIR,
		 IDH_WINDISK_DDEMDBPB_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDEMDBPB_REPAIR,
		 IDH_WINDISK_DDEMDBPB_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK_FINISH,		    // HelpButtonID
		IDH_SCANDISK_FINISH,		    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		0,				    // FixButtIndx
		CANTFIX,			    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRSIZE2A,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETCAN,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISHELPBALWAYS,			    // IsHelpButton
		ISHELPBALWAYS,			    // IsAltHelpButton
		IDH_CVF_TOO_SMALL_CHECK_HOST,	    // HelpButtonID
		IDH_CVF_TOO_SMALL_CHECK_HOST,	    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		TRUE},				    // IsNoOk
	{ISTR_DDERRSIZE2B,1,
		{ISTR_SE_REPAIR,0,0,0},
		{ISTR_SE_REPAIR,0,0,0},
		{ERETAFIX,0,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDESIZE2_REPAIR,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDESIZE2_REPAIR,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK_FINISH,		    // HelpButtonID
		IDH_SCANDISK_FINISH,		    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		0,				    // FixButtIndx
		CANTFIX,			    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRMDFAT,2,
		{BUTT1(ISTR_DDERRMDFAT),ISTR_SE_IGNORE,0,0},
		{BUTT1(ISTR_DDERRMDFAT),ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRMDFAT_REPAIR,
		 IDH_WINDISK_DDERRMDFAT_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRMDFAT_LOST_REPAIR,
		 IDH_WINDISK_DDERRMDFAT_LOST_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRLSTSQZ,3,
		{BUTT1(ISTR_DDERRLSTSQZ),BUTT2(ISTR_DDERRLSTSQZ),ISTR_SE_IGNORE,0},
		{BUTT1(ISTR_DDERRLSTSQZ),BUTT2(ISTR_DDERRLSTSQZ),ISTR_SE_IGNORE,0},
		{ERETFREE,ERETMKFILS,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRLSTSQZ_DISCARD,
		 IDH_WINDISK_DDERRLSTSQZ_KEEP,
		 IDH_WINDISK_DDERRLSTSQZ_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_DDERRLSTSQZ_DISCARD,
		 IDH_WINDISK_DDERRLSTSQZ_KEEP,
		 IDH_WINDISK_DDERRLSTSQZ_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRISNTBAD,3,
		{BUTT1(ISTR_ERRISNTBAD),BUTT2(ISTR_ERRISNTBAD),BUTT3(ISTR_ERRISNTBAD),0},
		{BUTT1(ISTR_ERRISNTBAD),BUTT2(ISTR_ERRISNTBAD),BUTT3(ISTR_ERRISNTBAD),0},
		{ERETIGN2,ERETMRKBAD,ERETRETRY,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISNTBAD_LEAVE,
		 IDH_WINDISK_ISNTBAD_CLEAR,
		 IDH_WINDISK_ISNTBAD_RETRY,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISNTBAD_LEAVE,
		 IDH_WINDISK_ISNTBAD_CLEAR,
		 IDH_WINDISK_ISNTBAD_RETRY,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRISBAD1,3,
		{BUTT1(ISTR_ERRISBAD1),BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE,0},
		{BUTT1(ISTR_ERRISBAD1),BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE,0},
		{RESTARTWITHCH,ERETRETRY,ERETIGN2,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_COMP_HOST_NOTDONE_RESTART,
		 IDH_WINDISK_ISBAD_SYSTEM_RETRY,
		 IDH_WINDISK_ISBAD_SYSTEM_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_COMP_HOST_NOTDONE_RESTART,
		 IDH_WINDISK_ISBAD_SYSTEM_RETRY,
		 IDH_WINDISK_ISBAD_SYSTEM_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRISBAD2,4,
		{BUTT1(ISTR_ERRISBAD1),ISTR_SE_REPAIR,BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE},
		{BUTT1(ISTR_ERRISBAD1),ISTR_SE_REPAIR,BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE},
		{RESTARTWITHCH,ERETMRKBAD,ERETRETRY,ERETIGN2},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_COMP_HOST_NOTDONE_RESTART,
		 IDH_WINDISK_ISBAD_COMP_HOST_NOTDONE_REPAIR,
		 IDH_WINDISK_ISBAD_COMP_RETRY,
		 IDH_WINDISK_ISBAD_IGNORE},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_COMP_HOST_NOTDONE_RESTART,
		 IDH_WINDISK_ISBAD_COMP_HOST_NOTDONE_REPAIR,
		 IDH_WINDISK_ISBAD_COMP_RETRY,
		 IDH_WINDISK_ISBAD_IGNORE},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK_FINISH_SURF,	    // HelpButtonID
		IDH_SCANDISK_FINISH_SURF,	    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		1,				    // FixButtIndx
		RECOV,				    // CantFixTstFlag
		TRUE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRISBAD3,2,
		{BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE,0,0},
		{BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE,0,0},
		{ERETRETRY,ERETIGN2,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_SYSTEM_RETRY,
		 IDH_WINDISK_ISBAD_SYSTEM_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_SYSTEM_RETRY,
		 IDH_WINDISK_ISBAD_SYSTEM_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRISBAD4,3,
		{ISTR_SE_REPAIR,BUTT2(ISTR_ERRISBAD4),ISTR_SE_IGNORE,0},
		{ISTR_SE_REPAIR,BUTT2(ISTR_ERRISBAD4),ISTR_SE_IGNORE,0},
		{ERETMRKBAD,ERETRETRY,ERETIGN2,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_COMP_HOST_DONE_REPAIR,
		 IDH_WINDISK_ISBAD_COMP_RETRY,
		 IDH_WINDISK_ISBAD_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_COMP_HOST_DONE_REPAIR,
		 IDH_WINDISK_ISBAD_COMP_RETRY,
		 IDH_WINDISK_ISBAD_IGNORE,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK_FINISH_SURF,	    // HelpButtonID
		IDH_SCANDISK_FINISH_SURF,	    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		0,				    // FixButtIndx
		RECOV,				    // CantFixTstFlag
		TRUE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRISBAD5,3,
		{ISTR_SE_REPAIR,BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE,0},
		{ISTR_SE_REPAIR,BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE,0},
		{ERETMRKBAD,ERETRETRY,ERETIGN2,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_COMP_HOST_DONE_REPAIR,
		 IDH_WINDISK_ISBAD_COMP_RETRY,
		 IDH_WINDISK_ISBAD_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_COMP_HOST_DONE_REPAIR,
		 IDH_WINDISK_ISBAD_COMP_RETRY,
		 IDH_WINDISK_ISBAD_IGNORE,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK_FINISH_SURF,	    // HelpButtonID
		IDH_SCANDISK_FINISH_SURF,	    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		0,				    // FixButtIndx
		RECOV,				    // CantFixTstFlag
		TRUE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRISBAD6,3,
		{ISTR_SE_REPAIR,BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE,0},
		{ISTR_SE_REPAIR,BUTT2(ISTR_ERRISBAD1),ISTR_SE_IGNORE,0},
		{ERETMRKBAD,ERETRETRY,ERETIGN2,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_UNCOMP_DATA_REPAIR,
		 IDH_WINDISK_ISBAD_UNCOMP_RETRY,
		 IDH_WINDISK_ISBAD_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_UNCOMP_DATA_REPAIR,
		 IDH_WINDISK_ISBAD_UNCOMP_RETRY,
		 IDH_WINDISK_ISBAD_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRMEM,2,
		{BUTT1(ISTR_ERRMEM),BUTT2(ISTR_ERRMEM),0,0},
		{BUTT1(ISTR_ERRMEM),BUTT2(ISTR_ERRMEM),0,0},
		{ERETRETRY,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_MEMORYERROR_RETRY,
		 IDH_WINDISK_MEMORYERROR_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_MEMORYERROR_RETRY,
		 IDH_WINDISK_MEMORYERROR_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISHELPBIFCANTDEL,		    // IsHelpButton
		ISHELPBIFCANTDEL,		    // IsAltHelpButton
		IDH_SCANDISK_FINISH,		    // HelpButtonID
		IDH_SCANDISK_FINISH,		    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		TRUE,				    // WarnCantFix
		1,				    // FixButtIndx
		RECOV,				    // CantFixTstFlag
		TRUE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRCANTDEL,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN2,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_DDERRMOUNT,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN2,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_READERR1,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETCAN,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		TRUE},				    // IsNoOk
	{ISTR_READERR2,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_READERR3,3,
		{BUTT1(ISTR_READERR3),BUTT2(ISTR_READERR3),BUTT3(ISTR_READERR3),0},
		{BUTT1(ISTR_READERR3),BUTT2(ISTR_READERR3),BUTT3(ISTR_READERR3),0},
		{RESTARTWITHSA,ERETRETRY,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH,
		 IDH_WINDISK_READERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_SYSTEM_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH,
		 IDH_WINDISK_READERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_SYSTEM_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_READERR4,3,
		{BUTT1(ISTR_READERR3),BUTT2(ISTR_READERR3),BUTT3(ISTR_READERR3),0},
		{BUTT1(ISTR_READERR3),BUTT2(ISTR_READERR3),BUTT3(ISTR_READERR3),0},
		{RESTARTWITHSA,ERETRETRY,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH,
		 IDH_WINDISK_READERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_DATA_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH,
		 IDH_WINDISK_READERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_DATA_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_READERR5,3,
		{BUTT1(ISTR_READERR5),BUTT2(ISTR_READERR3),ISTR_SE_IGNORE,0},
		{BUTT1(ISTR_READERR5),BUTT2(ISTR_READERR3),ISTR_SE_IGNORE,0},
		{RESTARTWITHCH,ERETRETRY,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_COMP_THOROUGH,
		 IDH_WINDISK_READERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_COMP_SYSTEM_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_COMP_THOROUGH,
		 IDH_WINDISK_READERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_COMP_SYSTEM_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_READERR6,3,
		{BUTT1(ISTR_READERR5),BUTT2(ISTR_READERR3),ISTR_SE_IGNORE,0},
		{BUTT1(ISTR_READERR5),BUTT2(ISTR_READERR3),ISTR_SE_IGNORE,0},
		{RESTARTWITHCH,ERETRETRY,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_COMP_THOROUGH,
		 IDH_WINDISK_READERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_DATA_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_COMP_THOROUGH,
		 IDH_WINDISK_READERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_DATA_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_WRITEERR1,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETCAN,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		TRUE},				    // IsNoOk
	{ISTR_WRITEERR2,0,
		{0,0,0,0},
		{0,0,0,0},
		{ERETIGN,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_WRITEERR3,3,
		{BUTT1(ISTR_READERR3),BUTT2(ISTR_WRITEERR3),BUTT3(ISTR_READERR3),0},
		{BUTT1(ISTR_READERR3),BUTT2(ISTR_WRITEERR3),BUTT3(ISTR_READERR3),0},
		{RESTARTWITHSA,ERETRETRY,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH,
		 IDH_WINDISK_WRITEERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_SYSTEM_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH,
		 IDH_WINDISK_WRITEERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_SYSTEM_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_WRITEERR4,3,
		{BUTT1(ISTR_READERR3),BUTT2(ISTR_WRITEERR3),BUTT3(ISTR_READERR3),0},
		{BUTT1(ISTR_READERR3),BUTT2(ISTR_WRITEERR3),BUTT3(ISTR_READERR3),0},
		{RESTARTWITHSA,ERETRETRY,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH,
		 IDH_WINDISK_WRITEERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_DATA_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH,
		 IDH_WINDISK_WRITEERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_DATA_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_WRITEERR5,3,
		{BUTT1(ISTR_READERR5),BUTT2(ISTR_WRITEERR3),ISTR_SE_IGNORE,0},
		{BUTT1(ISTR_READERR5),BUTT2(ISTR_WRITEERR3),ISTR_SE_IGNORE,0},
		{RESTARTWITHCH,ERETRETRY,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_COMP_THOROUGH,
		 IDH_WINDISK_WRITEERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_COMP_SYSTEM_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_COMP_THOROUGH,
		 IDH_WINDISK_WRITEERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_COMP_SYSTEM_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_WRITEERR6,3,
		{BUTT1(ISTR_READERR5),BUTT2(ISTR_WRITEERR3),ISTR_SE_IGNORE,0},
		{BUTT1(ISTR_READERR5),BUTT2(ISTR_WRITEERR3),ISTR_SE_IGNORE,0},
		{RESTARTWITHCH,ERETRETRY,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_COMP_THOROUGH,
		 IDH_WINDISK_WRITEERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_DATA_IGNORE,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_READWRITEERROR_COMP_THOROUGH,
		 IDH_WINDISK_WRITEERROR_RETRY,
		 IDH_WINDISK_READWRITEERROR_DATA_IGNORE,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ECORRDISK,0,
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ECORRMEM,0,
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ECORRFILCOL,0,
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ECORRUNEXP,0,
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ECORRCLUSA,0,
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ECORRFILCRT,0,
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ECORROTHWRT,0,
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
#ifdef OPK2
	{ISTR_FATERRBOOT,2,
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRBOOT_REPAIR,
		 IDH_WINDISK_FATERRBOOT_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_FATERRBOOT_REPAIR,
		 IDH_WINDISK_FATERRBOOT_IGNORE,0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATERRSHDSURF,2,
		{BUTT1(ISTR_FATERRSHDSURF),BUTT2(ISTR_FATERRSHDSURF),0,0},
		{BUTT1(ISTR_FATERRSHDSURF),BUTT2(ISTR_FATERRSHDSURF),0,0},
		{ERETENABSURF,ERETIGN2,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_SHDSURF_DOSURF,
		 IDH_WINDISK_SHDSURF_IGNORE,
		 0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_SHDSURF_DOSURF,
		 IDH_WINDISK_SHDSURF_IGNORE,
		 0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_FATERRROOTDIR,2,
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ISTR_SE_REPAIR,ISTR_SE_IGNORE,0,0},
		{ERETAFIX,ERETIGN,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRROOTDIR_REPAIR,
		 IDH_WINDISK_ISTR_FATERRROOTDIR_IGNORE,
		 0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISTR_FATERRROOTDIR_REPAIR,
		 IDH_WINDISK_ISTR_FATERRROOTDIR_IGNORE,
		 0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		TRUE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
	{ISTR_ERRISBAD7,2,
		{ISTR_SE_IGNORE,BUTT1(ISTR_ERRISBAD7),0,0},
		{ISTR_SE_IGNORE,BUTT1(ISTR_ERRISBAD7),0,0},
		{ERETIGN2,ERETRETRY,0,0},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_IGNORE,
		 IDH_WINDISK_ISBAD_UNCOMP_RETRY,
		 0xFFFFFFFFL,0xFFFFFFFFL},
		{0xFFFFFFFFL,
		 IDH_WINDISK_ISBAD_IGNORE,
		 IDH_WINDISK_ISBAD_UNCOMP_RETRY,
		 0xFFFFFFFFL,0xFFFFFFFFL},
		NULL,
		ISNOHELPB,			    // IsHelpButton
		ISNOHELPB,			    // IsAltHelpButton
		0xFFFFFFFFL,			    // HelpButtonID
		0xFFFFFFFFL,			    // AltHelpButtonID
		FALSE,				    // IsMoreInfoButton
		0xFFFFFFFFL,			    // MoreInfoButtonHID
		FALSE,				    // IsMultiError
		FALSE,				    // DelButFilDir
		0,				    // DelButIDDir
		0xFFFFFFFFL,			    // DelButDirHID
		FALSE,				    // FixButFilDir
		0,				    // FixButIDDir
		0xFFFFFFFFL,			    // FixButDirHID
		FALSE,				    // WarnCantDel
		0xFFFF, 			    // DelButtIndx
		0,				    // CantDelTstFlag
		FALSE,				    // WarnCantFix
		0xFFFF, 			    // FixButtIndx
		0,				    // CantFixTstFlag
		FALSE,				    // CantFixTstIsRev
		FALSE,				    // OkIsContinue
		FALSE}, 			    // IsNoOk
#endif
};

#pragma optimize("lge",off)

DWORD NEAR GetTime(VOID)
{

    _asm {
	mov	ax, 0x2C00
	int	0x21
	mov	ax, dx
	mov	dx, cx
    }
    if (0)
	return(0L);		/* remove warning, gets otimized out */
}

DWORD NEAR GetDate(VOID)
{

    _asm {
	mov	ax, 0x2A00
	int	0x21
	mov	ax, dx
	mov	dx, cx
    }
    if (0)
	return(0L);		/* remove warning, gets otimized out */
}

#pragma optimize("",on)


//WARNING--szBuf must be either 2048 characters long or longer than
//the longest possible message.

VOID SetTextWIRV(HWND hDlg,WORD resID, LPDWORD rgdwArgs, LPSTR szBuf)
{

    //REVIEW -- I pulled a languageID of 0 out of my butt.

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
		  (LPVOID)g_hInstance,
		  resID,
		  0,
		  (LPSTR)szBuf,
		  (DWORD)szScratchMax,
		  rgdwArgs);


    SendMessage(hDlg,WM_SETTEXT,0,(LPARAM)szBuf);
    return;
}

#define SetTextDCRV(hDlg,ctrlID,resID,rgdwArgs,szBuf) {\
		SetTextWIRV(GetDlgItem(hDlg,ctrlID),resID,rgdwArgs,szBuf);}

#define SetTextDCR(hDlg,ctrlID,resID,szBuf) {\
		SetTextWIRV(GetDlgItem(hDlg,ctrlID),resID,NULL,szBuf);}


VOID SEStuffTitle(HWND hwnd,LPMYCHKINFOSTRUCT lpMyChkInf,int resID)
{
    DWORD	    NmPtr;

    NmPtr = (DWORD)(LPSTR)(&(lpMyChkInf->lpwddi->driveNameStr[0]));

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
		  (LPVOID)g_hInstance,
		  resID,
		  0,
		  (LPSTR)lpMyChkInf->szScratch,
		  (DWORD)sizeof(lpMyChkInf->szScratch),
		  (LPDWORD)&(NmPtr));

    SetWindowText(hwnd,lpMyChkInf->szScratch);

    return;
}


VOID FAR SEInitLog(LPMYCHKINFOSTRUCT lpMyChkInf)
{
#ifdef	NEC_98
    char bootdrv = 0;	// C: may not exist on PC-98
#endif

    lpMyChkInf->Log.hSz=NULL;
    lpMyChkInf->Log.cchUsed=0L;
    lpMyChkInf->Log.cchAlloced=0L;
    lpMyChkInf->Log.fMemWarned=FALSE;
    if(lpMyChkInf->Log.LogFileName[0] == '\0')
    {
	lstrcpy(lpMyChkInf->Log.LogFileName,SCANDISKLOGFILENAME);
#ifdef	NEC_98
	_asm {
	    mov  ax, 3305h	; get startup drive
	    int  21h
	    dec  dl
	    mov  bootdrv, dl	; bootdrv - 0: A, 1: B,...
	}
	lpMyChkInf->Log.LogFileName[0] = 'A' + bootdrv;
#endif	// NEC_98
    }
    return;
}

VOID NEAR SEAddLogNoMem(LPMYCHKINFOSTRUCT lpMyChkInf)
{

    if(lpMyChkInf->Log.fMemWarned)
	return;

    MyChkdskMessageBox(lpMyChkInf, IDS_SEADDLOGNOMEM,
		       MB_ICONINFORMATION | MB_OK);

    lpMyChkInf->Log.fMemWarned = TRUE;
    return;
}

VOID FAR SEAddToLogStart(LPMYCHKINFOSTRUCT lpMyChkInf, BOOL FullHeader)
{
    PBYTE pMsgBuf;
    DWORD rgdwArgs[2];

#define SZBUFA4       60
#define SZBUFB4       128
#define SZBUFC4       200
#define TOTMSZ4       (SZBUFA4+SZBUFB4+SZBUFC4)

#define dBuf1 (&(pMsgBuf[0]))
#define dBuf2 (&(pMsgBuf[SZBUFA4]))
#define dBuf3 (&(pMsgBuf[SZBUFA4+SZBUFB4]))

    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOLOG)
    {
	return;
    }

    pMsgBuf = (PBYTE)LocalAlloc(LMEM_FIXED,TOTMSZ4);
    if(!pMsgBuf)
    {
	SEAddLogNoMem(lpMyChkInf);
	return;
    }

    if(FullHeader)
    {
	DWORD dwi;
	WORD  i;
	WORD  j;
	BYTE  TSep[2];
	BYTE  DSep[2];
	BYTE  DtFmt[20];
	BYTE  hBuf[8];

	SEAddToLogRCS(lpMyChkInf,IDL_TITLE1,NULL);
	SEAddToLogRCS(lpMyChkInf,IDL_TITLE1A,NULL);

	GetProfileString("Intl","sTime",":",TSep,sizeof(TSep));
	GetProfileString("Intl","sDate","/",DSep,sizeof(DSep));
	GetProfileString("Intl","sShortDate","M/d/yy",DtFmt,sizeof(DtFmt));
	TSep[1] = '\0';
	DSep[1] = '\0';

	dwi = GetTime();
	wsprintf(dBuf1,"%02d",HIBYTE(HIWORD(dwi)));
	lstrcat(dBuf1,TSep);
	wsprintf(hBuf,"%02d",LOBYTE(HIWORD(dwi)));
	lstrcat(dBuf1,hBuf);

	dwi = GetDate();
	i = 0;
	j = 0;
	dBuf2[0] = '\0';
	while(DtFmt[i])
	{
	    if((DtFmt[i] == 'y') || (DtFmt[i] == 'Y'))
	    {
		if(!(j & 0x0001))
		{
		    j |= 0x0001;
		    wsprintf(hBuf,"%d",HIWORD(dwi));
		    lstrcat(dBuf2,hBuf);
		}
	    } else if((DtFmt[i] == 'm') || (DtFmt[i] == 'M')) {
		if(!(j & 0x0002))
		{
		    j |= 0x0002;
		    wsprintf(hBuf,"%d",HIBYTE(LOWORD(dwi)));
		    lstrcat(dBuf2,hBuf);
		}
	    } else if((DtFmt[i] == 'd') || (DtFmt[i] == 'D')) {
		if(!(j & 0x0004))
		{
		    j |= 0x0004;
		    wsprintf(hBuf,"%d",LOBYTE(LOWORD(dwi)));
		    lstrcat(dBuf2,hBuf);
		}
	    } else if(DtFmt[i] == DSep[0]) {
		lstrcat(dBuf2,DSep);
	    }
	    i++;
	}

	rgdwArgs[0] = (DWORD)(LPSTR)dBuf1;
	rgdwArgs[1] = (DWORD)(LPSTR)dBuf2;

	FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
		      (LPVOID)g_hInstance,
		      IDL_TITLE2,
		      0,
		      (LPSTR)dBuf3,
		      SZBUFC4,
		      (LPDWORD)&rgdwArgs);

	SEAddToLog(lpMyChkInf, dBuf3, NULL);

	SEAddToLogRCS(lpMyChkInf,IDL_TITLE3,NULL);

	if(lpMyChkInf->MyFixOpt & DLGCHK_NOBAD)
	{
	    SEAddToLogRCS(lpMyChkInf,IDL_OPTST,NULL);
	} else {
	    SEAddToLogRCS(lpMyChkInf,IDL_OPTTH,NULL);
	    if(lpMyChkInf->MyFixOpt & DLGCHK_NODATA)
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_OPTSYSO,NULL);
	    } else if(lpMyChkInf->MyFixOpt & DLGCHK_NOSYS) {
		SEAddToLogRCS(lpMyChkInf,IDL_OPTDTAO,NULL);
	    }
	    if(lpMyChkInf->MyFixOpt & DLGCHK_ALLHIDSYS)
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_OPTHDSYS,NULL);
	    }
	    if(lpMyChkInf->MyFixOpt & DLGCHK_NOWRTTST)
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_OPTNWRT,NULL);
	    }
	}
	if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
	{
	    SEAddToLogRCS(lpMyChkInf,IDL_OPTPRE,NULL);
	}
	if(!(lpMyChkInf->MyFixOpt & DLGCHK_NOCHKDT))
	{
	    SEAddToLogRCS(lpMyChkInf,IDL_OPTCDT,NULL);
	}
	if(lpMyChkInf->MyFixOpt & DLGCHK_NOCHKNM)
	{
	    SEAddToLogRCS(lpMyChkInf,IDL_OPTCFN,NULL);
	}
	if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST)
	{
	    SEAddToLogRCS(lpMyChkInf,IDL_OPTCHST,NULL);
	}
	if(!(lpMyChkInf->MyFixOpt & DLGCHK_INTER))
	{
	    SEAddToLogRCS(lpMyChkInf,IDL_OPTAUTOFIX,NULL);
	}

	SEAddToLogRCS(lpMyChkInf,IDL_CRLF,NULL);
    }

    if((lpMyChkInf->IsSplitDrv) && (!(lpMyChkInf->DoingCompDrv)))
    {
	rgdwArgs[0] = (DWORD)(LPSTR)lpMyChkInf->CompdriveNameStr;
	rgdwArgs[1] = (DWORD)(BYTE)(lpMyChkInf->lpwddi->iDrive + 'A');

	FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
		      (LPVOID)g_hInstance,
		      IDL_COMPDISKH,
		      0,
		      (LPSTR)dBuf2,
		      SZBUFB4,
		      (LPDWORD)&rgdwArgs);

	rgdwArgs[0] = (DWORD)(LPSTR)dBuf2;
    } else {
	rgdwArgs[0] = (DWORD)(LPSTR)lpMyChkInf->lpwddi->driveNameStr;
    }
    rgdwArgs[1] = 0L;

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
		  (LPVOID)g_hInstance,
		  IDL_TITLE4,
		  0,
		  (LPSTR)dBuf3,
		  SZBUFC4,
		  (LPDWORD)&rgdwArgs);

    SEAddToLog(lpMyChkInf, dBuf3, NULL);

    LocalFree((HANDLE)pMsgBuf);
    return;
}

VOID NEAR DoLogXLList(LPMYCHKINFOSTRUCT lpMyChkInf, UINT IDLBasID)
{
    LPFATXLNKERR lpXLErr;
    LPDDXLNKERR  lpDDXLErr;
    WORD	 i;
    WORD	 j;
    WORD	 k;
    PBYTE	 pStrBuf = 0;
    BYTE	 IndexFmt[30];
    BYTE	 SpcChar[2] = " ";
    BYTE	 FilChar[2];
    BYTE	 FolChar[2];
    BYTE	 FrgChar[2];
    BYTE	 UnmChar[2];

    SEAddToLogRCS(lpMyChkInf,IDLBasID,NULL);

    pStrBuf = (PBYTE)LocalAlloc(LMEM_FIXED,800);
    if(!pStrBuf)
    {
	SEAddLogNoMem(lpMyChkInf);
	return;
    }
    LoadString(g_hInstance,IDL_XLLIST,IndexFmt,sizeof(IndexFmt));
    LoadString(g_hInstance,IDL_XLFILE,FilChar,sizeof(FilChar));
    LoadString(g_hInstance,IDL_XLFOLD,FolChar,sizeof(FolChar));
    LoadString(g_hInstance,IDL_XLFRAG,FrgChar,sizeof(FrgChar));
    LoadString(g_hInstance,IDL_XLUNMO,UnmChar,sizeof(UnmChar));

    lpXLErr = (LPFATXLNKERR)lpMyChkInf->lParam3;
    lpDDXLErr = (LPDDXLNKERR)lpMyChkInf->lParam3;

    if(lpMyChkInf->UseAltDlgTxt)
    {
	k = 0;
	for(i = 0; i < lpDDXLErr->DDXLnkFileCnt; i++)
	{
	    // Filter out internal cross links to self

	    for(j = 0; j < i; j++)
	    {
		if(lpDDXLErr->DDXLnkList[i].FileFirstCluster == lpDDXLErr->DDXLnkList[j].FileFirstCluster)
		    goto SkipIt;
	    }
	    k++;
	    if(lpDDXLErr->DDXLnkList[i].Flags != 0)
	    {
		if(lpDDXLErr->DDXLnkList[i].FileAttributes & 0x10)
		{
		    wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FolChar[0],
			     UnmChar[0],k,
			     (LPSTR)lpDDXLErr->DDXLnkList[i].FileName);
		} else {
		    wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FilChar[0],
			     UnmChar[0],k,
			     (LPSTR)lpDDXLErr->DDXLnkList[i].FileName);
		}
	    } else {
		if(lpDDXLErr->DDXLnkList[i].FileAttributes & 0x10)
		{
		    wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FolChar[0],
			     SpcChar[0],k,
			     (LPSTR)lpDDXLErr->DDXLnkList[i].FileName);
		} else {
		    wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FilChar[0],
			     SpcChar[0],k,
			     (LPSTR)lpDDXLErr->DDXLnkList[i].FileName);
		}
	    }
	    SEAddToLog(lpMyChkInf, pStrBuf, NULL);
SkipIt:
	    ;
	}

	// See if any of the xlinked clusters/MDFAT entries are LOST

	for(i = 0; i < lpDDXLErr->DDXLnkClusCnt; i++)
	{
	    for(j = 0; j < lpDDXLErr->DDXLnkFileCnt; j++)
	    {
		if(lpDDXLErr->DDXLnkList[j].LastSecNumNotXLnked ==
		   lpDDXLErr->DDXLnkClusterList[i])
		{
		    break;
		}
	    }
	    if(j >= lpDDXLErr->DDXLnkFileCnt)
	    {
		LoadString(g_hInstance, IDS_XLNOFIL,
			   lpMyChkInf->szScratch,
			   sizeof(lpMyChkInf->szScratch));

		k++;
		wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FrgChar[0],
			 SpcChar[0],k,
			 (LPSTR)lpMyChkInf->szScratch);
		SEAddToLog(lpMyChkInf, pStrBuf, NULL);
		goto DoneLstChk;
	    }
	}
DoneLstChk:
	;
    } else {
	for(i = 0; i < lpXLErr->XLnkFileCnt; i++)
	{
	    if(lpXLErr->XLnkList[i].Flags != 0)
	    {
		if(lpXLErr->XLnkList[i].FileAttributes & 0x10)
		{
		    wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FolChar[0],
			     UnmChar[0],i + 1,
			     (LPSTR)lpXLErr->XLnkList[i].FileName);
		} else {
		    wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FilChar[0],
			     UnmChar[0],i + 1,
			     (LPSTR)lpXLErr->XLnkList[i].FileName);
		}
	    } else {
		if(lpXLErr->XLnkList[i].FileAttributes & 0x10)
		{
		    wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FolChar[0],
			     SpcChar[0],i + 1,
			     (LPSTR)lpXLErr->XLnkList[i].FileName);
		} else {
		    wsprintf((LPSTR)pStrBuf,(LPSTR)IndexFmt,FilChar[0],
			     SpcChar[0],i + 1,
			     (LPSTR)lpXLErr->XLnkList[i].FileName);
		}
	    }
	    SEAddToLog(lpMyChkInf, pStrBuf, NULL);
	}
    }
    LocalFree((HANDLE)pStrBuf);
    return;
}

VOID FAR SEAddErrToLog(LPMYCHKINFOSTRUCT lpMyChkInf)
{
    LPEI pEI;
    UINT IDLBasID;
    WORD i;
    WORD j;

    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOLOG)
    {
	return;
    }

    if(LOWORD(lpMyChkInf->lParam2) == ERRLOCKV) // This guy ignored
    {
	return;
    }

    //
    // NOTE that the following overflows for IERR_FATXLNK and IERR_DDERRXLSQZ
    //
    IDLBasID = lpMyChkInf->iErr + IDL_ER_FIRST;
    pEI = &rgEI[lpMyChkInf->iErr];

    switch(lpMyChkInf->iErr)
    {
	case IERR_FATERRFILE:
	case IERR_FATERRDIR:
	case IERR_FATLSTCLUS:
	case IERR_FATCIRCC:
	case IERR_FATINVCLUS:
	case IERR_FATRESVAL:
	case IERR_FATFMISMAT:
	case IERR_FATERRVOLLAB:
	case IERR_FATERRMXPLENL:
	case IERR_FATERRMXPLENS:
	case IERR_FATERRCDLIMIT:
	case IERR_DDERRSIZE1:
	case IERR_DDERRFRAG:
	case IERR_DDERRALIGN:
	case IERR_DDERRNOXLCHK:
	case IERR_DDERRUNSUP:
	case IERR_DDERRCVFNM:
	case IERR_DDERRSIG:
	case IERR_DDERRBOOT:
	case IERR_DDERRMDBPB:
	case IERR_DDERRSIZE2A:
	case IERR_DDERRSIZE2B:
	case IERR_DDERRMDFAT:
	case IERR_DDERRLSTSQZ:
	case IERR_ERRISNTBAD:
	case IERR_ERRISBAD1:
	case IERR_ERRISBAD2:
	case IERR_ERRISBAD3:
	case IERR_ERRISBAD4:
	case IERR_ERRISBAD5:
	case IERR_ERRISBAD6:
	case IERR_ERRMEM:
	case IERR_ERRCANTDEL:
	case IERR_DDERRMOUNT:
	case IERR_READERR1:
	case IERR_READERR2:
	case IERR_READERR3:
	case IERR_READERR4:
	case IERR_READERR5:
	case IERR_READERR6:
	case IERR_WRITEERR1:
	case IERR_WRITEERR2:
	case IERR_WRITEERR3:
	case IERR_WRITEERR4:
	case IERR_WRITEERR5:
	case IERR_WRITEERR6:
#ifdef OPK2
	case IERR_FATERRBOOT:
	case IERR_FATERRSHDSURF:
	case IERR_FATERRROOTDIR:
	case IERR_ERRISBAD7:
#endif
	    j = 0;
	    lpMyChkInf->szScratch[j] = '\0';
	    if(lpMyChkInf->UseAltDlgTxt)
	    {
		switch(lpMyChkInf->iErr)
		{
		    case IERR_FATERRVOLLAB:
			i = IDL_FATERRVOLLABALT;
			break;

		    case IERR_FATERRMXPLENL:
			i = IDL_FATERRMXPLENLALT;
			break;

		    case IERR_FATERRMXPLENS:
			i = IDL_FATERRMXPLENSALT;
			break;

		    case IERR_DDERRMDFAT:
			i = IDL_DDERRMDFATALT;
			break;
#ifdef OPK2
		    case IERR_FATERRROOTDIR:
#endif
		    case IERR_FATERRFILE:
		    case IERR_ERRMEM:
		    case IERR_FATRESVAL:
		    default:
			i = IDLBasID;
			break;
		}
	    } else {
		i = IDLBasID;
	    }
	    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
			  (LPVOID)g_hInstance,
			  i,
			  0,
			  (LPSTR)&lpMyChkInf->szScratch[j],
			  szScratchMax-j,
			  (LPDWORD)&lpMyChkInf->rgdwArgs);
	    j = lstrlen(lpMyChkInf->szScratch);

	    if(pEI->IsMultiError)
	    {
		for(i=0;i<MAXMULTSTRNGS;i++)
		{
		    if(lpMyChkInf->MltELogStrings[i] != 0)
		    {
			lpMyChkInf->szScratch[j] = '\0';
			FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
				      (LPVOID)g_hInstance,
				      lpMyChkInf->MltELogStrings[i],
				      0,
				      (LPSTR)&lpMyChkInf->szScratch[j],
				      szScratchMax-j,
				      (LPDWORD)&(lpMyChkInf->MErgdwArgs[i]));
			j = lstrlen(lpMyChkInf->szScratch);
		    }
		}
	    }
	    SEAddToLog(lpMyChkInf, lpMyChkInf->szScratch, NULL);
	    break;

	case IERR_FATXLNK:
	    IDLBasID = IDL_ERFATXL;
	    goto DoXL;
	    break;

	case IERR_DDERRXLSQZ:
	    IDLBasID = IDL_ERDDXLQ;
DoXL:
	    DoLogXLList(lpMyChkInf,IDLBasID);
	    break;

	default:
	    SEAddToLogRCS(lpMyChkInf,IDL_ERUNKNO,NULL);
	    break;
    }

    return;
}

VOID FAR SEAddRetToLog(LPMYCHKINFOSTRUCT lpMyChkInf)
{

    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOLOG)
    {
	return;
    }
    if(HIWORD(lpMyChkInf->FixRet) == ERETCAN)
    {
	SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSCAN);
	return;
    }

    switch(LOWORD(lpMyChkInf->lParam2))
    {
	// AUTOFIX IGNORE CANCEL no file/dir involved (for resolution)

#ifdef OPK2
	case FATERRBOOT:
#endif
	case FATERRRESVAL:
	case FATERRMISMAT:
	case DDERRSIZE2:
	case DDERRSIG:
	case DDERRBOOT:
	case DDERRMDBPB:
	case DDERRMDFAT:
	case DDERRCVFNM:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETIGN:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		case ERETAFIX:
		    if(LOWORD(lpMyChkInf->lParam2) == DDERRMDBPB)
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSRMDBPB);
		    } else if(LOWORD(lpMyChkInf->lParam2) == DDERRMDFAT) {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSRMDF);
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSREP);
		    }
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;

	// IGNORE2 CANCEL warning only

	case DDERRSIZE1:
	case DDERRFRAG:
	case DDERRALIGN:
	case DDERRUNSUP:
	case DDERRMOUNT:
	case DDERRNOXLCHK:
	case ERRCANTDEL:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETIGN2:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;

	// MKFILS FREE IGNORE CANCEL

	case DDERRLSTSQZ:
	case FATERRLSTCLUS:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETFREE:
		    if(LOWORD(lpMyChkInf->lParam2) == DDERRLSTSQZ)
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSFLMDF);
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSFLC);
		    }
		    break;

		case ERETMKFILS:
		    if(LOWORD(lpMyChkInf->lParam2) == DDERRLSTSQZ)
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSKLMDF);
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSKLC);
		    }
		    break;

		case ERETIGN:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;

	// AUTOFIX DELDIR DELALL IGNORE CANCEL single file or dir

#ifdef OPK2
	case FATERRROOTDIR:
#endif
	case FATERRCIRCC:
	case FATERRINVCLUS:
	case FATERRDIR:
	case FATERRFILE:
	case FATERRVOLLAB:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETAFIX:
		    if((LOWORD(lpMyChkInf->lParam2) == FATERRCIRCC) ||
		       (LOWORD(lpMyChkInf->lParam2) == FATERRINVCLUS) )
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSTRNC);
		    } else if(LOWORD(lpMyChkInf->lParam2) == FATERRVOLLAB) {
			if(lpMyChkInf->UseAltDlgTxt)
			{
			    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSREPFF);
			} else {
			    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSREPVL);
			}
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSREP);
		    }
		    break;

		case ERETDELDIR:
		case ERETDELALL:
		    if((LOWORD(lpMyChkInf->lParam2) == FATERRVOLLAB) &&
		       (!(lpMyChkInf->UseAltDlgTxt))		       )
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSDELVL);
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSDEL);
		    }
		    break;

		case ERETIGN:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;

	// AUTOFIX MVDIR MVFIL DELDIR DELALL IGNORE CANCEL single file or dir

	case FATERRCDLIMIT:
	case FATERRMXPLEN:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETMVDIR:
	      //case ERETMVFIL:  SAME VALUE AS ERETMVDIR
		case ERETAFIX:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSMVRT);
		    break;

		case ERETDELDIR:
		case ERETDELALL:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSDEL);
		    break;

		case ERETIGN:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;

	// MKCPY DELALL IGNORE CANCEL multiple files

	case DDERRXLSQZ:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETMKCPY:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSREP);
		    break;

		case ERETDELALL:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSDELXLQ);
		    break;

		case ERETIGN:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;

	// MKCPY DELALL TNCALL SVONED SVONET IGNORE CANCEL multiple files

	case FATERRXLNK:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETMKCPY:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSMCXL);
		    break;

		case ERETSVONED:
		case ERETSVONET:
		    {
			PBYTE pStrBuf;

			pStrBuf = (PBYTE)LocalAlloc(LMEM_FIXED,(256*2));
			if(!pStrBuf)
			{
			    SEAddLogNoMem(lpMyChkInf);
			    return;
			}
			pStrBuf[0]   = '\0';
			pStrBuf[256] = '\0';

			if(HIWORD(lpMyChkInf->FixRet) == ERETSVONED)
			    LoadString(g_hInstance,IDL_RSSODXL,pStrBuf,256);
			else
			    LoadString(g_hInstance,IDL_RSSOTXL,pStrBuf,256);

			wsprintf((LPSTR)&(pStrBuf[256]),(LPSTR)pStrBuf,
				 (WORD)(LOWORD(lpMyChkInf->FixRet) + 1));

			LoadString(g_hInstance,IDL_RSPRE,pStrBuf,256);

			SEAddToLog(lpMyChkInf, pStrBuf, &(pStrBuf[256]));

			LocalFree((HANDLE)pStrBuf);
		    }
		    break;


		case ERETDELALL:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSDELXL);
		    break;

		case ERETTNCALL:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSTNCXL);
		    break;

		case ERETIGN:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;

	// MRKBAD RETRY IGNORE2 CANCEL single file or dir (or none)

	case ERRISBAD:
#ifdef OPK2
	case ERRISBAD2:
#endif
	case ERRISNTBAD:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETMRKBAD:
		    if(LOWORD(lpMyChkInf->lParam2) == ERRISBAD)
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSREP);
#ifdef OPK2
		    } else if(LOWORD(lpMyChkInf->lParam2) == ERRISNTBAD) {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNMARK);
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
#else
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNMARK);
#endif
		    }
		    break;

		case ERETRETRY:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSRETBD);
		    break;

		case ERETIGN2:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;
#ifdef OPK2
	case FATERRSHDSURF:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETENABSURF:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSDOSURF);
		    break;

		case ERETIGN2:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;
#endif

	// RETRY IGNORE CANCEL

	case MEMORYERROR:
	case READERROR:
	case WRITEERROR:
	    switch(HIWORD(lpMyChkInf->FixRet))
	    {
	      //case ERETCAN:	 FILTERED ABOVE

		case ERETRETRY:
		    if(LOWORD(lpMyChkInf->lParam2) == MEMORYERROR)
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSRETMM);
		    } else if(LOWORD(lpMyChkInf->lParam2) == READERROR) {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSRETRD);
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSRETWR);
		    }
		    break;

		case ERETIGN:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSIGN);
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_RSPRE,IDL_RSUNK);
		    break;
	    }
	    break;

	case ERRLOCKV:		// This guy ignored
	default:
	    break;
    }
    return;
}

VOID FAR SEAddToLogRCS(LPMYCHKINFOSTRUCT lpMyChkInf, UINT RCId, UINT PostRCId)
{
    PBYTE pStrBuf;

    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOLOG)
    {
	return;
    }
    pStrBuf = (PBYTE)LocalAlloc(LMEM_FIXED,(256*2));
    if(!pStrBuf)
    {
	SEAddLogNoMem(lpMyChkInf);
	return;
    }
    pStrBuf[0]	 = '\0';
    pStrBuf[256] = '\0';

    if(RCId)
    {
	LoadString(g_hInstance, RCId, pStrBuf,256);
    }
    if(PostRCId)
    {
	LoadString(g_hInstance, PostRCId, &(pStrBuf[256]),256);
    }

    if((pStrBuf[0] != '\0') || (pStrBuf[256] != '\0'))
    {
	SEAddToLog(lpMyChkInf, pStrBuf, &(pStrBuf[256]));
    }
    LocalFree((HANDLE)pStrBuf);
    return;
}

//Appends szNew + szPost to the log (szPost can be NULL)

VOID FAR SEAddToLog(LPMYCHKINFOSTRUCT lpMyChkInf, LPSTR szNew, LPSTR szPost)
{
    HGLOBAL	hT;
    BYTE huge * lpsz = NULL;
    LPLOG	lpLog;
    DWORD	cchAlloc;
    DWORD	cchTotNew;
    DWORD	cchSzNew;
    char	zero='\0';

    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOLOG)
    {
	return;
    }

    lpLog = &(lpMyChkInf->Log);

    if(szPost == NULL)
    {
	szPost=&zero;
    }

    // NOTE THAT lpLog->cchUsed does not include terminating \0!!!!!

    cchSzNew = MAKELONG(lstrlen(szNew),0);
    cchTotNew = cchSzNew + MAKELONG(lstrlen(szPost),0);
    if(cchTotNew == 0L)
    {
	return;
    }

    if(lpLog->hSz == NULL)
    {
	hT = GlobalAlloc(GHND,LOGCHUNK);
	if(hT == NULL)
	{
	    SEAddLogNoMem(lpMyChkInf);
	    return;
	}
	lpLog->hSz=hT;
	lpLog->cchAlloced = LOGCHUNK;
	lpLog->cchUsed = 0L;
    }

    if(lpLog->cchAlloced < (lpLog->cchUsed + cchTotNew + 1L))
    {
	cchAlloc=((cchTotNew + 1L) > LOGCHUNK) ? (cchTotNew + 1L) : LOGCHUNK;
	hT = GlobalReAlloc(lpLog->hSz,
			   lpLog->cchAlloced + cchAlloc,
			   GMEM_MOVEABLE);
	if(hT == NULL)
	{
	    SEAddLogNoMem(lpMyChkInf);
	    return;
	}
	lpLog->hSz = hT;
	lpLog->cchAlloced += cchAlloc;
    }

    lpsz = (BYTE huge *)MAKELP(lpLog->hSz,0);
    lstrcpy(&lpsz[lpLog->cchUsed],szNew);
    lstrcpy(&lpsz[lpLog->cchUsed+cchSzNew],szPost);
    lpLog->cchUsed += cchTotNew;

    return;
}

VOID FAR SERecordLog(LPMYCHKINFOSTRUCT lpMyChkInf, BOOL MltiDrvAppnd)
{
    BYTE huge * lpData = NULL;
    OFSTRUCT	of;
    HFILE	hf = HFILE_ERROR;
    UINT	OpenMode;
    UINT	i;
    UINT	j;

    if((lpMyChkInf->MyFixOpt2 & DLGCHK_NOLOG) ||
       (lpMyChkInf->Log.hSz == NULL)	      ||
       (lpMyChkInf->Log.cchUsed == 0L)		)
    {
	goto AllDone;
    }

    lpData = (BYTE huge *)MAKELP(lpMyChkInf->Log.hSz,0);
    if(lpData == NULL)
    {
	goto LogErr;
    }
    if((lpMyChkInf->MyFixOpt2 & DLGCHK_LOGAPPEND) || MltiDrvAppnd)
    {
	OpenMode = OF_READWRITE | OF_SHARE_COMPAT;
    } else {
	OpenMode = OF_CREATE;
    }
OpenAgain:

    hf = OpenFile(lpMyChkInf->Log.LogFileName,&of,OpenMode);

    if(hf == HFILE_ERROR)
    {
	if(OpenMode != OF_CREATE)
	{
	    if(of.nErrCode == 0x0002)	// File not found (doesn't exist)?
	    {
		OpenMode = OF_CREATE;
		goto OpenAgain;
	    }
	}
LogErr:
	MyChkdskMessageBox(lpMyChkInf, IDS_SELOGNOWRITE,
			   MB_ICONINFORMATION | MB_OK);
	goto AllDone;
    }

    if((lpMyChkInf->MyFixOpt2 & DLGCHK_LOGAPPEND) || MltiDrvAppnd)
    {
	if(_llseek(hf,0L,2) == -1)
	{
	    goto LogErr;
	}
    }

    while(lpMyChkInf->Log.cchUsed)
    {
	if(lpMyChkInf->Log.cchUsed > (0x00010000L - 512L))
	{
	    i = LOWORD(0x00010000L - 512L);
	} else {
	    i = LOWORD(lpMyChkInf->Log.cchUsed);
	}

	j = _lwrite(hf,lpData,i);

	if(j != i)
	{
	    goto LogErr;
	}
	lpData += i;
	lpMyChkInf->Log.cchUsed -= (DWORD)i;
    }
AllDone:
    if(hf != HFILE_ERROR)
    {
	_lclose(hf);
	hf = HFILE_ERROR;
    }
    if(lpMyChkInf->Log.hSz != NULL)
    {
	lpData = NULL;
	GlobalFree(lpMyChkInf->Log.hSz);
	lpMyChkInf->Log.hSz = NULL;
    }
    return;
}

BOOL NEAR SEStuffDlg (HWND hDlg,LPMYCHKINFOSTRUCT lpMyChkInf)
{
    LPEI		    pEI;
    WORD		    i;
    WORD		    j;
    WORD		    k;
    DWORD FAR		    *pDw;

    pEI = &rgEI[lpMyChkInf->iErr];

    // If there's a special function, call it.

    if(pEI->pfnStuff)
    {
	return(pEI->pfnStuff(hDlg,lpMyChkInf));
    }
    SEStuffTitle(hDlg,lpMyChkInf, TITLE(pEI->idBase));

    j = 0;
    for (i=0;i<IDC_SE_CTXT;i++)
    {
	if(lpMyChkInf->UseAltDlgTxt)
	    k = ALTDLTXT1(pEI->idBase + i);
	else
	    k = DLTXT1(pEI->idBase + i);

	lpMyChkInf->szScratch[j] = '\0';
	FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
		      (LPVOID)g_hInstance,
		      k,
		      0,
		      (LPSTR)&lpMyChkInf->szScratch[j],
		      szScratchMax-j,
		      (LPDWORD)&lpMyChkInf->rgdwArgs);
	j = lstrlen(lpMyChkInf->szScratch);
    }
    if(pEI->IsMultiError)
    {
	for(i=0;i<MAXMULTSTRNGS;i++)
	{
	    if(lpMyChkInf->MltEStrings[i] != 0)
	    {
		lpMyChkInf->szScratch[j] = '\0';
		FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
			      (LPVOID)g_hInstance,
			      lpMyChkInf->MltEStrings[i],
			      0,
			      (LPSTR)&lpMyChkInf->szScratch[j],
			      szScratchMax-j,
			      (LPDWORD)&(lpMyChkInf->MErgdwArgs[i]));
		j = lstrlen(lpMyChkInf->szScratch);
	    }
	}
    }
    SendMessage(GetDlgItem(hDlg,IDC_SE_TXT),WM_SETTEXT,0,
		(LPARAM)(LPSTR)lpMyChkInf->szScratch);

    for(i=0;i<pEI->cButtons;i++)
    {
	if((i == pEI->DelButtIndx) &&
	   (pEI->DelButFilDir)	   &&
	   (lpMyChkInf->IsFolder)    )
	{
	    SetTextDCR(hDlg,
		       IDC_SE_BUT1+i,
		       pEI->DelButIDDir,
		       lpMyChkInf->szScratch);
	} else if((i == pEI->FixButtIndx) &&
		  (pEI->FixButFilDir)	  &&
		  (lpMyChkInf->IsFolder)    )
	{
	    SetTextDCR(hDlg,
		       IDC_SE_BUT1+i,
		       pEI->FixButIDDir,
		       lpMyChkInf->szScratch);
	} else {
	    if(lpMyChkInf->UseAltDlgTxt)
		k = pEI->AltrgBut[i];
	    else
		k = pEI->rgBut[i];
	    SetTextDCR(hDlg,
		       IDC_SE_BUT1+i,
		       k,
		       lpMyChkInf->szScratch);
	}
    }
    for (i=pEI->cButtons;i<IDC_SE_CBUTTONS;i++)
    {
	DestroyWindow(GetDlgItem(hDlg,IDC_SE_BUT1+i));
    }

    if(!pEI->IsMoreInfoButton)
    {
	DestroyWindow(GetDlgItem(hDlg,IDC_SE_MOREINFO));
    }

    if(lpMyChkInf->UseAltDlgTxt)
    {
	if(pEI->IsAltHelpButton == ISNOHELPB)
	{
	    DestroyWindow(GetDlgItem(hDlg,IDC_SE_HELP));
	} else if(pEI->IsAltHelpButton == ISHELPBIFCANTDEL) {
	    goto ChkHlpDisable;
	}
    } else {
	if(pEI->IsHelpButton == ISNOHELPB)
	{
	    DestroyWindow(GetDlgItem(hDlg,IDC_SE_HELP));
	} else if(pEI->IsHelpButton == ISHELPBIFCANTDEL) {
ChkHlpDisable:
	    if(pEI->WarnCantFix)
	    {
		if(lpMyChkInf->UseAltCantFix)
		{
		    if(!(HIWORD(lpMyChkInf->lParam2) & lpMyChkInf->AltCantFixTstFlag))
		    {
			DestroyWindow(GetDlgItem(hDlg,IDC_SE_HELP));
		    }
		} else {
		    if(pEI->CantFixTstIsRev)
		    {
			if((HIWORD(lpMyChkInf->lParam2) & pEI->CantFixTstFlag))
			{
			    DestroyWindow(GetDlgItem(hDlg,IDC_SE_HELP));
			}
		    } else {
			if(!(HIWORD(lpMyChkInf->lParam2) & pEI->CantFixTstFlag))
			{
			    DestroyWindow(GetDlgItem(hDlg,IDC_SE_HELP));
			}
		    }
		}
	    } else {
		if(!(HIWORD(lpMyChkInf->lParam2) & pEI->CantDelTstFlag))
		{
		    DestroyWindow(GetDlgItem(hDlg,IDC_SE_HELP));
		}
	    }
	}
    }

    if(pEI->WarnCantDel)
    {
	if(HIWORD(lpMyChkInf->lParam2) & pEI->CantDelTstFlag)
	{
	    EnableWindow(GetDlgItem(hDlg,IDC_SE_BUT1+pEI->DelButtIndx),FALSE);
	}
    }

    if(pEI->WarnCantFix)
    {
	if(lpMyChkInf->UseAltCantFix)
	{
	    if((HIWORD(lpMyChkInf->lParam2) & lpMyChkInf->AltCantFixTstFlag))
	    {
		EnableWindow(GetDlgItem(hDlg,IDC_SE_BUT1+pEI->FixButtIndx),FALSE);
	    }
	} else {
	    if(pEI->CantFixTstIsRev)
	    {
		if(!(HIWORD(lpMyChkInf->lParam2) & pEI->CantFixTstFlag))
		{
		    EnableWindow(GetDlgItem(hDlg,IDC_SE_BUT1+pEI->FixButtIndx),FALSE);
		}
	    } else {
		if(HIWORD(lpMyChkInf->lParam2) & pEI->CantFixTstFlag)
		{
		    EnableWindow(GetDlgItem(hDlg,IDC_SE_BUT1+pEI->FixButtIndx),FALSE);
		}
	    }
	}
    }

    // Following delayed till SECrunchDlg
    //
    // if(pEI->IsNoOk)
    // {
    //	   DestroyWindow(GetDlgItem(hwnd,IDC_SE_OK));
    // }

    if(pEI->OkIsContinue)
    {
	SetTextDCR(hDlg,
		   IDC_SE_OK,
		   ISTR_SE_CONTB,
		   lpMyChkInf->szScratch);
    }

    for (i=0;i<20;i++)
    {
	lpMyChkInf->aIds[i]=aIdsMaster[i];
    }
    if(pEI->IsMoreInfoButton)
    {
	lpMyChkInf->aIds[17]=pEI->MoreInfoButtonHID;
    }
    //
    // Be CAREFUL, for the following loop:
    //	0 is the TXT help
    //	1 is the first radio button
    //	2 is the second radio button
    //	etc.
    //
    for(i=0,pDw = (lpMyChkInf->aIds)+5;i<1+pEI->cButtons;i++,pDw+=2)
    {
	if((i == (pEI->DelButtIndx + 1)) &&
	   (pEI->DelButFilDir)		 &&
	   (lpMyChkInf->IsFolder)	   )
	{
	    *pDw=pEI->DelButDirHID;
	} else if((i == (pEI->DelButtIndx + 1)) &&
		  (pEI->FixButFilDir)		&&
		  (lpMyChkInf->IsFolder)	  )
	{
	    *pDw=pEI->FixButDirHID;
	} else {
	    if((pEI->WarnCantFix)	     &&
	       (i == (pEI->FixButtIndx + 1)) &&
	       (lpMyChkInf->UseAltCantFix)   &&
	       (HIWORD(lpMyChkInf->lParam2) & lpMyChkInf->AltCantFixTstFlag))
	    {
		*pDw=lpMyChkInf->AltCantFixRepHID;
	    } else {
		if(lpMyChkInf->UseAltDlgTxt)
		{
		    *pDw=pEI->AltrgHelp[i];
		} else {
		    *pDw=pEI->rgHelp[i];
		}
	    }
	}
    }
    return(TRUE);
}

VOID NEAR SECrunchXLDDDlg(HWND hwnd)
{
    HWND    hWndT;
    HWND    hWndTT;
    RECT    rc;
    RECT    rcStatic;
    int     i;
    int     dy;
    int     dyOkCancel;

    // Get the rectangle of the third button, this is the last one

    hWndTT = GetDlgItem(hwnd,IDC_XL_BUT1 + 2);
    GetWindowRect(hWndTT,&rc);
    MapWindowRect(NULL,hwnd,&rc);

    hWndT =  GetDlgItem(hwnd,IDC_SE_OK);
    GetWindowRect(hWndT,&rcStatic);
    MapWindowRect(NULL,hwnd,&rcStatic);

    dyOkCancel = rcStatic.top - rc.bottom - ((5 * HIWORD(GetDialogBaseUnits())) / 8);

    //	Move the OK, Cancel, help buttons up
	
    for(i=IDC_SE_OK;i<=IDC_SE_HELP;i++)
    {
	if(i == IDC_SE_CANCELA)
	   hWndTT=GetDlgItem(hwnd,IDC_SE_CANCEL);
	else
	   hWndTT=GetDlgItem(hwnd,i);
	if(NULL==hWndTT)
	    continue;
	hWndT=hWndTT;
	GetWindowRect(hWndT,&rc);
	MapWindowRect(NULL,hwnd,&rc);
	MoveWindow(hWndT,rc.left,rc.top-dyOkCancel,
		   rc.right-rc.left,rc.bottom-rc.top,FALSE);
    }

    // Resize the main window to the bottom of the buttons

    GetWindowRect(hWndT,&rc);

    GetWindowRect(hwnd, &rcStatic);

    dy = ((10 * HIWORD(GetDialogBaseUnits())) / 8);

    MoveWindow(hwnd,rcStatic.left,rcStatic.top,
	       rcStatic.right-rcStatic.left,rc.bottom-rcStatic.top+dy,FALSE);
    return;
}

VOID NEAR SECrunchDlg(HWND hwnd,LPSTR szScratch,LPMYCHKINFOSTRUCT lpMyChkInf)
{
    DWORD   dwi;
    HWND    hWndT;
    HWND    hWndTT;
    HWND    hWndTTT;
    RECT    rc;
    RECT    rcStatic;
    int     cyText;
    int     i;
    HDC     hdc;
    PSTR    szText;
    LRESULT cchText;
    int     dy;
    int     dx;
    int     dyOkCancel;
    LPEI    pEI;

    pEI = &rgEI[lpMyChkInf->iErr];

    //Get Size of Text box

    hWndT = GetDlgItem (hwnd,IDC_SE_TXT);
    if(NULL==hWndT)
	return;
    cchText = SendMessage(hWndT,WM_GETTEXTLENGTH,(LPARAM)0,(WPARAM)0);
    szText = (char *) LocalAlloc(LMEM_FIXED,(UINT)cchText);
    if(NULL==szText)
	return;
    GetWindowText(hWndT, szText,(UINT)cchText);
    GetWindowRect(hWndT, &rcStatic);
    MapWindowRect(NULL,hwnd,&rcStatic);

    hdc = GetDC(hWndT);

    SelectObject(hdc, GetWindowFont(hWndT));

    // Ask DrawText for the right cx and cy
    rc.left	= 0;
    rc.top	= 0;

    // Even though we may remove the border, we still have to account
    // for it. NOTE: We say the borders are 3* the edge metric instead
    // of 2*, this accounts for the text margins.

    rc.right	= (rcStatic.right - rcStatic.left) - (3 * GetSystemMetrics(SM_CXEDGE));
    rc.bottom = dy = (rcStatic.bottom - rcStatic.top) - (3 * GetSystemMetrics(SM_CYEDGE));

    cyText = DrawTextEx(hdc, szText, -1, &rc,
	    DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS | DT_EDITCONTROL |
	    DT_NOPREFIX | DT_EXTERNALLEADING, NULL);

    LocalFree((HLOCAL)szText);
    dy = (cyText - dy);

    if(dy <= 0)
    {
	// Remove the scroll bar and border and resize the text
	// control

	dwi = GetWindowLong(hWndT,GWL_STYLE);
	dwi &= ~(WS_VSCROLL | WS_BORDER);
	SetWindowLong(hWndT,GWL_STYLE,dwi);
	dwi = GetWindowLong(hWndT,GWL_EXSTYLE);
	dwi &= ~(WS_EX_CLIENTEDGE);
	SetWindowLong(hWndT,GWL_EXSTYLE,dwi);

	SetWindowPos(hWndT,NULL,rcStatic.left,rcStatic.top,
		     rcStatic.right-rcStatic.left,
		     rcStatic.bottom-rcStatic.top+dy,
		     SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW |
		     SWP_NOACTIVATE);
    } else {
	dy = 0; 	// Don't move the radio buttons
    }
    SendMessage(hWndT,EM_SETSEL,1,0xFFFFFFFFL);

    // This should automatically deselect the font
    ReleaseDC(hWndT, hdc);
    hWndT=NULL;

    //	Move the radio buttons up so that they are just below the text

    for(i=IDC_SE_BUT1;i<IDC_SE_BUT1 + IDC_SE_CBUTTONS;i++)
    {
	hWndTT = GetDlgItem(hwnd,i);
	if (NULL==hWndTT) break;
	hWndT=hWndTT;
	GetWindowRect(hWndT,&rc);
	MapWindowRect(NULL,hwnd,&rc);
	dyOkCancel = rc.bottom;
	MoveWindow(hWndT,rc.left,rc.top+dy,
				      rc.right-rc.left,rc.bottom-rc.top,FALSE);

    }
    if(NULL!=hWndT)   // Set the delta for the Ok Cancel to be the
    {		      // distance traveled by the last dialog button
	GetWindowRect(hWndT,&rc);
	MapWindowRect(NULL,hwnd,&rc);
	dyOkCancel = rc.bottom - dyOkCancel;
    } else {
	// In this case, there are no radiobuttons

	dyOkCancel = dy - 42;	 //dy is still the dy from the text box
				 //42 is the delta between the top
				 //of the button and the original text
				 //box
    }

    //	Move the OK, Cancel, more info, help buttons up
	
    for(i=IDC_SE_OK;i<=IDC_SE_HELP;i++)
    {
	if(i == IDC_SE_CANCELA)
	   hWndTT=GetDlgItem(hwnd,IDC_SE_CANCEL);
	else
	   hWndTT=GetDlgItem(hwnd,i);
	if(NULL==hWndTT)
	    continue;
	hWndT=hWndTT;
	GetWindowRect(hWndT,&rc);
	MapWindowRect(NULL,hwnd,&rc);
	MoveWindow(hWndT,rc.left,rc.top+dyOkCancel,
		   rc.right-rc.left,rc.bottom-rc.top,FALSE);
    }

    // Compute a "buton width" for following moves

    hWndTT=GetDlgItem(hwnd,IDC_SE_OK);
    hWndTTT=GetDlgItem(hwnd,IDC_SE_CANCEL);
    if((hWndTTT == NULL) || (hWndTT == NULL))
	goto SkipHelpLeft;
    GetWindowRect(hWndTTT,&rc);
    MapWindowRect(NULL,hwnd,&rc);
    GetWindowRect(hWndTT,&rcStatic);
    MapWindowRect(NULL,hwnd,&rcStatic);
    dx = rc.left - rcStatic.left;

    // If no Ok button, destroy it and move other buttons
    // to the left by a button width.

    if(pEI->IsNoOk)
    {
	DestroyWindow(GetDlgItem(hwnd,IDC_SE_OK));
	for(i=IDC_SE_OK+1;i<=IDC_SE_HELP;i++)
	{
	    if(i == IDC_SE_CANCELA)
	       hWndTTT=GetDlgItem(hwnd,IDC_SE_CANCEL);
	    else
	       hWndTTT=GetDlgItem(hwnd,i);
	    if(hWndTTT == NULL)
		continue;
	    GetWindowRect(hWndTTT,&rc);
	    MapWindowRect(NULL,hwnd,&rc);
	    MoveWindow(hWndTTT,rc.left-dx,rc.top,
		       rc.right-rc.left,rc.bottom-rc.top,FALSE);
	}
    }

    // If no More Info button, but is a help button, move HELP button
    // to the left by a button width.

    if(lpMyChkInf->UseAltDlgTxt)
    {
	if((pEI->IsAltHelpButton != ISNOHELPB) &&
	   (!(pEI->IsMoreInfoButton))		 )
	{
	    goto MoveHelp;
	}
    } else {
	if((pEI->IsHelpButton != ISNOHELPB) &&
	   (!(pEI->IsMoreInfoButton))	      )
	{
MoveHelp:
	    hWndTTT=GetDlgItem(hwnd,IDC_SE_HELP);
	    if(hWndTTT == NULL)
		goto SkipHelpLeft;
	    GetWindowRect(hWndTTT,&rc);
	    MapWindowRect(NULL,hwnd,&rc);
	    MoveWindow(hWndTTT,rc.left-dx,rc.top,
		       rc.right-rc.left,rc.bottom-rc.top,FALSE);
	}
    }
SkipHelpLeft:


    // Resize the main window to the bottom of the buttons

    GetWindowRect(hWndT,&rc);

    GetWindowRect(hwnd, &rcStatic);

    dy = ((10 * HIWORD(GetDialogBaseUnits())) / 8);

    MoveWindow(hwnd,rcStatic.left,rcStatic.top,
	       rcStatic.right-rcStatic.left,rc.bottom-rcStatic.top+dy,FALSE);
    return;
}


BOOL WINAPI SEMIDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPMYCHKINFOSTRUCT lpMyChkInf;
    WORD	      i;
    WORD	      j;
    HMENU	      hMenu;
    LPEI	      pEI;
	
    lpMyChkInf = (LPMYCHKINFOSTRUCT)GetWindowLong(hwnd,DWL_USER);
    if(NULL != lpMyChkInf)
	pEI = &rgEI[lpMyChkInf->iErr];

    switch(msg)
    {
	case  WM_INITDIALOG:
	    SetWindowLong(hwnd,DWL_USER,lParam);
	    lpMyChkInf = (LPMYCHKINFOSTRUCT)lParam;
	    pEI = &rgEI[lpMyChkInf->iErr];

	    SEStuffTitle(hwnd,lpMyChkInf, MORETIT(pEI->idBase));

	    j = 0;
	    for (i=0;i<IDC_SE_CMORT;i++)
	    {
		lpMyChkInf->szScratch[j] = '\0';
		FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
			      (LPVOID)g_hInstance,
			      MORETXT1(pEI->idBase + i),
			      0,
			      (LPSTR)&lpMyChkInf->szScratch[j],
			      szScratchMax-j,
			      (LPDWORD)&lpMyChkInf->MIrgdwArgs);
		j = lstrlen(lpMyChkInf->szScratch);
	    }

	    SendMessage(GetDlgItem(hwnd,IDC_SE_TXT),WM_SETTEXT,0,(LPARAM)(LPSTR)lpMyChkInf->szScratch);

	    for(i=0;i<IDC_SE_CBUTTONS;i++) {
		DestroyWindow(GetDlgItem(hwnd,IDC_SE_BUT1+i));
	    }
	    DestroyWindow(GetDlgItem(hwnd,IDC_SE_MOREINFO));
	    DestroyWindow(GetDlgItem(hwnd,IDC_SE_HELP));
	    DestroyWindow(GetDlgItem(hwnd,IDC_SE_CANCEL));

	    // Since there is no CANCEL button now, but there was one
	    // in the dialog template, we need to disable the close
	    // menu item which will also disable the X close cookie in
	    // in the title bar.

	    hMenu = GetSystemMenu(hwnd,FALSE);
	    if(hMenu)
	    {
		EnableMenuItem(hMenu,SC_CLOSE,MF_BYCOMMAND | MF_GRAYED);
	    }

	    SECrunchDlg(hwnd,lpMyChkInf->szScratch,lpMyChkInf);
	    SetFocus(GetDlgItem(hwnd,IDC_SE_OK));
	    SendDlgItemMessage(hwnd,IDC_SE_TXT,EM_SETSEL,1,0xFFFFFFFFL);
	    return FALSE;

	case WM_HELP:
	    WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
		    (DWORD) (LPSTR) MIaIds);
	    return(TRUE);
	    break;

	case WM_CONTEXTMENU:
	    WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
			    (DWORD) (LPSTR) MIaIds);
	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch(wParam)
	    {
		case IDC_SE_OK:
		    EndDialog(hwnd, IDC_SE_OK);
		    return FALSE;
		    break;

	    }
	    break;
    }
    return FALSE;
}

BOOL WINAPI SEDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPMYCHKINFOSTRUCT lpMyChkInf;
    DWORD	      dwi;
    WORD	      i;
    WORD	      iChoice;
    LPEI	      pEI;
    HWND	      hWndT;
	
    lpMyChkInf = (LPMYCHKINFOSTRUCT)GetWindowLong(hwnd,DWL_USER);
    if(NULL != lpMyChkInf)
	pEI = &rgEI[lpMyChkInf->iErr];

    switch(msg)
    {
	case  WM_INITDIALOG:
	    SetWindowLong(hwnd,DWL_USER,lParam);
	    lpMyChkInf = (LPMYCHKINFOSTRUCT)lParam;
	    pEI = &rgEI[lpMyChkInf->iErr];

	    //BUGBUG  check return for possible failure in dlg stuff
	    SEStuffDlg(hwnd,lpMyChkInf);

	    //Set the default item

	    if(lpMyChkInf->UseAltDefBut)
		i = lpMyChkInf->AltDefButIndx;
	    else
		i = 0;

	    if(pEI->cButtons)
	    {
		if(hWndT=GetDlgItem(hwnd,IDC_SE_BUT1+i))
		{
		    if(!IsWindowEnabled(hWndT))
		    {
			goto LookEnabled;
		    }
		    SendMessage(hWndT,BM_SETCHECK,1,0);
		} else {
LookEnabled:
		    for(i=0;i<pEI->cButtons;i++)
		    {
			if(IsWindowEnabled(GetDlgItem(hwnd,IDC_SE_BUT1+i)))
			    break;
		    }
		    if(hWndT=GetDlgItem(hwnd,IDC_SE_BUT1+i))
		    {
			SendMessage(hWndT,BM_SETCHECK,1,0);
		    } else {
			// There are buttons, but none of them are enabled,
			// so we can't fix this error.
			if(!pEI->IsNoOk)
			{
			    EnableWindow(GetDlgItem(hwnd,IDC_SE_OK),FALSE);
			}
		    }
		}
	    }

	    SECrunchDlg(hwnd,lpMyChkInf->szScratch,lpMyChkInf);

	    if(pEI->OkIsContinue)
	    {
		if(lpMyChkInf->UseAltDlgTxt)
		{
		    if(pEI->IsAltHelpButton == ISHELPBALWAYS)
		    {
			i = IDC_SE_HELP;
		    } else {
			i = IDC_SE_OK;
		    }
		} else {
		    if(pEI->IsHelpButton == ISHELPBALWAYS)
		    {
			i = IDC_SE_HELP;
		    } else {
			i = IDC_SE_OK;
		    }
		}
	    } else if(pEI->IsNoOk) {
		i = IDC_SE_HELP;
	    } else if(lpMyChkInf->CancelIsDefault) {
		i = IDC_SE_CANCEL;
	    } else {
		i = IDC_SE_OK;
	    }
	    if((i == IDC_SE_OK) &&
	       (!IsWindowEnabled(GetDlgItem(hwnd,IDC_SE_OK))))
	    {
		i = IDC_SE_HELP;
	    }
	    SendDlgItemMessage(hwnd,i,BM_SETSTYLE,(WPARAM)BS_DEFPUSHBUTTON,FALSE);
	    SetFocus(GetDlgItem(hwnd,i));
	    SendDlgItemMessage(hwnd,IDC_SE_TXT,EM_SETSEL,1,0xFFFFFFFFL);
	    if(pEI->cButtons)
		lpMyChkInf->FixRet = MAKELONG(0,ERETIGN);
	    else
		lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
	    return FALSE;
	    break;

	case WM_HELP:
	    WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
		    (DWORD) (LPSTR) lpMyChkInf->aIds);
	    return(TRUE);
	    break;

	case WM_CONTEXTMENU:
	    WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
			    (DWORD) (LPSTR) lpMyChkInf->aIds);
	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch(wParam)
	    {
		case IDC_SE_MOREINFO:
		    DialogBoxParam(g_hInstance,
				   MAKEINTRESOURCE(IDD_SE_DLG),
				   hwnd,
				   SEMIDlgProc,
				   (LPARAM)lpMyChkInf);
		    return FALSE;
		    break;

		case IDC_SE_HELP:
		    if(lpMyChkInf->UseAltDlgTxt)
		    {
			dwi = pEI->AltHelpButtonID;
		    } else {
			dwi = pEI->HelpButtonID;
		    }
		    if((lpMyChkInf->UseAltCantFix) &&
		       (HIWORD(lpMyChkInf->lParam2) & lpMyChkInf->AltCantFixTstFlag))
		    {
			dwi = lpMyChkInf->AltCantFixHID;
		    }
		    if(dwi != 0L)
			WinHelp(hwnd, "windows.hlp>Proc4", HELP_CONTEXT, dwi);
		    return FALSE;
		    break;

		case IDC_SE_OK:
		    iChoice = 0;
		    //
		    // if cButtons == 0 (no radio buttons) iChoice will be 0
		    //
		    for (i = 0; i<pEI->cButtons;i++)
		    {
			if (IsDlgButtonChecked(hwnd,IDC_SE_BUT1+i))
			{
			    iChoice=i;
			    break;
			}
		    }

		    if((pEI->WarnCantFix)	    &&
		       (iChoice == pEI->FixButtIndx)  )
		    {
			if(lpMyChkInf->UseAltCantFix)
			{
			    if((HIWORD(lpMyChkInf->lParam2) & lpMyChkInf->AltCantFixTstFlag))
			    {
				goto DoCantFix;
			    }
			} else {
			    if(pEI->CantFixTstIsRev)
			    {
				if(!(HIWORD(lpMyChkInf->lParam2) & pEI->CantFixTstFlag))
				{
				    goto DoCantFix;
				}
			    } else {
				if(HIWORD(lpMyChkInf->lParam2) & pEI->CantFixTstFlag)
				{
DoCantFix:
				    MyChkdskMessageBox(lpMyChkInf, IDS_CANTFIX,
						       MB_ICONINFORMATION | MB_OK);
				    i = 0;
				    while((i == pEI->FixButtIndx) || (i == pEI->DelButtIndx))
					i++;
				    SendDlgItemMessage(hwnd,IDC_SE_BUT1+pEI->FixButtIndx,BM_SETCHECK,0,0);
				    SendDlgItemMessage(hwnd,IDC_SE_BUT1+i,BM_SETCHECK,1,0);
				    EnableWindow(GetDlgItem(hwnd,IDC_SE_BUT1+pEI->FixButtIndx),FALSE);
				    return FALSE;
				}
			    }
			}
		    }

		    if((pEI->WarnCantDel)	    &&
		       (iChoice == pEI->DelButtIndx)  )
		    {
			if(HIWORD(lpMyChkInf->lParam2) & pEI->CantDelTstFlag)
			{
			    if(lpMyChkInf->IsFolder)
			    {
				i = IDS_CANTDELD;
			    } else {
				i = IDS_CANTDELF;
			    }
			    goto WarnDel;
			}
			if(lpMyChkInf->IsRootFolder)
			{
			    i = IDS_CANTDELROOT;
WarnDel:
			    MyChkdskMessageBox(lpMyChkInf, i,
					       MB_ICONINFORMATION | MB_OK);
			    if(pEI->DelButtIndx)
				i = 0;
			    else
				i = 1;
			    if((pEI->WarnCantFix) && (i == pEI->FixButtIndx))
			    {
				while((i == pEI->FixButtIndx) || (i == pEI->DelButtIndx))
				    i++;
			    }
			    SendDlgItemMessage(hwnd,IDC_SE_BUT1+pEI->DelButtIndx,BM_SETCHECK,0,0);
			    SendDlgItemMessage(hwnd,IDC_SE_BUT1+i,BM_SETCHECK,1,0);
			    EnableWindow(GetDlgItem(hwnd,IDC_SE_BUT1+pEI->DelButtIndx),FALSE);
			    return FALSE;
			}
			if(lpMyChkInf->IsFolder)
			{
			    if(MyChkdskMessageBox(lpMyChkInf, IDS_DIRDEL,
						  MB_ICONQUESTION | MB_OKCANCEL)
				== IDCANCEL)
			    {
				return FALSE;
			    }
			}
		    }
		    lpMyChkInf->FixRet = MAKELONG(0,pEI->rgRet[iChoice]);
		    DUMP ("Returning %d",pEI->rgRet[iChoice]);
		    EndDialog(hwnd, IDC_SE_OK);
		    return FALSE;
		    break;

		case IDC_SE_CANCEL:
		    lpMyChkInf->FixRet=MAKELONG(0,ERETCAN);
		    EndDialog(hwnd,IDC_SE_CANCEL);
		    return FALSE;
		    break;
	    }
	    break;
    }
    return FALSE;
}

typedef struct XLData_ {
	LPMYCHKINFOSTRUCT	lpMyChkInf;
	WORD			iSelected;
	BOOL			isDirDel;
} XLDATA, FAR *LPXLDATA;
	

BOOL WINAPI SEXLCantDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPFATXLNKERR lpXLErr;
    LPDDXLNKERR  lpDDXLErr;
    WORD	 i;
    HWND	 hWndT;
    LPXLDATA	 lpXLData;

    lpXLData = (LPXLDATA) GetWindowLong(hDlg,DWL_USER);

    switch(msg)
    {
	case WM_INITDIALOG:
	    SetWindowLong(hDlg,DWL_USER,lParam);
	    lpXLData = (LPXLDATA) lParam;
	    lpXLErr = (LPFATXLNKERR)lpXLData->lpMyChkInf->lParam3;
	    lpDDXLErr = (LPDDXLNKERR)lpXLData->lpMyChkInf->lParam3;
	    hWndT = GetDlgItem(hDlg,IDC_XL_LIST);
	    if(lpXLData->lpMyChkInf->UseAltDlgTxt)
	    {
		for(i = 0; i < lpDDXLErr->DDXLnkFileCnt; i++)
		{
		    if(lpXLData->isDirDel)
		    {
			if((lpDDXLErr->DDXLnkList[i].FileAttributes & 0x10))
			{
			    SendMessage(hWndT, LB_ADDSTRING,0,
				    (LPARAM)(LPSTR)lpDDXLErr->DDXLnkList[i].FileName);
			}
		    } else {
			if((lpDDXLErr->DDXLnkList[i].Flags & (XFF_ISSYSFILE |
							      XFF_ISSYSDIR)))
			{
			    SendMessage(hWndT, LB_ADDSTRING,0,
				    (LPARAM)(LPSTR)lpDDXLErr->DDXLnkList[i].FileName);
			}
		    }
		}
	    } else {
		for(i = 0; i < lpXLErr->XLnkFileCnt; i++)
		{
		    // lpXLData->iSelected == 0xFFFF if
		    // delete or truncate ALL
		    if(lpXLData->isDirDel)
		    {
			if ((i!=lpXLData->iSelected) &&
				(lpXLErr->XLnkList[i].FileAttributes & 0x10))
			{
			    SendMessage(hWndT, LB_ADDSTRING,0,
				    (LPARAM)(LPSTR)lpXLErr->XLnkList[i].FileName);
			}
		    } else {
			if ((i!=lpXLData->iSelected) &&
				(lpXLErr->XLnkList[i].Flags & (XFF_ISSYSFILE |
						     XFF_ISSYSDIR)))
			{
			    SendMessage(hWndT, LB_ADDSTRING,0,
				    (LPARAM)(LPSTR)lpXLErr->XLnkList[i].FileName);
			}
		    }
		}
	    }
	    if(lpXLData->isDirDel)
	    {
		SetTextDCR(hDlg,
			IDC_XL_CTXT,
			IDS_CXL_DIRDEL,
			lpXLData->lpMyChkInf->szScratch);

		LoadString(g_hInstance, IDS_CXL_DIRTIT, (LPSTR)lpXLData->lpMyChkInf->szScratch,sizeof(lpXLData->lpMyChkInf->szScratch));
		SetWindowText(hDlg,lpXLData->lpMyChkInf->szScratch);
	    }
	    return TRUE;
	    break;

	case WM_COMMAND:
	    switch(wParam)
	    {
		case IDC_SE_OK:
		    EndDialog(hDlg,TRUE);
		    return FALSE;
		    break;

		case IDC_SE_CANCEL:
		    EndDialog(hDlg,FALSE);
		    return FALSE;
		    break;
	    }
	    break;
    }
    return FALSE;
}
			

BOOL WINAPI SEXLDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPMYCHKINFOSTRUCT lpMyChkInf;
    LPFATXLNKERR      lpXLErr;
    LPDDXLNKERR       lpDDXLErr;
    DWORD	      dwi;
    DWORD	      dwj;
    WORD	      i;
    WORD	      j;
    WORD	      k;
    WORD	      iChoice;
    HWND	      hWndT;
    XLDATA	      xlData;
    WORD	      lowRet;
			
    lpMyChkInf = (LPMYCHKINFOSTRUCT)GetWindowLong(hwnd,DWL_USER);
    switch(msg)
    {
	case  WM_INITDIALOG:
	    SetWindowLong(hwnd,DWL_USER,lParam);
	    lpMyChkInf = (LPMYCHKINFOSTRUCT)lParam;
	    lpMyChkInf->MultCantDelFiles = FALSE;

	    if(lpMyChkInf->UseAltDlgTxt)
	    {
		FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
			      (LPVOID)g_hInstance,
			      ISTR_XL_ALTTXT1,
			      0,
			      (LPSTR)&lpMyChkInf->szScratch,
			      szScratchMax,
			      (LPDWORD)&lpMyChkInf->rgdwArgs);

		SendMessage(GetDlgItem(hwnd,IDC_XL_TXT1),WM_SETTEXT,0,
			    (LPARAM)(LPSTR)lpMyChkInf->szScratch);

		j = 0;
		for (i=0;i<2;i++)
		{
		    lpMyChkInf->szScratch[j] = '\0';
		    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
				  (LPVOID)g_hInstance,
				  ISTR_XL_ALTTXT2A + i,
				  0,
				  (LPSTR)&lpMyChkInf->szScratch[j],
				  szScratchMax-j,
				  (LPDWORD)&lpMyChkInf->rgdwArgs);
		    j = lstrlen(lpMyChkInf->szScratch);
		}

		SendMessage(GetDlgItem(hwnd,IDC_XL_TXT2),WM_SETTEXT,0,
			    (LPARAM)(LPSTR)lpMyChkInf->szScratch);

		SetTextDCR(hwnd,
			   IDC_XL_BUT1,
			   ISTR_XL_ALTBUTTREP,
			   lpMyChkInf->szScratch);

		SetTextDCR(hwnd,
			   IDC_XL_BUT1 + 1,
			   ISTR_XL_ALTBUTTDEL,
			   lpMyChkInf->szScratch);

		SetTextDCR(hwnd,
			   IDC_XL_BUT1 + 2,
			   ISTR_SE_IGNORE,
			   lpMyChkInf->szScratch);

		for(i=3;i<6;i++)
		    DestroyWindow(GetDlgItem(hwnd,IDC_XL_BUT1+i));

		SECrunchXLDDDlg(hwnd);
	    }

	    lpMyChkInf->FixRet = MAKELONG(0,ERETIGN);
	    SEStuffTitle(hwnd,lpMyChkInf, ISTR_XL_TITLE);

	    lpXLErr = (LPFATXLNKERR)lpMyChkInf->lParam3;
	    lpDDXLErr = (LPDDXLNKERR)lpMyChkInf->lParam3;

	    hWndT = GetDlgItem(hwnd, IDC_XL_LIST);
	    SendMessage(hWndT, LB_RESETCONTENT, 0, 0L);

	    j = 0xFFFF;

	    if(lpMyChkInf->UseAltDlgTxt)
	    {
		dwi = 0L;
		k = 0;
		for(i = 0; i < lpDDXLErr->DDXLnkFileCnt; i++)
		{
		    // Filter out internal cross links to self

		    for(j = 0; j < i; j++)
		    {
			if(lpDDXLErr->DDXLnkList[i].FileFirstCluster == lpDDXLErr->DDXLnkList[j].FileFirstCluster)
			    goto SkipIt;
		    }
		    SendMessage(hWndT, LB_ADDSTRING,0,(LPARAM)(LPSTR)lpDDXLErr->DDXLnkList[i].FileName);
		    SendMessage(hWndT, LB_SETITEMDATA,k,(LPARAM)(DWORD)i);
		    k++;
		    if(lpDDXLErr->DDXLnkList[i].Flags != 0)
		    {
			if(dwi)
			    lpMyChkInf->MultCantDelFiles = TRUE;
			dwi++;
		    }
SkipIt:
		    ;
		}

		// See if any of the xlinked clusters/MDFAT entries are LOST

		for(i = 0; i < lpDDXLErr->DDXLnkClusCnt; i++)
		{
		    for(j = 0; j < lpDDXLErr->DDXLnkFileCnt; j++)
		    {
			if(lpDDXLErr->DDXLnkList[j].LastSecNumNotXLnked ==
			   lpDDXLErr->DDXLnkClusterList[i])
			{
			    break;
			}
		    }
		    if(j >= lpDDXLErr->DDXLnkFileCnt)
		    {
			LoadString(g_hInstance, IDS_XLNOFIL,
				   lpMyChkInf->szScratch,
				   sizeof(lpMyChkInf->szScratch));
			SendMessage(hWndT, LB_ADDSTRING,0,
				    (LPARAM)(LPSTR)lpMyChkInf->szScratch);
			// Following sets item data of this item to LB_ERR
			SendMessage(hWndT, LB_SETITEMDATA,k,(LPARAM)0xFFFFFFFFL);
			k++;
			goto DoneLstChk;
		    }
		}
DoneLstChk:
		;
	    } else {
		dwi = 0L;
		for(i = 0; i < lpXLErr->XLnkFileCnt; i++)
		{
		    SendMessage(hWndT, LB_ADDSTRING,0,(LPARAM)(LPSTR)lpXLErr->XLnkList[i].FileName);
		    SendMessage(hWndT, LB_SETITEMDATA,i,(LPARAM)(DWORD)i);
		    if(lpXLErr->XLnkList[i].Flags != 0)
		    {
			if(dwi)
			    lpMyChkInf->MultCantDelFiles = TRUE;
			dwi++;
		    }
		    if((j == 0xFFFF) && (lpXLErr->XLnkList[i].Flags != 0))
		    {
			j = i;
		    }
		}
	    }
	    if(j == 0xFFFF)
	    {
		if(!lpMyChkInf->UseAltDlgTxt)
		{
		    for(i = 0; i < lpXLErr->XLnkFileCnt; i++)
		    {
			if(lpXLErr->XLnkList[i].FileAttributes & 0x10)
			{
			    j = i;
			    break;
			}
		    }
		}
		if(j == 0xFFFF)
		{
		    j = 0;
		}
	    }
	    if(!(HIWORD(lpMyChkInf->lParam2) & ERRCANTDEL))
	    {
		DestroyWindow(GetDlgItem(hwnd,IDC_SE_HELP));
	    } else {
		EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1+1),FALSE);
		if(!lpMyChkInf->UseAltDlgTxt)
		{
		    EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1+2),FALSE);
		    if(lpMyChkInf->MultCantDelFiles)
		    {
			EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1),FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1+3),FALSE);
			EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1+4),FALSE);
		    }
		}
	    }
	    SendMessage(hWndT, LB_SETCURSEL, j, (LONG)NULL);

	    if(lpMyChkInf->UseAltDefBut)
		i = lpMyChkInf->AltDefButIndx;
	    else
		i = 0;
	    if((!lpMyChkInf->UseAltDlgTxt) &&
	       (!IsWindowEnabled(GetDlgItem(hwnd,IDC_XL_BUT1+i))))
	    {
		for(i=0;i<6;i++)
		{
		    if(IsWindowEnabled(GetDlgItem(hwnd,IDC_XL_BUT1+i)))
			break;
		}
	    }
	    if(hWndT=GetDlgItem(hwnd,IDC_XL_BUT1+i))
	    {
		SendMessage(hWndT,BM_SETCHECK,1,0);
	    }
	    return(TRUE);
	    break;


	case WM_HELP:
	    if(lpMyChkInf->UseAltDlgTxt)
	    {
		WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
			(DWORD) (LPSTR) XLaIdsAlt);
	    } else {
		WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
			(DWORD) (LPSTR) XLaIds);
	    }
	    return(TRUE);
	    break;

	case WM_CONTEXTMENU:
	    if(lpMyChkInf->UseAltDlgTxt)
	    {
		WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
				(DWORD) (LPSTR) XLaIdsAlt);
	    } else {
		WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
				(DWORD) (LPSTR) XLaIds);
	    }
	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch (wParam) {

		case IDC_SE_HELP:
		    WinHelp(hwnd, "windows.hlp>Proc4", HELP_CONTEXT,(DWORD)IDH_SCANDISK);
		    return FALSE;
		    break;

		 case IDC_SE_OK:
		    iChoice = 0;
		    for(i = 0; i<6;i++) {
			if(IsDlgButtonChecked(hwnd,IDC_XL_BUT1+i))
			{
			    iChoice=i;
			    break;
			}
		    }

		    if(lpMyChkInf->UseAltDlgTxt)
		    {
			lowRet = 0;
		    } else {
			lowRet = (WORD) SendDlgItemMessage(hwnd,IDC_XL_LIST,LB_GETCURSEL,(WPARAM)0,(LPARAM)0);
		    }

		    //Make sure that we can delete all files that user asks us to
		    //Warn the user if we can't.

		    lpXLErr = (LPFATXLNKERR)lpMyChkInf->lParam3;
		    lpDDXLErr = (LPDDXLNKERR)lpMyChkInf->lParam3;

		    if (HIWORD(lpMyChkInf->lParam2) & ERRCANTDEL)
		    {
			if(lpMyChkInf->UseAltDlgTxt)
			{
			    if((iChoice >= 2) || (0==iChoice))
			    {
				// Ignore, Make Copy
				goto CanDeleteAll;
			    } else {
				xlData.iSelected=0xFFFF;
				goto CantDeleteAll2;
			    }
			} else {
			    if((5==iChoice) || (0==iChoice))
			    {
				// Ignore, Make Copy
				goto CanDeleteAll;
			    } else if ((1==iChoice) || (2==iChoice)) {
				//All files deleted or truncated
				xlData.iSelected=0xFFFF;
				goto CantDeleteAll2;
			    } else {
				//Check non-selected files
				j = lowRet;
			    }
			    for(i=0;i<lpXLErr->XLnkFileCnt;i++)
			    {
				if ((i!=j) &&
				    (lpXLErr->XLnkList[i].Flags & (XFF_ISSYSDIR |
								   XFF_ISSYSFILE)))
				    goto CantDeleteAll;
			    }
			}
			goto CanDeleteAll;
CantDeleteAll:
			xlData.iSelected=lowRet;
CantDeleteAll2:
			xlData.lpMyChkInf=lpMyChkInf;
			xlData.isDirDel=FALSE;

			if(!DialogBoxParam(g_hInstance,
					   MAKEINTRESOURCE(IDD_XL_CANT),
					   hwnd,
					   SEXLCantDlgProc,
					   (LPARAM)(LPXLDATA)&xlData))
			{
			    return FALSE;  // Don't end the dialog if the user cancels
			}
		    }  // If ERRCANTDEL  . . .
CanDeleteAll:
		    if(lpMyChkInf->UseAltDlgTxt)
		    {
			if((iChoice >= 2) || (0==iChoice))
			{
			    // Ignore, Make Copy
			    goto CanDelDir;
			}
			for(i=0;i<lpDDXLErr->DDXLnkFileCnt;i++)
			{
			    if(lpDDXLErr->DDXLnkList[i].FileAttributes & 0x10)
			    {
				xlData.iSelected=0xFFFF;
				goto CantDelDir2;
			    }
			}
			goto CanDelDir;
		    } else {
			if (HIWORD(lpMyChkInf->lParam2) & ERRXLNKDIR)
			{
			    if((5==iChoice) || (0==iChoice) || (2==iChoice) || (4==iChoice))
			    {
				// Ignore, Make Copy, either truncate
				goto CanDelDir;
			    } else if (1==iChoice) {  //All files deleted
				xlData.iSelected=0xFFFF;
				goto CantDelDir2;
			    } else {
				j = lowRet;	  //Check non-selected files
			    }
			    for(i=0;i<lpXLErr->XLnkFileCnt;i++)
			    {
				if ((i!=j) &&
				    (lpXLErr->XLnkList[i].FileAttributes & 0x10))
				{
				    goto CantDelDir;
				}
			    }
			    goto CanDelDir;
CantDelDir:
			    xlData.iSelected=lowRet;
CantDelDir2:
			    xlData.lpMyChkInf=lpMyChkInf;
			    xlData.isDirDel=TRUE;

			    if(!DialogBoxParam(g_hInstance,
					       MAKEINTRESOURCE(IDD_XL_CANT),
					       hwnd,
					       SEXLCantDlgProc,
					       (LPARAM)(LPXLDATA)&xlData))
			    {
				return FALSE;  // Don't end the dialog if the user cancels
			    }
			}  // If ERRXLNKDIR
		    } // if lpMyChkInf->UseAltDlgTxt
CanDelDir:
		    // If the response doesn't have to select a file to
		    // salvage, then we have to return a loword of 0.
		    // Hence the ANDing in the low word

		    if(lpMyChkInf->UseAltDlgTxt)
		    {
			lpMyChkInf->FixRet=MAKELONG(
			    LOWORD(XLrgRetAlt[iChoice]&lowRet),HIWORD(XLrgRetAlt[iChoice]));
		    } else {
			lpMyChkInf->FixRet=MAKELONG(
			    LOWORD(XLrgRet[iChoice]&lowRet),HIWORD(XLrgRet[iChoice]));
		    }
		    EndDialog(hwnd, IDC_SE_OK);
		    return FALSE;

		case IDC_SE_CANCEL:
		    lpMyChkInf->FixRet=MAKELONG(0,ERETCAN);
		    EndDialog(hwnd,IDC_SE_CANCEL);
		    return FALSE;

		case IDC_XL_LIST:
		    hWndT = (HWND)LOWORD(lParam);
		    switch(HIWORD(lParam))
		    {
			case LBN_SELCHANGE:
			    if((!lpMyChkInf->UseAltDlgTxt)		  &&
			       (HIWORD(lpMyChkInf->lParam2) & ERRCANTDEL) &&
			       (!lpMyChkInf->MultCantDelFiles)		    )
			    {
				lpXLErr = (LPFATXLNKERR)lpMyChkInf->lParam3;
				dwi = SendMessage(hWndT,LB_GETCOUNT,0,0L);
				k = 0;
				for(i = 0; i < LOWORD(dwi); i++)
				{
				    dwj = SendMessage(hWndT,LB_GETSEL,i,0L);
				    if(dwj != LB_ERR)
				    {
					if(!dwj)  // Not selected item
					{
					    dwj = SendMessage(hWndT,LB_GETITEMDATA,i,0L);
					    if(dwj != LB_ERR)
					    {
						 if((LOWORD(dwj) < lpXLErr->XLnkFileCnt) &&
						    (lpXLErr->XLnkList[LOWORD(dwj)].Flags != 0))
						 {
						     k++;
						 }
					    }

					}
				    }
				}
				iChoice = 0;
				for(i = 0; i<6;i++) {
				    if(IsDlgButtonChecked(hwnd,IDC_XL_BUT1+i))
				    {
					iChoice=i;
					break;
				    }
				}
				if(k)
				{
				    if((iChoice == 3) || (iChoice == 4))
				    {
					if(hWndT=GetDlgItem(hwnd,IDC_XL_BUT1+iChoice))
					{
					    SendMessage(hWndT,BM_SETCHECK,0,0);
					}
				    }
				    EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1+3),FALSE);
				    EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1+4),FALSE);
				    if((iChoice == 3) || (iChoice == 4))
				    {
					for(i=0;i<6;i++)
					{
					    if(IsWindowEnabled(GetDlgItem(hwnd,IDC_XL_BUT1+i)))
						break;
					}
					if(hWndT=GetDlgItem(hwnd,IDC_XL_BUT1+i))
					{
					    SendMessage(hWndT,BM_SETCHECK,1,0);
					}
				    }
				} else {
				    EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1+3),TRUE);
				    EnableWindow(GetDlgItem(hwnd,IDC_XL_BUT1+4),TRUE);
				}
			    }
			    break;

			default:
			    break;
		    }
		    break;

	    } // Switch (wParam) in WM_COMMAND

    }  //Switch (msg)
    return FALSE;
}
