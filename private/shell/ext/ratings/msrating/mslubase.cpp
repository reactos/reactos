/****************************************************************************\
 *
 *   MSLUBASE.C --Structures for holding pics information
 *
 *     Created:      Jason Thomas
 *     Updated:    Ann McCurdy
 *     
\****************************************************************************/

/*Includes------------------------------------------------------------------*/
#include "msrating.h"
#include "mslubase.h"

#include <buffer.h>
#include <regentry.h>

#include <mluisupp.h>

/*Helpers-------------------------------------------------------------------*/
char PicsDelimChar='/';

int MyMessageBox(HWND hwnd, LPCSTR pszText, UINT uTitle, UINT uType){
    CHAR szTitle[MAXPATHLEN];
    
    MLLoadStringA(uTitle, szTitle, sizeof(szTitle));

    return MessageBox(hwnd, pszText, szTitle, uType);
}

int MyMessageBox(HWND hwnd, UINT uText, UINT uTitle, UINT uType){
    CHAR szText[MAXPATHLEN];
    
    MLLoadStringA(uText, szText, sizeof(szText));

    return MyMessageBox(hwnd, szText, uTitle, uType);
}

/* Variant for long messages: uText2 message will be concatenated onto the end
 * of uText before display. Message text should contain \r\n or other desired
 * separators, this function won't add them.
 */
int MyMessageBox(HWND hwnd, UINT uText, UINT uText2, UINT uTitle, UINT uType)
{
    CHAR szText[MAXPATHLEN*2] = { 0 };

    MLLoadStringA(uText, szText, sizeof(szText));

    /* Using lstrlen in case MLLoadString really returns a count of CHARACTERS,
     * on a DBCS system...
     */
    UINT cbFirst = lstrlen(szText);
    MLLoadStringA(uText2, szText + cbFirst, sizeof(szText) - cbFirst);

    return MyMessageBox(hwnd, szText, uTitle, uType);
}

/*******************************************************************

    NAME:        MyRegDeleteKey (stolen from net\user\applet32\admincfg

    SYNOPSIS:    This does what RegDeleteKey should do, which is delete
                keys that have subkeys.  Unfortunately, NT jumped off
                the empire state building and we have to jump off after
                them, so RegDeleteKey will (one day, I'm told) puke on
                keys that have subkeys.  This fcn enums them and recursively
                deletes the leaf keys.

    NOTES:        This algorithm goes against the spirit of what's mentioned
                in the SDK, which is "don't mess with a key that you're enuming",
                since it deletes keys during an enum.  However, MarcW the
                registry guru says that this is the way to do it.  The alternative,
                I suppose, is to allocate buffers and enum all the subkeys into
                the buffer, close the enum and go back through the buffer
                elminating the subkeys.  But since you have to do this recursively,
                you would wind up allocating a lot of buffers.  This is
                all so stupid.

********************************************************************/
LONG MyRegDeleteKey(HKEY hkey,LPCSTR pszSubkey)
{
    CHAR szSubkeyName[MAX_PATH+1];
    HKEY hkeySubkey;
    UINT uRet;

    uRet = RegOpenKey(hkey,pszSubkey,&hkeySubkey);
    if (uRet != ERROR_SUCCESS)
        return uRet;
    
    // best algorithm according to marcw: keep deleting zeroth subkey till
    // there are no more
    while (RegEnumKey(hkeySubkey,0,szSubkeyName,sizeof(szSubkeyName))
        == ERROR_SUCCESS && (uRet == ERROR_SUCCESS)) {
        uRet=MyRegDeleteKey(hkeySubkey,szSubkeyName);
    }

    RegCloseKey(hkeySubkey);
    if (uRet != ERROR_SUCCESS)
        return uRet;

    return RegDeleteKey(hkey,pszSubkey);
}


BOOL MyAtoi(char *pIn, int *pi)
{
    char *pc;
    ASSERT(pIn);
   
    *pi = 0;

    pc = NonWhite(pIn);
    if (!pc)
        return FALSE;
     
    if (*pc < '0' || *pc > '9')
        return FALSE;

    int accum = 0;

    while (*pc >= '0' && *pc <= '9')
    {
        accum = accum * 10 + (*pc - '0');
        pc++;
    }

    *pi = accum;
    return TRUE;
}


/*Simple types--------------------------------------------------------------*/

/* ETN */
#ifdef DEBUG
void  ETN::Set(int rIn){
    Init();
    r = rIn;
}
int ETN::Get(){
    ASSERT(fIsInit());
    return r;
}
#endif

ETN* ETN::Duplicate(){
    ETN *pETN=new ETN;
    if (fIsInit()) pETN->Set(Get());
    return pETN;
}


/* ETB */
#ifdef DEBUG
BOOL ETB::Get()
{
    ASSERT(fIsInit());
    return m_nFlags & ETB_VALUE;
}

void ETB::Set(BOOL b)
{
    m_nFlags = ETB_ISINIT | (b ? ETB_VALUE : 0);
}
#endif

ETB* ETB::Duplicate()
{
    ASSERT(fIsInit());

    ETB *pETB = new ETB;
    if (pETB != NULL)
        pETB->m_nFlags = m_nFlags;
    return pETB;
}

/* ETS */

ETS::~ETS()
{
    if (pc != NULL) {
        delete pc;
        pc = NULL;
    }
}

#ifdef DEBUG
char* ETS::Get()
{
    ASSERT(fIsInit()); 
    return pc;
}
#endif

void ETS::Set(const char *pIn)
{
    if (pc != NULL)
        delete pc;

    if (pIn != NULL) {
        pc = new char[strlenf(pIn) + 1];
        if (pc != NULL) {
            strcpyf(pc, pIn);
        }
    }
    else {
        pc = NULL;
    }
}


void ETS::SetTo(char *pIn)
{
    if (pc != NULL)
        delete pc;

    pc = pIn;
}


ETS* ETS::Duplicate()
{
    ETS *pETS=new ETS;
    if (pETS != NULL)
        pETS->Set(Get());
    return pETS;
}


UINT EtBoolRegRead(ETB &etb, HKEY hKey, char *pKeyWord)
{
    ASSERT(pKeyWord);
    DWORD dwSize, dwValue, dwType;
    UINT uErr;

    etb.Set(FALSE);
    
    dwSize = sizeof(dwValue);

    uErr = RegQueryValueEx(hKey, pKeyWord, NULL, &dwType, 
                            (LPBYTE)&dwValue, &dwSize);

    if (uErr == ERROR_SUCCESS)
    {
       if ((dwType == REG_DWORD) && (dwValue != 0))
           etb.Set(TRUE);
    }

    return uErr;
}

UINT EtBoolRegWrite(ETB &etb, HKEY hKey, char *pKeyWord)
{
    DWORD dwNum = (etb.fIsInit() && etb.Get());

    return RegSetValueEx(hKey, pKeyWord, 0, REG_DWORD, (LPBYTE)&dwNum, sizeof(dwNum));
}


UINT EtStringRegRead(ETS &ets, HKEY hKey, char *pKeyWord)
{
    DWORD dwSize;
    UINT uErr;
    char szTemp[80];        /* default size */

    ASSERT(pKeyWord);

    dwSize = sizeof(szTemp);

    uErr = RegQueryValueEx(hKey, pKeyWord, NULL, NULL, 
                            (LPBYTE)szTemp, &dwSize);

    if (uErr == ERROR_SUCCESS)
        ets.Set(szTemp);
    else if (uErr == ERROR_MORE_DATA) {
        char *pszTemp = new char[dwSize];
        if (pszTemp == NULL)
            uErr = ERROR_NOT_ENOUGH_MEMORY;
        else {
            
            uErr = RegQueryValueEx(hKey, pKeyWord, NULL, NULL, (LPBYTE)pszTemp,
                                   &dwSize);
            if (uErr == ERROR_SUCCESS) {
                ets.SetTo(pszTemp);
                /* ets now owns pszTemp memory, don't delete it here */
            }
            else
                delete pszTemp;
        }
    }

    return uErr;
}

UINT EtStringRegWrite(ETS &ets, HKEY hKey, char *pKeyWord)
{
    ASSERT(pKeyWord);

    if (ets.fIsInit())
        return RegSetValueEx(hKey, pKeyWord, 0, REG_SZ, (LPBYTE)ets.Get(), strlenf(ets.Get())+1);

    return ERROR_SUCCESS;
}
                     

UINT EtNumRegRead(ETN &etn, HKEY hKey, char *pKeyWord)
{
    ASSERT(pKeyWord);
    DWORD dwSize, dwType;
    int nValue;
    UINT uErr;

    dwSize = sizeof(nValue);

    uErr = RegQueryValueEx(hKey, pKeyWord, NULL, &dwType, 
                            (LPBYTE)&nValue, &dwSize);

    if (uErr == ERROR_SUCCESS)
        etn.Set(nValue);

    return uErr;
}

UINT EtNumRegWrite(ETN &etn, HKEY hKey, char *pKeyWord)
{
    int nTemp;

    if (etn.fIsInit())
    {
        nTemp = etn.Get();
        return RegSetValueEx(hKey, pKeyWord, 0, REG_DWORD, (LPBYTE)&nTemp, sizeof(nTemp));
    }

    return ERROR_SUCCESS;
}


/*PicsDefault---------------------------------------------------------------*/

PicsDefault::PicsDefault()
{
    /* nothing to do but construct members */
}

PicsDefault::~PicsDefault()
{
    /* nothing to do but destruct members */
}

/*PicsEnum------------------------------------------------------------------*/

PicsEnum::PicsEnum()
{
    /* nothing to do but construct members */
}

PicsEnum::~PicsEnum()
{
    /* nothing to do but destruct members */
}

/*PicsCategory--------------------------------------------------------------*/

PicsCategory::PicsCategory()
{
    /* nothing to do but construct members */
}

PicsCategory::~PicsCategory()
{
    arrpPC.DeleteAll();
    arrpPE.DeleteAll();
}


/*PicsRatingSystem----------------------------------------------------------*/

PicsRatingSystem::PicsRatingSystem()
    : m_pDefaultOptions(NULL),
      dwFlags(0),
      nErrLine(0)
{
    /* nothing to do but construct members */
}

PicsRatingSystem::~PicsRatingSystem()
{
    arrpPC.DeleteAll();
    if (m_pDefaultOptions != NULL)
        delete m_pDefaultOptions;
}

void PicsRatingSystem::ReportError(HRESULT hres)
{
    NLS_STR nls1(etstrFile.Get());
    if (!nls1.QueryError()) {
        ISTR istrMarker(nls1);
        if (nls1.strchr(&istrMarker, '*'))
            nls1.DelSubStr(istrMarker);
    }
    else
        nls1 = szNULL;

    UINT idMsg, idTemplate;

    NLS_STR nlsBaseMessage(MAX_RES_STR_LEN);
    char szNumber[16];            /* big enough for a 32-bit (hex) number */

    if (hres == E_OUTOFMEMORY || (hres > RAT_E_BASE && hres <= RAT_E_BASE + 0xffff)) {
        idTemplate = IDS_LOADRAT_SYNTAX_TEMPLATE;    /* default is ratfile content error */
        switch (hres) {
        case E_OUTOFMEMORY:
            idMsg = IDS_LOADRAT_MEMORY;
            idTemplate = IDS_LOADRAT_GENERIC_TEMPLATE;
            break;
        case RAT_E_EXPECTEDLEFT:
            idMsg = IDS_LOADRAT_EXPECTEDLEFT; break;
        case RAT_E_EXPECTEDRIGHT:
            idMsg = IDS_LOADRAT_EXPECTEDRIGHT; break;
        case RAT_E_EXPECTEDTOKEN:
            idMsg = IDS_LOADRAT_EXPECTEDTOKEN; break;
        case RAT_E_EXPECTEDSTRING:
            idMsg = IDS_LOADRAT_EXPECTEDSTRING; break;
        case RAT_E_EXPECTEDNUMBER:
            idMsg = IDS_LOADRAT_EXPECTEDNUMBER; break;
        case RAT_E_EXPECTEDBOOL:
            idMsg = IDS_LOADRAT_EXPECTEDBOOL; break;
        case RAT_E_DUPLICATEITEM:
            idMsg = IDS_LOADRAT_DUPLICATEITEM; break;
        case RAT_E_MISSINGITEM:
            idMsg = IDS_LOADRAT_MISSINGITEM; break;
        case RAT_E_UNKNOWNITEM:
            idMsg = IDS_LOADRAT_UNKNOWNITEM; break;
        case RAT_E_UNKNOWNMANDATORY:
            idMsg = IDS_LOADRAT_UNKNOWNMANDATORY; break;
        case RAT_E_EXPECTEDEND:
            idMsg = IDS_LOADRAT_EXPECTEDEND; break;
        default:
            ASSERT(FALSE);        /* there aren't any other RAT_E_ errors  */
            idMsg = IDS_LOADRAT_UNKNOWNERROR;
            break;
        }

        wsprintf(szNumber, "%d", nErrLine);
        NLS_STR nlsNumber(STR_OWNERALLOC, szNumber);

        const NLS_STR *apnls[] = { &nlsNumber, NULL };
        nlsBaseMessage.LoadString((USHORT)idMsg, apnls);
    }
    else {
        idTemplate = IDS_LOADRAT_GENERIC_TEMPLATE;
        if (HRESULT_FACILITY(hres) == FACILITY_WIN32) {
            wsprintf(szNumber, "%d", HRESULT_CODE(hres));
            idMsg = IDS_LOADRAT_WINERROR;
        }
        else {
            wsprintf(szNumber, "0x%x", hres);
            idMsg = IDS_LOADRAT_MISCERROR;
        }
        NLS_STR nls1(STR_OWNERALLOC, szNumber);
        const NLS_STR *apnls[] = { &nls1, NULL };
        nlsBaseMessage.LoadString((USHORT)idMsg, apnls);
    }

    NLS_STR nlsMessage(MAX_RES_STR_LEN);
    const NLS_STR *apnls[] = { &nls1, &nlsBaseMessage, NULL };
    nlsMessage.LoadString((USHORT)idTemplate, apnls);
    if (!nlsMessage.QueryError()) {
        MyMessageBox(NULL, nlsMessage.QueryPch(), IDS_GENERIC, MB_OK | MB_ICONWARNING);
    }
}


/*Rating Information--------------------------------------------------------*/
BOOL PicsRatingSystemInfo::LoadProviderFiles(HKEY hKey)
{
    char szFileName[8 + 7 + 1];    /* "Filename" + big number plus null byte */
    ETS  etstrFileName;
    int  index = 0;
   
    EtStringRegRead(etstrRatingBureau, hKey, (char *)szRATINGBUREAU);
   
    wsprintf(szFileName, szFilenameTemplate, index);
    while (EtStringRegRead(etstrFileName, hKey, szFileName) == ERROR_SUCCESS) 
    {
        PicsRatingSystem *pPRS;
        HRESULT hres = LoadRatingSystem(etstrFileName.Get(), &pPRS);
        if (pPRS != NULL) {
            arrpPRS.Append(pPRS);

            /* If the thing has a pathname, write it back out to the policy
             * file, in case loading the rating system marked it as invalid
             * (or it had been marked as invalid, but the user fixed things
             * and it's OK now).
             */
            if (pPRS->etstrFile.fIsInit()) {
                /* If the rating system is not valid and was not previously
                 * marked invalid, then report the error to the user.
                 * LoadRatingSystem will have already marked the filename
                 * as invalid for us.
                 */
                if ((pPRS->dwFlags & (PRS_ISVALID | PRS_WASINVALID)) == 0) {
                    pPRS->ReportError(hres);    /* report error to user */
                }
                EtStringRegWrite(pPRS->etstrFile, hKey, szFileName);
            }
        }

        index++;
        wsprintf(szFileName, szFilenameTemplate, index);
    }

    return arrpPRS.Length() != 0;
}


BOOL RunningOnNT()
{
    return !(::GetVersion() & 0x80000000);
}


BOOL BuildPolName(LPSTR pBuffer, UINT cbBuffer, UINT (WINAPI *PathProvider)(LPTSTR, UINT))
{
    if ((*PathProvider)(pBuffer, cbBuffer) + strlenf(szPOLFILE) + 2 > cbBuffer)
        return FALSE;

    LPSTR pchBackslash = strrchrf(pBuffer, '\\');
    if (pchBackslash == NULL || *(pchBackslash+1) != '\0')
        strcatf(pBuffer, "\\");

    strcatf(pBuffer, szPOLFILE);

    return TRUE;
}


SID_IDENTIFIER_AUTHORITY siaNTAuthority = SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY siaWorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

HKEY CreateRegKeyNT(LPCSTR pszKey)
{
    HKEY hKey = NULL;
    LONG err = RegOpenKey(HKEY_LOCAL_MACHINE, pszKey, &hKey);
    if (err == ERROR_SUCCESS)
        return hKey;

    SECURITY_ATTRIBUTES sa;
    LPSECURITY_ATTRIBUTES lpsa;
    PSECURITY_DESCRIPTOR psd = NULL;
    PACL pACL = NULL;
    PSID psidAdmins = NULL;
    PSID psidWorld = NULL;
    UINT cbACL = 1024;                /* default ACL size */

    if (RunningOnNT()) {

        psd = (PSECURITY_DESCRIPTOR)MemAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (psd == NULL ||
            !InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION))
        {
            goto cleanup;
        }

        pACL = (PACL)MemAlloc(cbACL);
        if (pACL == NULL || !InitializeAcl(pACL, cbACL, ACL_REVISION2))
        {
            goto cleanup;
        }

        if (!AllocateAndInitializeSid(&siaNTAuthority,
                2,                                /* number of subauthorities */
                SECURITY_BUILTIN_DOMAIN_RID,    /* first subauthority: this domain */
                DOMAIN_ALIAS_RID_ADMINS,        /* second: admins local group */
                0, 0, 0, 0, 0, 0,                /* unused subauthorities */
                &psidAdmins))
        {
            goto cleanup;
        }

        if (!AllocateAndInitializeSid(&siaWorldAuthority,
                1,                                /* number of subauthorities */
                SECURITY_WORLD_RID,                /* first subauthority: all users */
                0, 0, 0, 0, 0, 0, 0,            /* unused subauthorities */
                &psidWorld))
        {
            goto cleanup;
        }

        if (!AddAccessAllowedAce(pACL, ACL_REVISION2, KEY_ALL_ACCESS, psidAdmins) ||
            !AddAccessAllowedAce(pACL, ACL_REVISION2, KEY_READ, psidWorld))
        {
            goto cleanup;
        }

        ACE_HEADER *pAce;

        /* Make both ACEs inherited by subkeys created later */
        if (GetAce(pACL, 0, (LPVOID *)&pAce))
            pAce->AceFlags |= CONTAINER_INHERIT_ACE;

        if (GetAce(pACL, 1, (LPVOID *)&pAce))
            pAce->AceFlags |= CONTAINER_INHERIT_ACE;

        if (!SetSecurityDescriptorDacl(psd,
            TRUE,            /* fDaclPresent */
            pACL,
            FALSE))            /* not a default discretionary ACL */
        {
            goto cleanup;
        }

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = psd;
        sa.bInheritHandle = FALSE;

        lpsa = &sa;
    }
    else
        lpsa = NULL;

    DWORD dwDisp;
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszKey, NULL, "",
                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, lpsa,
                       &hKey, &dwDisp) != ERROR_SUCCESS)
    {
        hKey = NULL;
    }

cleanup:
    if (lpsa != NULL) {
        FreeSid(psidAdmins);
        FreeSid(psidWorld);
        if (psd != NULL)
            MemFree(psd);
        if (pACL != NULL)
            MemFree(pACL);
    }

    return hKey;
}


HKEY OpenHiveFile(BOOL fCreate)
{
    RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);

    HKEY hkeyUser;

    char szPath[MAXPATHLEN+1];
    char szPath2[MAXPATHLEN+1];

    BuildPolName(szPath, sizeof(szPath), GetSystemDirectory);
    BuildPolName(szPath2, sizeof(szPath2), GetWindowsDirectory);


    // RegLoadKey is slow. Assume ratings disabled and check for file first
    // before doing RegLoadKey...
    LONG err=ERROR_FILE_NOT_FOUND;

    if (GetFileAttributes(szPath) != 0xFFFFFFFF)
    {
        err = RegLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA, szPath);
    }

    if (err != ERROR_SUCCESS && GetFileAttributes(szPath2) != 0xFFFFFFFF)
    {
        err = RegLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA, szPath2);
    }


    if (err == ERROR_SUCCESS) {
        err = RegOpenKey(HKEY_LOCAL_MACHINE, szPOLUSER, &hkeyUser);
        if (err == ERROR_SUCCESS)
            return hkeyUser;
    }

    if (!fCreate)
        return NULL;

    RegDeleteKey(HKEY_LOCAL_MACHINE,szTMPDATA);

    HKEY hkeyHive = NULL;

    if ((err=RegCreateKey(HKEY_LOCAL_MACHINE,szTMPDATA,&hkeyHive)) != ERROR_SUCCESS ||
        (err=RegCreateKey(hkeyHive,szUSERS,&hkeyUser)) != ERROR_SUCCESS) 
    {
        if (hkeyHive) RegCloseKey(hkeyHive);
        return NULL;
     }

    LPSTR pszPath = szPath;
    if ((err = RegSaveKey(hkeyHive, szPath, 0)) != ERROR_SUCCESS)
    {
        pszPath = szPath2;
        err = RegSaveKey(hkeyHive, szPath2, 0);
    }

    RegCloseKey(hkeyHive);
    RegCloseKey(hkeyUser);

    /* Now that we've saved the thing, we need to load it up again as a
     * hive file, so that it'll disappear completely when we unload later.
     */    
    MyRegDeleteKey(HKEY_LOCAL_MACHINE,szTMPDATA);

    if (err == ERROR_SUCCESS) {
        RegFlushKey(HKEY_LOCAL_MACHINE);
        if (RegLoadKey(HKEY_LOCAL_MACHINE, szTMPDATA, pszPath) == ERROR_SUCCESS)
            RegOpenKey(HKEY_LOCAL_MACHINE, szPOLUSER, &hkeyUser);
    }

    return hkeyUser;
}


BOOL PicsRatingSystemInfo::Init()
{
    PicsUser    *pPU;
#ifdef NASH
    int            z;
    char        rgBuf[PICS_FILE_BUF_LEN];    // BUGBUG - this is 20K of stack!
    UINT        uRet;
#endif
    BOOL        fRet = TRUE;
    BOOL        fIsNT;
    HKEY        hkeyUser = NULL;

    fRatingInstalled = FALSE;
    pUserObject = NULL;

    fIsNT = RunningOnNT();

    if (fIsNT)
    {
        hkeyUser = CreateRegKeyNT(szRATINGS);
        fStoreInRegistry = TRUE;
    }
    else {
        RegEntry re(szPOLICYKEY, HKEY_LOCAL_MACHINE);
        if (re.GetNumber(szPOLICYVALUE) != 0) {
            RegEntry reLogon(szLogonKey, HKEY_LOCAL_MACHINE);
            if (reLogon.GetNumber(szUserProfiles) != 0) {
                /* The ratings key has the supervisor password and maybe other
                 * settings.  To see if there are other settings here, try to
                 * find the user subkey (".Default").  If it's not there, we'll
                 * try a policy file.
                 */
                if (RegOpenKey(HKEY_LOCAL_MACHINE, szRATINGS, &hkeyUser) == ERROR_SUCCESS) {
                    HKEY hkeyTemp;
                    if (RegOpenKey(hkeyUser, szDefaultUserName, &hkeyTemp) == ERROR_SUCCESS) {
                        RegCloseKey(hkeyTemp);
                    }
                    else {
                        RegCloseKey(hkeyUser);
                        hkeyUser = NULL;
                    }
                }
            }
        }

        if (hkeyUser != NULL)
            fStoreInRegistry = TRUE;
        else {
            fStoreInRegistry = FALSE;
            hkeyUser = OpenHiveFile(FALSE);
        }
    }

    // read information from whatever key we opened
    if (hkeyUser != NULL)
    {

        //First load the rating files, then load the user names.
        fRatingInstalled = LoadProviderFiles(hkeyUser);

#ifdef NASH        
        // enumerate the subkeys, which will be user names
        z = 0;
        while ((uRet=RegEnumKey(hkeyUser,z,rgBuf,sizeof(rgBuf)))== ERROR_SUCCESS)
        {
               pPU = new PicsUser;
            pPU->ReadFromRegistry(hkeyUser, rgBuf); 
            arrpPU.Append(pPU);
            z++;
        }
        RegCloseKey(hkeyUser);
#else
        pPU = new PicsUser;
        if (pPU != NULL) {
            pPU->ReadFromRegistry(hkeyUser, (char *)szDefaultUserName);
            pUserObject = pPU;
        }
        else
            fRet = FALSE;
        RegCloseKey(hkeyUser);
#endif

        /* Make sure the user settings have defaults for all installed
         * rating systems.
         */
        for (int i=0; i<arrpPRS.Length(); i++)
            CheckUserSettings(arrpPRS[i]);
    }

    if (!fStoreInRegistry)
        RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);

    /* Check to see if there is a supervisor password set.  If there is,
     * but we have no settings, then someone's been tampering.
     */
    if (SUCCEEDED(VerifySupervisorPassword(NULL)) && !fRatingInstalled) {
        MyMessageBox(NULL, IDS_NO_SETTINGS, IDS_GENERIC, MB_OK | MB_ICONSTOP);
        fSettingsValid = FALSE;
    }
    else
        fSettingsValid = TRUE;

    return fRet;
}

BOOL PicsRatingSystemInfo::FreshInstall()
{
    PicsUser    *pPU;

    pPU = new PicsUser;
    if (pPU != NULL) {
        pPU->NewInstall();
#ifdef NASH
        arrpPU.Append(pPU);
#else
        pUserObject = pPU;
#endif
        fRatingInstalled = TRUE;    // we have settings now
        return TRUE;
    }
    else
        return FALSE;
}

extern HANDLE g_hsemStateCounter;   // created at process attatch time

long GetStateCounter()
{
    long count;

    ReleaseSemaphore(g_hsemStateCounter, 1, &count);    // poll and bump the count
    WaitForSingleObject(g_hsemStateCounter, 0);         // reduce the count
    return count;
}

void BumpStateCounter()
{
    ReleaseSemaphore(g_hsemStateCounter, 1, NULL);      // bump the count
}

// check the global semaphore count to see if we need to reconstruct our
// state. 

void CheckGlobalInfoRev(void)
{
    ENTERCRITICAL;

    if (gPRSI != NULL) {

        if (gPRSI->nInfoRev != GetStateCounter()) {
            delete gPRSI;
            gPRSI = new PicsRatingSystemInfo;
            if (gPRSI != NULL) {
                gPRSI->Init();
            }
            CleanupRatingHelpers();
            InitRatingHelpers();
        }
    }

    LEAVECRITICAL;
}

void PicsRatingSystemInfo::SaveRatingSystemInfo()
{
    int z;
    HKEY hkeyUser = NULL;
    char szFileName[MAXPATHLEN];
    BOOL fIsNT = RunningOnNT();

    if (!fSettingsValid || !fRatingInstalled)
        return;                /* ratings aren't installed, nothing to save */

    // load the hive file
    if (fStoreInRegistry) {
        RegCreateKey(HKEY_LOCAL_MACHINE, szRATINGS, &hkeyUser);
    }
    else {
        hkeyUser = OpenHiveFile(TRUE);
    }
    
    // read information from local registry
     if (hkeyUser != NULL)
    {
        if (etstrRatingBureau.fIsInit())
        {
            EtStringRegWrite(etstrRatingBureau, hkeyUser, (char *)szRATINGBUREAU);
        }
        else
        {
            RegDeleteValue(hkeyUser, szRATINGBUREAU);
        }    

        for (z = 0; z < arrpPRS.Length(); ++z)
        {
            wsprintf(szFileName, szFilenameTemplate, z);
            EtStringRegWrite(arrpPRS[z]->etstrFile, hkeyUser, szFileName);
        }

        // Delete the next one, as a safety precaution
        wsprintf(szFileName, szFilenameTemplate, z);
        RegDeleteValue(hkeyUser, szFileName);

#ifdef NASH
        for (z=0;z<arrpPU.Length();++z){
            arrpPU[z]->WriteToRegistry(hkeyUser);
        }
#else
        pUserObject->WriteToRegistry(hkeyUser);
#endif
        RegCloseKey(hkeyUser);
    }

    if (!fStoreInRegistry)
        RegUnLoadKey(HKEY_LOCAL_MACHINE,szTMPDATA);

    BumpStateCounter();
    nInfoRev = GetStateCounter();
}


HRESULT LoadRatingSystem(LPCSTR pszFilename, PicsRatingSystem **pprsOut)
{
    PicsRatingSystem *pPRS = new PicsRatingSystem;

    *pprsOut = pPRS;
    if (pPRS == NULL)
        return E_OUTOFMEMORY;

    UINT cbFilename = strlenf(pszFilename) + 1 + 1;    /* room for marker character */
    LPSTR pszNameCopy = new char[cbFilename];
    if (pszNameCopy == NULL)
        return E_OUTOFMEMORY;

    strcpyf(pszNameCopy, pszFilename);
    pPRS->etstrFile.SetTo(pszNameCopy);

    LPSTR pszMarker = strchrf(pszNameCopy, '*');
    if (pszMarker != NULL) {                /* ended in marker character... */
        ASSERT(*(pszMarker+1) == '\0');
        pPRS->dwFlags |= PRS_WASINVALID;    /* means it failed last time */
        *pszMarker = '\0';                    /* strip marker for loading */
    }

    HRESULT hres;

    HANDLE hFile = CreateFile(pszNameCopy, GENERIC_READ,
                              FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD cbFile = ::GetFileSize(hFile, NULL);
        BUFFER bufData(cbFile + 1);
        if (bufData.QueryPtr() != NULL) {
            LPSTR pszData = (LPSTR)bufData.QueryPtr();
            DWORD cbRead;
            if (ReadFile(hFile, pszData, cbFile, &cbRead, NULL)) {
                pszData[cbRead] = '\0';        /* null terminate whole file */

                hres = pPRS->Parse(pszFilename, pszData);
                if (SUCCEEDED(hres))
                {
                    pPRS->dwFlags |= PRS_ISVALID;
                }
            }
            else
                hres = HRESULT_FROM_WIN32(::GetLastError());

            CloseHandle(hFile);
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else
        hres = HRESULT_FROM_WIN32(::GetLastError());

    if (!(pPRS->dwFlags & PRS_ISVALID))
        strcatf(pszNameCopy, "*");            /* mark filename as invalid */

    return hres;
}
