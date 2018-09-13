#if !defined (EXTERN)
#define EXTERN extern
#endif

#if !defined (ASSIGN)
#define ASSIGN(value)
#endif

/* the 'extern' must be forced for constant arrays, because 'const'
 * in C++ implies 'static' otherwise.
 */
#define EXTTEXT(n) extern const CHAR n[]
#define TEXTCONST(name,text) EXTTEXT(name) ASSIGN(text)

TEXTCONST(szProfileList,REGSTR_PATH_SETUP "\\ProfileList");
TEXTCONST(szSupervisor,"Supervisor");
TEXTCONST(szProfileImagePath,"ProfileImagePath");
TEXTCONST(szDefaultUserName,".Default");
TEXTCONST(szRATINGS,        "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Ratings");
TEXTCONST(szRatingsSupervisorKeyName,"Key");
TEXTCONST(szUsersSupervisorKeyName,"Key2");
TEXTCONST(szLogonKey,"Network\\Logon");
TEXTCONST(szUserProfiles,"UserProfiles");
TEXTCONST(szUsername,"Username");
TEXTCONST(szSupervisorPWLKey,"MSLOCUSR!SuperPW");

TEXTCONST(szProfilePrefix,"PRO");	/* for generating temp. profile file names */
TEXTCONST(szProfiles,"Profiles");
#define szProfilesDirectory	szProfiles	/* name appended to windows dir */
#define szProfileListRootKey szProfileList
TEXTCONST(szStdNormalProfile,"USER.DAT");

TEXTCONST(szReconcileRoot,"Software\\Microsoft\\Windows\\CurrentVersion");
TEXTCONST(szReconcilePrimary,"ProfileReconciliation");
TEXTCONST(szReconcileSecondary,"SecondaryProfileReconciliation");
TEXTCONST(szLocalFile,"LocalFile");
TEXTCONST(szDefaultDir,"DefaultDir");
TEXTCONST(szReconcileName,"Name");
TEXTCONST(szWindirAlias,"*windir");
TEXTCONST(szReconcileRegKey,"RegKey");
TEXTCONST(szReconcileRegValue,"RegValue");
TEXTCONST(szUseProfiles,"UserProfiles");
TEXTCONST(szDisplayProfileErrors,"DisplayProfileErrors");

TEXTCONST(szNULL, "");

TEXTCONST(szOurCLSID, "{95D0F020-451D-11CF-8DAB-00AA006C1A01}");
TEXTCONST(szCLSID,			"CLSID");
TEXTCONST(szINPROCSERVER32,	"InProcServer32");
TEXTCONST(szDLLNAME,		"mslocusr.dll");
TEXTCONST(szTHREADINGMODEL,	"ThreadingModel");
TEXTCONST(szAPARTMENT,		"Apartment");

TEXTCONST(szHelpFile,       "users.hlp");
TEXTCONST(szRatingsHelpFile,"ratings.hlp");

EXTERN CHAR abSupervisorKey[16] ASSIGN({0});		/* supervisor password hash */
EXTERN CHAR fSupervisorKeyInit ASSIGN(FALSE);		/* whether abSupervisorKey has been initialized */
