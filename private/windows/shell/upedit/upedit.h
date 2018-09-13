//#define UNICODE 1
#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#endif

#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <port1632.h>
#include <commdlg.h>
#include <shellapi.h>
#include <getuser.h>

/* Icon IDs */
#define IDI_UPEICON             1

/* Dialog IDs */
#define IDD_UPEDLG              1

#define IDD_TEXT		        -1
#define IDD_UNLOCKEDGRPS        100
#define IDD_LOCKEDGRPS	        101
#define IDD_LOCK 		        102
#define IDD_UNLOCK  	        103
#define IDD_EDITLEVEL           104
#define IDD_NORUN		        105
#define IDD_NOSAVE  	        106
#define IDD_STARTUP		        107
#define IDD_NETPRINTMGR	        112
#define IDD_NETFILEMGR	        113
#define IDD_SHOWCOMMONGRPS      114
#define IDD_USEDBY              115
#define IDD_BROWSER             116
#define IDD_SYNCLOGONSCRIPT     117

/* Menu Command Defines */
#define UPEMENU                 200
#define IDM_NEW                 201
#define IDM_OPEN                202
#define IDM_SAVECURRENT         203
#define IDM_SAVEDEFAULT         204
#define IDM_SAVESYSTEM          205
#define IDM_SAVEAS              206
#define IDM_EXIT                207
#define IDM_HELPINDEX           208
#define IDM_HELPSEARCH          209
#define IDM_HELPHELP            210
#define IDM_ABOUT               211

/* StringTable Defines */
#define IDS_PROGRAMGROUPS       301
#define IDS_EXIT                302
#define IDS_SAVEAS              303
#define IDS_SAVESETTINGS        304
#define IDS_ERRORSAVING         305
#define IDS_DEFEXT              306
#define IDS_FILTERS             307
#define IDS_NONE                308
#define IDS_UPETITLE            309
#define IDS_HELPERROR           310
#define IDS_LOCKERROR           311
#define IDS_RESETLOCKERROR      312
#define IDS_SAVEFILEMERROR      313
#define IDS_SAVEPRINTMERROR     314
#define IDS_PRIVILEGEERROR      315
#define IDS_EDITLEVEL0          316
#define IDS_EDITLEVEL1          317
#define IDS_EDITLEVEL2          318
#define IDS_EDITLEVEL3          319
#define IDS_OPENTITLE           320
#define IDS_OPENWARNING         321
#define IDS_OPENWARNINGMSG      322
#define IDS_UPETITLE1           323
#define IDS_COPYCURRENT         324
#define IDS_BROWSERTITLE        325
#define IDS_OPENERROR           326
#define IDS_OPENFAILED          327
#define IDS_OPENACCESSDENIED    328
#define IDS_OPENBADFORMAT       329
#define IDS_SELECTUSER          330
#define IDS_RESETPROTECTIONFAILED    332
#define IDS_SAVESYSTEM          333
#define IDS_SAVEDEFAULT         334
#define IDS_SAVECURRENT         335
#define IDS_SAVESYSFAILED       336
#define IDS_SAVEDEFFAILED       337
#define IDS_SAVECURFAILED       338
#define IDS_PROTECTERROR        339
#define IDS_NEWSYSTEMDEF        340
#define IDS_BROWSERERROR        341
#define IDS_ACCOUNTUNKNOWN      342
#define IDS_STARTUPNONE         343
#define IDS_SAVE                344
#define IDS_STARTUPGRP          345
#define IDS_INVALIDSID          346

// Help ids
#define IDH_HELPFIRST           5000
#define IDH_SYSMENU             (IDH_HELPFIRST + 1000)
#define IDH_OPENDLG             (IDH_HELPFIRST + 1001)
#define IDH_SAVEASDLG           (IDH_HELPFIRST + 2001)
#define IDH_USERBROWSERDLG      (IDH_HELPFIRST + 3001)
#define IDH_BROWSERLOCALGROUP   (IDH_USERBROWSERDLG + 1)
#define IDH_BROWSERGLOBALGROUP  (IDH_USERBROWSERDLG + 2)
#define IDH_BROWSERFINDUSER     (IDH_USERBROWSERDLG + 3)
#define IDH_NEW                 (IDH_HELPFIRST + IDM_NEW)
#define IDH_OPEN                (IDH_HELPFIRST + IDM_OPEN)
#define IDH_SAVECURRENT         (IDH_HELPFIRST + IDM_SAVECURRENT)
#define IDH_SAVEDEFAULT         (IDH_HELPFIRST + IDM_SAVEDEFAULT)
#define IDH_SAVESYSTEM          (IDH_HELPFIRST + IDM_SAVESYSTEM)
#define IDH_SAVEAS              (IDH_HELPFIRST + IDM_SAVEAS)
#define IDH_EXIT                (IDH_HELPFIRST + IDM_EXIT)
#define IDH_HELPINDEX           (IDH_HELPFIRST + IDM_HELPINDEX)
#define IDH_HELPSEARCH          (IDH_HELPFIRST + IDM_HELPSEARCH)
#define IDH_HELPHELP            (IDH_HELPFIRST + IDM_HELPHELP)
#define IDH_ABOUT               (IDH_HELPFIRST + IDM_ABOUT)

#define MAXGROUPNAMELEN 30
#define MAXMESSAGELEN   MAX_PATH
#define MAXTITLELEN      50
#define MAXKEYLEN       100

//
// Extracted from Progman.h
// Needed to get the group name from the group data.
//
typedef struct tagGROUPDEF {
    DWORD   dwMagic;        /* magical bytes 'PMCC' */
    DWORD   cbGroup;        /* length of group segment */
    RECT    rcNormal;       /* rectangle of normal window */
    POINT   ptMin;          /* point of icon */
    WORD    wCheckSum;      /* adjust this for zero sum of file */
    WORD    nCmdShow;       /* min, max, or normal state */
    DWORD   pName;          /* name of group */
                            /* these four change interpretation */
    WORD    cxIcon;         /* width of icons */
    WORD    cyIcon;         /* hieght of icons */
    WORD    wIconFormat;    /* planes and BPP in icons */
    WORD    wReserved;      /* This word is no longer used. */

    WORD    cItems;         /* number of items in group */
    WORD    Reserved1;
    DWORD   Reserved2;
    DWORD   rgiItems[1];    /* array of ITEMDEF offsets */
} GROUPDEF, *PGROUPDEF;
typedef GROUPDEF *LPGROUPDEF;

typedef struct tagGROUPDEF_A {
    DWORD   dwMagic;        /* magical bytes 'PMCC' */
    WORD    wCheckSum;      /* adjust this for zero sum of file */
    WORD    cbGroup;        /* length of group segment */
    RECT    rcNormal;       /* rectangle of normal window */
    POINT   ptMin;          /* point of icon */
    WORD    nCmdShow;       /* min, max, or normal state */
    WORD    pName;          /* name of group */
                            /* these four change interpretation */
    WORD    cxIcon;         /* width of icons */
    WORD    cyIcon;         /* hieght of icons */
    WORD    wIconFormat;    /* planes and BPP in icons */
    WORD    wReserved;      /* This word is no longer used. */

    WORD    cItems;         /* number of items in group */
    WORD    rgiItems[1];    /* array of ITEMDEF offsets */
} GROUPDEF_A, *PGROUPDEF_A;
typedef GROUPDEF_A *LPGROUPDEF_A;

typedef struct tagGROUPDATA {
    BOOL bOrgLock;          // True if the group was orginally locked
    LPTSTR lpGroupKey;      // The group key name in the registry. It may
                            // differ from the actual group name.
} GROUPDATA, *LPGROUPDATA;

#define GROUP_MAGIC        0x43434D50L  /* 'PMCC' */
#define GROUP_UNICODE      0x43554D50L  /* 'PMUC' */

//
// in upedit.c
//
BOOL InitApplication(HANDLE);
LONG APIENTRY MainWndProc(HWND, UINT, UINT, LONG);
LONG APIENTRY UPEDlgProc(HWND, UINT, WPARAM, LONG);
BOOL IsGroupLocked(LPTSTR);
BOOL InitializeUPESettings(HWND hwnd, BOOL bInitOrgSettings);
BOOL SaveUPESettingsToRegistry();
INT  MessageFilter(INT nCode, WPARAM wParam, LPMSG lParam);

extern HANDLE hInst;
extern HWND hwndUPE;
extern HKEY hkeyCurrentUser;
extern HKEY hkeyProgramManager;
extern HKEY hkeyProgramGroupsCurrent;
extern HKEY hkeyProgramGroups;
extern HKEY hkeyPMRestrict;
extern HKEY hkeyPMSettings;
extern HANDLE hProgramGroupsEvent;
extern TCHAR szRestrict[];
extern TCHAR szUPETitle[];
extern BOOL bWorkFromCurrent;
extern UINT uiSaveSettingsMessage;
extern DWORD dwHelpContext;
extern HHOOK hhkMsgFilter;

//
// in upeutil.c
//

void CentreWindow(HWND hwnd);

HANDLE APIENTRY CreateProgramGroupsEvent();

VOID APIENTRY ResetProgramGroupsEvent();

BOOL APIENTRY HasProgramGroupsKeyChanged();

VOID APIENTRY HandleProgramGroupsKeyChange(HWND hwnd);

BOOL APIENTRY EnablePrivilege(DWORD Privilege, BOOL Enable);

BOOL APIENTRY GetCurrentUserSid(PVOID *pCurrentUserSid);

BOOL APIENTRY GetUserOrGroup(PVOID *pUserOrGroupSID, LPTSTR lpUserOrGroupName, DWORD cb);

BOOL APIENTRY LockGroups(BOOL bResetOriginalLock);

VOID InitializeGlobalSids();

BOOL GetCurrentProfileSecurityDescriptor(PSECURITY_DESCRIPTOR *pSecDesc);

//
// For Security ACL munging
//
NTSTATUS
MakeKeyUserAdminWriteableOnly(
    IN HANDLE RootKey,
    IN PUNICODE_STRING RelativeName
    );

BOOL
IsKeyUserAdminWriteableOnly(
    IN HANDLE RootKey,
    IN PUNICODE_STRING RelativeName
    );

BOOL
GetSidFromOpenedProfile(
    PSID *pSid
    );

#ifndef UPP_SEC_UMTEST
NTSTATUS
   ApplyProfileProtection(
      IN PSECURITY_DESCRIPTOR SecDesc,
      IN PSID UserOrGroup,
      IN HANDLE RootKey
      );
#endif


NTSTATUS
ApplyAclToRegistryTree (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE RootKey
    );

NTSTATUS
ApplyAclToChildren (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE Parent
    );

BOOL APIENTRY ClearTempUserProfile();

VOID APIENTRY FixupNulls(LPTSTR p);

BOOL APIENTRY GetProfileName(LPTSTR lpFilePath, DWORD cb, BOOL bOpenFilename);

BOOL APIENTRY OpenUserProfile(LPTSTR szFileName, PSID *pUserSid);

BOOL APIENTRY ResetCurrentProfileProtection(PSID CurrentUserSid);

BOOL APIENTRY SaveUserProfile(PSID UserSid, LPTSTR lpFilePath);

BOOL APIENTRY SaveCurrentProfile(PSECURITY_DESCRIPTOR pInitialSecDesc, PSID CurrentUserSid);

BOOL APIENTRY SaveDefaultProfile();

BOOL APIENTRY SaveSystemProfile();

PSECURITY_DESCRIPTOR CreateSecurityDescriptorForFile(PSID pSid, LPTSTR lpFile);
BOOL DeleteSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor);


