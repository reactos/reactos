#ifndef _CDL_H_
#define _CDL_H_

// Zero impact ifdefs are scattered throughout urlmon\download
#define _ZEROIMPACT
#define MAX_DEBUG_STRING_LENGTH                    2048
#define MAX_DEBUG_FORMAT_STRING_LENGTH             1024
#define MAX_VERSIONLENGTH                          27  // sizeof(2DWORDS)/(log 10 base 2) + 3 (seperators) 
                                                       // == 64/3 + 3 == 25
// CDL.h
// Code Downloader header file
//
// Read "class descriptions" first for understanding how the
// code downloader works.

#define STRING(x) (((x) != NULL) ? (x) : (L"(null)"))

#include <safeocx.h>
#include "debmacro.h"
#include <msxml.h>
#include "wvtp.h"
//#include "..\inc\clist.hxx"
#ifndef unix
#include "..\utils\coll.hxx"
#else
#include "../utils/coll.hxx"
#endif /* !unix */
#include "packet.hxx"
#include "shlwapi.h"
#include "strids.h"

#include <pkgmgr.h>

#ifdef WX86
#ifdef __cplusplus            // make classes invisible to 'C'
// Support for multiple architectures during code download.  This must
// be declared before softdist.hxx can be included.
class CMultiArch {
public:
    CMultiArch() { m_RequiredArch = PROCESSOR_ARCHITECTURE_UNKNOWN; };
    DWORD   GetRequiredArch() { return m_RequiredArch;}
    HRESULT RequirePrimaryArch();
    HRESULT RequireAlternateArch();
    VOID    SelectArchitecturePreferences(
                char *szNativeArch,
                char *szIntelArch,
                char **pszPreferredArch,
                char **pszAlternateArch);

private:
    DWORD             m_RequiredArch;
};
#endif
#endif

#include "softdist.hxx"

#include <capi.h>

#define     MAX_REGSTR_LEN              1024

#define     DU_TAG_SOFTDIST             L"SOFTPKG"
#define     DU_TAG_NATIVECODE           L"msicd::NativeCode"
#define     DU_TAG_JAVA                 L"msicd::Java"
#define     DU_TAG_EXPIRE               L"msicd::Expire"

#define     DU_TAG_UNINSTALL_OLD        L"msicd::UninstallOld"
#define     INF_TAG_UNINSTALL_OLD       "UninstallOld"

#ifdef _ZEROIMPACT
#define     DU_TAG_ZEROIMPACT           L"msicd::ZeroImpact"
#endif
#define     DU_TAG_CODE                 L"Code"
#define     DU_TAG_CODEBASE             L"CodeBase"
#define     DU_TAG_PACKAGE              L"Package"
#define     DU_TAG_TITLE                L"TITLE"
#define     DU_TAG_ABSTRACT             L"ABSTRACT"
#define     DU_TAG_LANG                 L"LANGUAGE"
#define     DU_TAG_DEPENDENCY           L"Dependency"
#define     DU_TAG_PROCESSOR            L"Processor"
#define     DU_TAG_PLATFORM             L"Platform"
#define     DU_TAG_CONFIG               L"IMPLEMENTATION"
#define     DU_TAG_USAGE                L"Usage"
#define     DU_TAG_OS                   L"OS"
#define     DU_TAG_OSVERSION            L"OSVersion"
#define     DU_TAG_NAMESPACE            L"NameSpace"
#define     DU_TAG_DELETEONINSTALL      L"DeleteOnInstall"

#define     DU_ATTRIB_NAME              L"NAME"
#define     DU_ATTRIB_FILENAME          L"FILENAME"
#define     DU_ATTRIB_VALUE             L"VALUE"
#define     DU_ATTRIB_VERSION           L"VERSION"
#define     DU_ATTRIB_STYLE             L"STYLE"
#define     DU_ATTRIB_SIZE              L"SIZE"
#define     DU_ATTRIB_PRECACHE          L"PRECACHE"
#define     DU_ATTRIB_AUTOINSTALL       L"AUTOINSTALL"
#define     DU_ATTRIB_EMAIL             L"EMAIL"
#define     DU_ATTRIB_HREF              L"HREF"
#define     DU_ATTRIB_ACTION            L"ACTION"
#define     DU_ATTRIB_CLSID             L"CLASSID"
#define     DU_ATTRIB_DL_GROUP          L"GROUP"
#define     DU_ATTRIB_RANDOM            L"RANDOM"

#define     DU_STYLE_MSICD              "MSICD"
#define     DU_STYLE_ACTIVE_SETUP       "ActiveSetup"
#define     DU_STYLE_MSINSTALL          "MSInstall.SoftDist"
#define     DU_STYLE_LOGO3              "MSAppLogo5"

#define     DU_TAG_SYSTEM               L"System"

// Used by JAVA
#define     DU_TAG_NEEDSTRUSTEDSOURCE   L"NeedsTrustedSource"

#define     CHANNEL_ATTRIB_BASE         L"BASE"

#define     MAX_EXPIRE_DAYS             3650

// BUGBUG take out once in winnt.h
#ifndef IMAGE_FILE_MACHINE_OMNI
#define IMAGE_FILE_MACHINE_OMNI     0xACE1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "fdi.h"

#ifndef DEB_CODEDL
#define DEB_CODEDL   1
#endif

// JIT Window data

#define JIT_DIALOG_CLASS_NAME    "Internet Explorer_TridentDlgFrame"
#define JIT_DIALOG_CAPTION       "Internet Explorer Install on Demand"

// return value from the JIT setup page.
#define JITPAGE_RETVAL_SUCCESS              0x0     // successfully installed
#define JITPAGE_RETVAL_CANCELLED            0x1     // was cancelled by user
#define JITPAGE_RETVAL_DONTASK_THISWINDOW   0x2     // don;t ask again in this
                                                    // window
#define JITPAGE_RETVAL_DONTASK_EVER         0x3     // don't ask ever. user
                                                    // goes to addon page to
                                                    // install
#define JITPAGE_RETVAL_NEED_REBOOT          ERROR_SUCCESS_REBOOT_REQUIRED

// FaultInIEFeature flags
// urlmon.idl flag definition
// internal flags are here

#define FIEF_FLAG_CHECK_CIFVERSION      0x100       // checks if requested version
                                                    // can be installed by JIT

#define REGSTR_PATH_INFODEL_REST     "Software\\Policies\\Microsoft\\Internet Explorer\\Infodelivery\\Restrictions"
#define REGVAL_JIT_REST        "NoJITSetup"
#define REGVAL_UI_REST        "NoWinVerifyTrustUI"

// registry paths for ModuleUsage
#define REGSTR_PATH_SHAREDDLLS     "Software\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"

#define REGSTR_PATH_MODULE_USAGE   "Software\\Microsoft\\Windows\\CurrentVersion\\ModuleUsage"

#define REGSTR_PATH_CODE_STORE   "Software\\Microsoft\\Code Store Database"
#define REGSTR_PATH_DIST_UNITS   "Software\\Microsoft\\Code Store Database\\Distribution Units"
#define REGSTR_PATH_JAVA_PKGS   "Software\\Microsoft\\Code Store Database\\Java Packages"

#define REGSTR_PATH_IE_SETTINGS     "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define REGSTR_PATH_IE_MAIN     "Software\\Microsoft\\Internet Explorer\\Main"

#define REGSTR_PATH_LOGO3_SETTINGS  "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
#define REGVAL_LOGO3_MAJORVERSION   "VersionMajor"
#define REGVAL_LOGO3_MINORVERSION   "VersionMinor"
#define REGSTR_LOGO3_ADVERTISED_VERSION "AdvertisedVersion"
#define REGKEY_LOGO3_AVAILABLE_VERSION "AvailableVersion"

#define REGSTR_PATH_NT5_LOCKDOWN_TEST    "Software\\Microsoft\\Code Store Database\\NT5LockDownTest"
#define REGVAL_USE_COINSTALL             "UseCoInstall"

// If you modify this then make appropriate entries in urlmon\dll\selfreg.inx
// to clean this out on urlmon dlluninstall
// this key will be renamed before ship of each major release
// so we won't remember the rejected features in PP1 and not prompt for final
// release

#define REGKEY_DECLINED_COMPONENTS     "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Declined Components IE5"

#define REGKEY_DECLINED_IOD   "Software\\Microsoft\\Active Setup\\Declined Install On Demand IEv5"

#define REGKEY_ACTIVESETUP_COMPONENTS   "Software\\Microsoft\\Active Setup\\Installed Components"
#define REGKEY_ACTIVESETUP              "Software\\Microsoft\\Active Setup"
#define REGKEY_ACTIVESETUP_CLSIDFEATURE   "Software\\Microsoft\\Active Setup\\ClsidFeature"
#define REGKEY_ACTIVESETUP_MIMEFEATURE   "Software\\Microsoft\\Active Setup\\MimeFeature"
#define REGKEY_ACTIVESETUP_FEATURECOMPID   "Software\\Microsoft\\Active Setup\\FeatureComponentID"
#ifdef _ZEROIMPACT
#define REGKEY_ZEROIMPACT_DIRS "Software\\Microsoft\\Code Store Database\\Distribution Units"
#define DPF_DIRECTORYNAME TEXT("Downloaded Program Files")
#endif
#define REGVAL_VERSION_AVAILABLE        "Version available"
#define REGVAL_VERSION_ADVERTISED        "Version advertised"
#define REGVAL_ABSTRACT_AVAILABLE        "Abstract"
#define REGVAL_TITLE_AVAILABLE        "Title available"
#define REGVAL_HREF_AVAILABLE        "HREF available"
#define REGVAL_FIRST_HOME_PAGE        "First Home Page"
#define REGKEY_MSICD_ADVERTISED_VERSION "AdvertisedVersion"
#define REGVAL_ADSTATE "AdState"

#define DISTUNIT_NAME_IE4       "{89820200-ECBD-11cf-8B85-00AA005B4383}"


// Code Download Setup Flags
// DWORD  g_dwCodeDownloadSetupFlags = 0;
typedef enum {
    CDSF_INIT,
    CDSF_USE_SETUPAPI
} CodeDownloadSetupFlags;

// buffer size for downloads in CBSC::m_cbuffer
#define BUFFERMAX 2048

// File Name List
//
// used as pFilesToExtract to track files in the CAB we need extracted
//
// or a pFileList in PSESSION
//
// We keep track of all files that are in a cabinet
// keeping their names in a list and when the download
// is complete we use this list to delete temp files

struct sFNAME {
    LPSTR               pszFilename;
    struct sFNAME       *pNextName;
    DWORD               status; /* out */
};

typedef struct sFNAME FNAME;
typedef FNAME *PFNAME;

// SFNAME.status: success is 0 or non-zero error code in extraction
#define SFNAME_INIT         1
#define SFNAME_EXTRACTED    0

// FILE extentions we know about
typedef enum {
    FILEXTN_NONE,
    FILEXTN_UNKNOWN,
    FILEXTN_CAB,
    FILEXTN_DLL,
    FILEXTN_OCX,
    FILEXTN_INF,
    FILEXTN_EXE,
    FILEXTN_OSD,
    FILEXTN_CAT
} FILEXTN;


//
// Master State Information for File Extraction: used by extract.c
//

typedef struct {
    UINT        cbCabSize;
    ERF         erf;
    PFNAME      pFileList;              // List of Files in CAB
    UINT        cFiles;
    DWORD       flags;                  // flags: see below for list
    char        achLocation[MAX_PATH];  // Dest Dir
    char        achFile[MAX_PATH];      // Current File
    char        achCabPath[MAX_PATH];   // Current Path to cabs
    PFNAME      pFilesToExtract;        // files to extract;null=enumerate only

} SESSION, *PSESSION;

typedef enum {
    SESSION_FLAG_NONE           = 0x0,
    SESSION_FLAG_ENUMERATE      = 0x1,
    SESSION_FLAG_EXTRACT_ALL    = 0x2,
    SESSION_FLAG_EXTRACTED_ALL  = 0x4
} SESSION_FLAGS;

typedef struct CodeDownloadDataTag {
    LPCWSTR      szDistUnit;       // Distribution unit to look for
    LPCWSTR      szClassString;    // clsid (or class string) of object desired
    LPCWSTR      szURL;            // codebase to download
    LPCWSTR      szMimeType;       // mime type (com translate -> clsid)
    LPCWSTR      szExtension;     // extension (com translate -> clsid)
    LPCWSTR      szDll;           // dll to load object from after download (for zero impact)
    DWORD       dwFileVersionMS;  // file version
    DWORD       dwFileVersionLS;
    DWORD       dwFlags;          // flags
} CodeDownloadData;

#define    chPATH_SEP1            '\\'
#define    chPATH_SEP2            '/'
#define    chDRIVE_SEP            ':'

// size of [Add.Code] section in INF in bytes
#define MAX_INF_SECTIONS_SIZE       1024

// from extract.c
HRESULT Extract(PSESSION psess, LPCSTR lpCabName);
VOID DeleteExtractedFiles(PSESSION psess);
BOOL catDirAndFile(
    char    *pszResult, 
    int     cbResult, 
    char    *pszDir,
    char    *pszFile);

#ifdef __cplusplus
}
#endif

#define CHECK_ERROR_EXIT(cond, iResID) if (!(cond)) { \
        if (iResID) \
            CodeDownloadDebugOut(DEB_CODEDL, TRUE, iResID); \
        goto Exit;}

// Download states that a CDownload passes thru from obj creation to done.
typedef enum {
    DLSTATE_INIT,                  // obj constructed
    DLSTATE_BINDING,               // download in progress
    DLSTATE_DOWNLOADED,            // at begining of OnStopBinding
    DLSTATE_EXTRACTING,            // begin CAB extraction (where applicable)
    DLSTATE_INF_PROCESSING,        // begin ProcessInf (where applicable)
    DLSTATE_READY_TO_SETUP,        // end of OnStopBinding
    DLSTATE_SETUP,                 // Start DoSetup
    DLSTATE_DONE,                  // all done, ready to free obj,
                                   // delete temp files
    DLSTATE_ABORT                  // aborted this download
} DLSTATE;

// INSTALL_STATE used to give BINDSTATUS_INSTALLING_COMPONENTS OnProgress
// during setup phase
// given as INSTALL_COPY of INSTALL_PHASES, szStatusText pointing to filename

typedef enum {
    INSTALL_INIT = 0,
    INSTALL_VERIFY = 1,
    INSTALL_COPY = 2,
    INSTALL_REGISTER =3,
    INSTALL_PHASES = 4
} INSTALL_STATE;

#define INSTALL_DONE INSTALL_PHASES


// directions to CSetup on dest dir for file if no prev version exists
typedef enum {

    LDID_OCXCACHE=0,                // ocxcache dir, now = windows\ocxcache
    LDID_WIN=10,                    // INF convetion to mean windows dir
    LDID_SYS=11                     // INF convetion to mean sysdir

} DESTINATION_DIR ;


// Software distribution styles
typedef enum {
    STYLE_UNKNOWN = -1,             // Unknown otherwise.
    STYLE_MSICD = 1,
    STYLE_ACTIVE_SETUP,
    STYLE_LOGO3,
    STYLE_MSINSTALL,                // Darwin
    STYLE_OTHER                     // Provides own ISoftDistExt interface for access
};

#ifdef __cplusplus            // make classes invisible to 'C'

#include "langcode.h"

// <Config> tag processor.  Shared by Code Download and CSoftDist.
HRESULT ProcessImplementation(IXMLElement *pConfig,
                              CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList,
                              LCID lcidOverride,
#ifdef WX86
                              CMultiArch *MultiArch,
#endif
                              LPWSTR szBaseURL = NULL);
HRESULT ProcessCodeBaseList(IXMLElement *pCodeBase,
                            CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList,
                            LPWSTR szBaseURL = NULL);

#ifdef _ZEROIMPACT
// Duplicates CoGetClassObject, but gets objects from the zeroimpact installations
// Defined in cdlapi.cxx
HRESULT ZIGetClassObject(LPCWSTR wszDistUnit, LPCOLESTR wszClassString, DWORD dwFileVersionMS, 
                         DWORD dwFileVersionLS, REFIID riid, void **ppv);
// Gets the installation firectory for the given distunit and version
// Defined in cdlapi.cxx
HRESULT ZIGetInstallationDir(LPCWSTR wszDistUnit, DWORD *pdwFileVersionMS, 
                             DWORD *pdwFileVersionLS, LPTSTR szBuffer, int * cBufLen);
// Gets the dll name for the given class id in the given dist unit's given directory
// Defined in cdlapi.cxx
HRESULT ZIGetDllName(LPCTSTR szDir, LPCWSTR wszDistUnit, LPCOLESTR wszClassString, DWORD dwFileVersionMS, 
             DWORD dwFileVersionLS, LPTSTR szBuffer, int * pcBufLen);
// Useful helper function to get a class object from a dll name and a class string
// Defined in helpers.cxx
HRESULT
GetClassObjectFromDll(LPCWSTR wszClass, LPWSTR wszDll, REFIID riid,
                      DWORD grfBINDF, void **ppv);

// Get the zeroimpact root dir
LPCTSTR GetZeroImpactRootDir();
#endif

// %%Classes: ----------------------------------------------------------------

/*
 *  class descriptions
 *
 *  CCodeDownload (main class tracking as a whole)
 *  It has the client's BSC and creates a CClBinding for client.
 *
 *  CClBinding (For client's IBinding for code download)
 *
 *  CDownload (tracks individual downloads) implements a
 *
 *  CBindStatusCallback (BSC)
 *
 *  Csetup obj: zero or more associated with every CDownload obj
 *  Some CDownload's may have no CSetup (eg. INF file)
 *
 *
 *
 *
 * We do our code download in 2 stages.
 * 1) download stage
 * 2) setup and registeration stage
 *
 * CCodeDownload is the main class for codedownload.
 * AsyncGetClassBits() the entry into the Code downloader creates this obj
 * for the given CODE, CLSID, FileVersion, BSC (from BindCtx)
 * we do not check to see if a code download is already in progress
 * in the system at a given moment. Nor we do we keep track of individual
 * downloads and possible clashes between various silmultaneous code
 * downloads system wide. We leave it to URL moniker (above us) to ensure
 * that duplicate calls are not made into AsynGetClassBits. The second
 * problem of different code downloads trying to bring down a common
 * dependent DLL is POSTPONED to version 2 implementation.
 *
 * The CodeDownload obj once created is asked to perform its function
 * thru CCodeDownload::DoCodeDownload().
 * This triggers creation of the first CDownload object for the CODE url
 * if a local check for CLSID,FileVersion returns update_needed.
 * (note : it is interesting to note here that if a control needs to just
 * update a dependent DLL file it still needs to update the FileVersion
 * of the primary control file (with CLSID implementation) for triggering
 * any download at all!
 *
 * Once DoCodeDownload determines that an update is in order it creates
 * a CClBinding for its client to call client BSC::OnstartBinding with.
 *
 * It then adds this CDownload obj to its list of downloads.
 *
 * If the m_url is a CAB or INF we need to download it before we know
 * what we need to do next. Otherwise we create a CSetup obj for the
 * download and add it to CDownload's list of pending Setup processing for
 * stage 2 (setup and registeration). CSetup details later.
 *
 * CDownload is the basic download obj. It's action entry point is DoDownload
 * Here it creates a URL moniker for the given m_url and a bind ctx to go
 * with it and then calls pmk->BindToStorage to get the bits. Note how we
 * use URL mon's services to get the bits even as URLmon is our client for
 * the Code Download. We are its client for individual downloads. CDownload
 * has a BSC implementation to track progress and completion. This BSC is
 * where the magic of taking us from one state to next occurs.
 *
 * BSC::OnProgress
 * Here we get the master CodeDownload obj to collate progress and report
 * cumulative code download progress to client BSC::OnProgress.
 *
 * BSC::OnDataAvailable
 * At the last notification we get the filename URLmon has downloaded the
 * m_url data to and rename it to a file in the temp dir.
 *
 * BSC:: OnStopBinding
 * we get here when we have fully downloaded 'this'. this is the place
 * to call into WinVerifyTrust api if appropriate
 * This triggers a change state in our state machine. Depending on the
 * obj we have downloaded (a CAB or INF or DLL/OCX/EXE) we:
 *
 * OCX:
 *    Csetup for this download is usually previously created
 *      mark this download as done and
 *    call into main CodeDownload::CompleteOne (state analyser)
 *
 * CAB:
 *    if we don't have an INF already we look for one in the CAB
 *           if INF in CAB
 *               process INF (may trigger further extractions/downloads/Csetup)
 *           else
 *              look for primary OCX in CAB and create CSetup or it.
 *
 * INF:
 *      Process INF
 *
 * CCodeDownload::CompleteOne is called whenever a CDownload obj completes
 * its download and initiates further downloads if necessary (eg. ProcessInf)
 * It does nothing until all pending downloads are complete. Until then it
 * just returns and we unwind back to BSC::OnStopBinding
 *
 * When all downloads completed, we then start processingall the Csetups
 * We do this code download in two stages to
 * keep capability to back out of entire code download for as late as we can
 * until the setup stage calling CClBinding::Abort with IBinding returned by
 * code downloader in client's BSC::OnStartBinding will cleanly abort and
 * restore initial state.
 * We don't honor Abort once in setup stage.
 *
 * To keep this stage as clean and failsafe as we can we check for
 * disk space in the OCX cache as well as check for IN USE OCXes that we
 * plan on updating. We abort on either of these two conditions.
 *
 * CCodeDownload::CompleteOne than proceeds to walk thru all its download objs
 * calling DoSetup which in turn causes CSetup::DoSetup() to get invoked
 * for every CSetup.
 *
 * In the guts of Csetup we move the temp OCX file to the dest dir ( usually
 * OCXCACHE dir unless upgrading over previous version), and we call
 * register OCX (if version info suggests so)
 *
 * When done with the setup stage CompleteOne calls client's BSC::OnStopBinding
 * and then frees all memory and clens up temp files.
 *
 */

class CLocalComponentInfo {
    public:

    CLocalComponentInfo();
    ~CLocalComponentInfo();
    HRESULT MakeDestDir();

    BOOL IsPresent() { return (dwLocFVMS | dwLocFVLS); }
#ifdef _ZEROIMPACT
    BOOL IsZI() { return m_bIsZI;}
#endif
    BOOL IsLastModifiedTimeAvailable() { 
        return ( ftLastModified.dwHighDateTime | ftLastModified.dwLowDateTime);
        }
    FILETIME * GetLastModifiedTime() {
        return IsLastModifiedTimeAvailable()?&ftLastModified:NULL;
        }

    LPSTR GetLocalVersionEtag() { return pbEtag;}

    LCID GetLocalVersionLCID() { return lcid; }

    // data members

    char            szExistingFileName[MAX_PATH];
    LPSTR           pBaseExistingFileName;
    LPSTR           lpDestDir;
    DWORD           dwLocFVMS;
    DWORD           dwLocFVLS;
    FILETIME        ftLastModified;
    LPSTR           pbEtag;
    LCID            lcid;
    BOOL            bForceLangGetLatest;
    DWORD           dwAvailMS;
    DWORD           dwAvailLS;
#ifdef _ZEROIMPACT
    BOOL            m_bIsZI;
    WCHAR           wszDllName[MAX_PATH];
#endif
};

// CModuleUsage: created for every module added to ModuleUsage
// is walked thru and run after all setups are complete in COmpleteAll
// can also be used to optionally rollback a setup


class CModuleUsage {
  public:

    // constructor
    CModuleUsage(LPCSTR lpFileName, DWORD dwFlags, HRESULT *phr);
    ~CModuleUsage();

    HRESULT Update(LPCSTR szClientName);
    LPCSTR GetFileName() {return m_szFileName;}

    // data members

    LPSTR              m_szFileName;
    DWORD              m_dwFlags;
};

// CSetup: created for every setup item
// each CDownload has zero or more of these linked into a list

typedef enum {

                                            // the default behaviour for
                                            // registering a server is to do
                                            // so only when the version rsrc
                                            // has the string "OleSelfregister"
                                            // the user can override this
                                            // behaviour with setting in .INF
                                            // registerserver=yes/no

    CST_FLAG_REGISTERSERVER_OVERRIDE=1,      // when set user has overriden 
    CST_FLAG_REGISTERSERVER=2                // when set along with above
                                            // means user wants us to register
                                            // server

} CST_FLAGS;

class CDownload;

typedef enum {
    CJS_FLAG_INIT=0,
    CJS_FLAG_NOSETUP=1,                     // don't setup just mark that this
                                            // java package is used by this dist
                                            // unit.
    CJS_FLAG_SYSTEM=2,                      // system class
    CJS_FLAG_NEEDSTRUSTEDSOURCE=4,           // need trusted source
};

class CJavaSetup {
    public:

    CJavaSetup(
        CDownload *pdl,
        LPCWSTR szPackageName,
        LPCWSTR szNameSpace,
        IXMLElement *pPackage,
        DWORD dwVersionMS,
        DWORD dwVersionLS,
        DWORD flags,
        HRESULT *phr);

    ~CJavaSetup();

    HRESULT DoSetup();

    INSTALL_STATE GetState() const { return m_state;}
    VOID SetState(INSTALL_STATE state) { m_state = state;}

    LPCWSTR GetPackageName() { return m_szPackageName; }
    LPCWSTR GetNameSpace() { return m_szNameSpace; }
    void GetPackageVersion(DWORD &dwVersionMS, DWORD &dwVersionLS) {
        dwVersionMS = m_dwVersionMS; 
        dwVersionLS = m_dwVersionLS; 
    }
    DWORD GetPackageFlags() { return m_flags; }
    IXMLElement *GetPackageXMLElement() { return m_pPackage; }

    private:

    INSTALL_STATE      m_state;            // state of install operation
    CDownload*         m_pdl;
    LPWSTR             m_szPackageName;
    LPWSTR             m_szNameSpace;
    IXMLElement *      m_pPackage;
    DWORD              m_dwVersionMS;
    DWORD              m_dwVersionLS;
    DWORD              m_flags;
};

class CSetup {

  public:

    HRESULT DoSetup(CCodeDownload *pcdl, CDownload *pdl);

    LPCSTR GetSrcFileName() const {return m_pSrcFileName;}

    HRESULT SetSrcFileName(LPCSTR pSrcFileName);

    FILEXTN GetExtn() const {return m_extn;}

    CSetup *GetNext() const { return m_pSetupnext;}
    VOID SetNext(CSetup *pSetupnext) { m_pSetupnext = pSetupnext;}

    INSTALL_STATE GetState() const { return m_state;}
    VOID SetState(INSTALL_STATE state) { m_state = state;}

    LPCSTR GetBaseFileName() const { return m_pBaseFileName; }

    // constructor
    CSetup(LPCSTR pSrcFileName, LPCSTR pBaseFileName, FILEXTN extn, LPCSTR pDestDir, HRESULT *phr, DESTINATION_DIR dest = LDID_OCXCACHE);
    ~CSetup();

    HRESULT GetDestDir(CCodeDownload *pcdl, LPSTR szDestDir);

    HRESULT CheckForNameCollision(CCodeDownload *pcdl, LPCSTR szCacheDir);

    HRESULT InstallFile(
        CCodeDownload *pcdl,
        LPSTR szDestDir,
        LPWSTR szStatusText,
        LPUINT pcbStatusText);

    VOID SetCopyFlags(DWORD dwcf) {
        m_advcopyflags = dwcf;
    }

    VOID SetUserOverrideRegisterServer(BOOL fRegister) {
        m_flags |= CST_FLAG_REGISTERSERVER_OVERRIDE;
        if (fRegister) {
            m_flags |= CST_FLAG_REGISTERSERVER;
        }
    }

    BOOL UserOverrideRegisterServer() {
        return (m_flags & CST_FLAG_REGISTERSERVER_OVERRIDE); 
    }

    BOOL WantsRegisterServer() {
        return (m_flags & CST_FLAG_REGISTERSERVER); 
    }

#ifdef _ZEROIMPACT
    BOOL IsZeroImpact() {return m_bIsZeroImpact;}
    void SetZeroImpact(BOOL bZI) {m_bIsZeroImpact = bZI;}
#endif

    void SetExactVersion(BOOL bFlag) {m_bExactVersion = bFlag;}

  private:

    CSetup*            m_pSetupnext;

    LPSTR              m_pSrcFileName;     // fully qualified src file name
    LPSTR              m_pBaseFileName;    // base filename
    FILEXTN            m_extn;

    LPCSTR             m_pExistDir;        // dest dir for setting up obj
                                           // if null default to ocxcache dir

    INSTALL_STATE      m_state;            // state of install operation

    DESTINATION_DIR    m_dest;

    DWORD              m_flags;            // overrides for register server
    DWORD              m_advcopyflags;     // flags for AdvInstallFile
#ifdef _ZEROIMPACT
    BOOL               m_bIsZeroImpact;
#endif

    BOOL m_bExactVersion;
};

// CClBinding to pass to client of CodeDownload in client's BSC::OnStartBinding

class CClBinding : public IBinding {

  public:

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IBinding methods
    STDMETHOD(Abort)( void);
    STDMETHOD(Suspend)( void);
    STDMETHOD(Resume)( void);
    STDMETHOD(SetPriority)(LONG nPriority);
    STDMETHOD(GetPriority)(LONG *pnPriority);
    STDMETHOD(GetBindResult)(CLSID *pclsidProtocol, DWORD *pdwResult, LPWSTR *pszResult,DWORD *pdwReserved);


    CClBinding::CClBinding(
        CCodeDownload *pcdl,
        IBindStatusCallback *pAssClientBSC,
        IBindCtx *pAssClientBC,
        REFCLSID rclsid,
        DWORD dwClsContext,
        LPVOID pvReserved,
        REFIID riid,
        IInternetHostSecurityManager* m_pHostSecurityManager);

    ~CClBinding();

    HRESULT InstantiateObjectAndReport(CCodeDownload *pcdl);

    REFCLSID GetClsid() const { return m_clsid;}

    HRESULT ReleaseClient();

    DWORD GetState() const { return m_dwState;}
    VOID SetState(DWORD dwState) { m_dwState = dwState;}

    IBindStatusCallback* GetAssBSC() {return m_pAssClientBSC;}

    IBindCtx* GetAssBC() {return m_pAssClientBC;}

    ICodeInstall* GetICodeInstall(); // side-effect: sets m_pCodeInstall!
    IWindowForBindingUI* GetIWindowForBindingUI(); // side-effect: sets m_pWindowForBindingUI!

    IBindHost* GetIBindHost(); // side-effect: sets m_pBindHost!

    IInternetHostSecurityManager* GetHostSecurityManager();

    HWND GetHWND(REFGUID rguidReason = IID_ICodeInstall);

    HRESULT SetClassString(LPCWSTR pszClassString);
    const LPWSTR GetClassString();

    void SetZIDll(LPCWSTR pszDll);
    const LPWSTR GetZIDll();

    private:

    CLSID                m_clsid;
                                                // foll: for CoGetClassObject
    DWORD                m_dwClsContext;        // CLSCTX flags
    LPVOID               m_pvReserved;          // Must be NULL
    REFIID               m_riid;                // Usually IID_IClassFactory


    DWORD                m_cRef;
    LONG                 m_nPriority;     // priority of this binding
    DWORD                m_dwState;       // state of operation
    CCodeDownload*       m_pcdl;
    IBindStatusCallback* m_pAssClientBSC; // associated Client BSC
    IBindCtx*            m_pAssClientBC;  // associated client bind ctx


    IBindHost*           m_pBindHost;        // IBindHost

    IWindowForBindingUI* m_pWindowForBindingUI; // IWindowForBindingUI
                                                // passed in by client
                                                // to pass in hwnd for
                                                // WinVerifyTrust as well

    IInternetHostSecurityManager* m_pHostSecurityManager;

    ICodeInstall*        m_pCodeInstall;        // ICodeInstall
                                                // passed in by client
                                                // to pass in hwnd for
                                                // WinVerifyTrust as well
                                                // as handle module update
                                                // contingencies like
                                                // out of disk space and
                                                // permission to update
                                                // existing file with newer ver

    HWND                 m_hWnd;                // client hwnd obtained thru
                                                // ICodeInstall
                                                // ::NeedVerificationUI
                                                // safe to cache this?
    LPWSTR               m_wszClassString;
    LPWSTR               m_wszZIDll;
};


// CodeDownload states 
typedef enum
{
    CDL_NoOperation = 0,            // operation did not start yet
    CDL_Downloading,                // downloading in progress
    CDL_Suspend,                    // operation suspended
    CDL_Aborted,                    // operation aborted
    CDL_ReadyToSetup,               // ready to setup
    CDL_Setup,                      // setup in progress
    CDL_SetupDone,                  // setup done
    CDL_Completed                   // done, and complete signalled CompleteAll
} CDL_STATE;

// values for CCodeDownload::m_flags
//   these will be defined in objbase.h. For now make sure they are defined so we don't break the build
#ifndef CD_FLAGS_DEFINED
#define CD_FLAGS_DEFINED
typedef enum {
    CD_FLAGS_INIT =                     0x0,
    CD_FLAGS_FORCE_DOWNLOAD =           0x1,
    CD_FLAGS_PEEK_STATE =               0x2,
    CD_FLAGS_NEED_CLASSFACTORY =        0x4,
    CD_FLAGS_PARANOID_VERSION_CHECK =   0x8,
    CD_FLAGS_SKIP_DECLINED_LIST_CHECK = 0x20,
    CD_FLAGS_USE_CODEBASE_ONLY      =   0x80,  // Set by OLE32 Class Store
    CD_FLAGS_HINT_JAVA               =  0x100, // to turn off advance
                                               // disabling of code download
                                               // if ActiveX Signed+Unsigned 
                                               // is off unless this flag is
                                               // passed any distunit that looks
                                               // like a clsid will not code
                                               // download if ActiveX is off
    CD_FLAGS_HINT_ACTIVEX            =  0x200,
    CD_FLAGS_FORCE_INTERNET_DOWNLOAD =  0x400, // For OLE32 CoInstall

    // below are internal flags

    CD_FLAGS_WAITING_FOR_EXE =          0x40,
    CD_FLAGS_SILENTOPERATION =          0x800,
    CD_FLAGS_TRUST_SOME_FAILED =        0x40000,
    CD_FLAGS_NEED_REBOOT =              0x1000,
    CD_FLAGS_BITS_IN_CACHE =            0x2000,
    CD_FLAGS_NEW_CONTEXT_MONIKER =      0x4000,
    CD_FLAGS_FAKE_SUCCESS =             0x8000,
    CD_FLAGS_DELETE_EXE =               0x10000,
    CD_FLAGS_UNSAFE_ABORT =             0x20000,
    CD_FLAGS_USER_CANCELLED =           0x40000,
    CD_FLAGS_HAVE_INF =                 0x80000,
    CD_FLAGS_ONSTACK =                  0x100000,
    CD_FLAGS_USED_CODE_URL =            0x200000,
    CD_FLAGS_EXACT_VERSION =            0x400000
    

} CD_FLAGS;

#endif // CD_FLAGS_DEFINED

#define CD_FLAGS_EXTERNAL_MASK      (CD_FLAGS_FORCE_DOWNLOAD| \
                                     CD_FLAGS_PEEK_STATE| \
                                     CD_FLAGS_NEED_CLASSFACTORY| \
                                     CD_FLAGS_PARANOID_VERSION_CHECK| \
                                     CD_FLAGS_SKIP_DECLINED_LIST_CHECK| \
                                     CD_FLAGS_USE_CODEBASE_ONLY| \
                                     CD_FLAGS_HINT_JAVA| \
                                     CD_FLAGS_HINT_ACTIVEX| \
                                     CD_FLAGS_FORCE_INTERNET_DOWNLOAD)

class CDownload;
class DebugLogElement;
class CDLDebugLog;

// main class
class CCodeDownload {

    public:

    // constructor
    CCodeDownload(
        LPCWSTR szDistUnit,
        LPCWSTR szURL,
        LPCWSTR szType, 
        LPCWSTR szExt, 
        DWORD dwFileVersionMS,
        DWORD dwFileVersionLS,
        HRESULT *phr);

    ~CCodeDownload();

    HRESULT DoCodeDownload(
        CLocalComponentInfo *plci,
        DWORD flags);

    HRESULT CCodeDownload::CreateClientBinding(
        CClBinding **ppClientBinding,
        IBindCtx* pClientBC,
        IBindStatusCallback* pClientbsc,
        REFCLSID rclsid,
        DWORD dwClsContext,
        LPVOID pvReserved,
        REFIID riid,
        BOOL fAddHead,
        IInternetHostSecurityManager *pHostSecurityManager);

    int GetCountClientBindings() const {
        return m_pClientbinding.GetCount();
    }

    CClBinding* GetClientBinding() const {
        return m_pClientbinding.GetHead();
    }

    IBindStatusCallback* GetClientBSC() const {
        return (GetClientBinding())->GetAssBSC();
    }

    IBindCtx* GetClientBC() const {
        return (GetClientBinding())->GetAssBC();
    }

    REFCLSID GetClsid() const {
        return (GetClientBinding())->GetClsid();
    }

    CDownload* GetDownloadHead() const {
        return m_pDownloads.GetHead();
    }

    VOID AddDownloadToList(CDownload *pdl);

    HRESULT FindDupCABInThread(IMoniker *pmk, CDownload **ppdlMatch);
    HRESULT FindCABInDownloadList(LPCWSTR szURL, CDownload *pdlHint, CDownload **ppdlMatch);

    VOID CompleteOne(CDownload *pdl, HRESULT hrOSB, HRESULT hrStatus, HRESULT hrResponseHdr, LPCWSTR szError);

    VOID CompleteAll(HRESULT hr, LPCWSTR szError);

    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    HRESULT ProcessHooks(CDownload *pdl);
    HRESULT ProcessHookSection(LPCSTR lpCurHook, CDownload *pdl);

    HRESULT GetSatelliteName( LPSTR lpCurCode);

    BOOL IsSectionInINF(LPCSTR lpCurCode);

    HRESULT GetInfCodeLocation( LPCSTR lpCurCode, LPSTR szURL);

    HRESULT GetInfSectionInfo(
        LPSTR lpCurCode,
        LPSTR szURL,
        LPCLSID *plpClsid,
        LPDWORD pdwFileVersionMS,
        LPDWORD pdwFileVersionLS,
        DESTINATION_DIR *pdest,
        LPDWORD pdwRegisterServer,
        LPDWORD pdwCopyFlags,
        BOOL *pbDestDir
        );

    HRESULT SetupInf(const char *szInf, char *szInfBaseName,CDownload *pdl);

    VOID ProcessInf(CDownload *pdl);

    HRESULT ParseOSD(const char *szOSD, char *szOSDBaseName, CDownload *pdl);

    HRESULT QueueModuleUsage( LPCSTR szFileName, LONG muFlags);
    HRESULT UpdateModuleUsage();

    HRESULT StartDownload(
        LPSTR szCurCode,
        CDownload *pdl,
        LPSTR szURL,
        DESTINATION_DIR dest,
        LPSTR szDestDir,
        DWORD dwRegisterServer,
        DWORD dwCopyFlags,
        CList <CCodeBaseHold *, CCodeBaseHold *> *pcbhList = NULL);

    BOOL HaveManifest() {return (HaveInf() || GetOSD());}

    BOOL NeedToReboot() const {return (m_flags & CD_FLAGS_NEED_REBOOT);}
    VOID SetRebootRequired() {m_flags |= CD_FLAGS_NEED_REBOOT;}

    BOOL IsSilentMode() const {return (m_flags & CD_FLAGS_SILENTOPERATION);}
    VOID SetSilentMode() {m_flags |= CD_FLAGS_SILENTOPERATION;}

    BOOL IsAllTrusted() const {return ((m_flags & CD_FLAGS_TRUST_SOME_FAILED) == 0);}
    VOID SetTrustSomeFailed() {m_flags |= CD_FLAGS_TRUST_SOME_FAILED;}

    BOOL ForceDownload() const {return (m_flags & CD_FLAGS_FORCE_DOWNLOAD);}

    BOOL HaveInf() const {return (m_flags & CD_FLAGS_HAVE_INF);}
    VOID SetHaveInf() {m_flags |= CD_FLAGS_HAVE_INF;}

    BOOL UsedCodeURL() const {return (m_flags & CD_FLAGS_USED_CODE_URL);}
    VOID SetUsedCodeURL() {m_flags |= CD_FLAGS_USED_CODE_URL;}

    BOOL SafeToAbort() const {return ((m_flags & CD_FLAGS_UNSAFE_ABORT) == 0);}
    VOID SetUnsafeToAbort() {m_flags |= CD_FLAGS_UNSAFE_ABORT;}

    BOOL BitsInCache() const {return (m_flags & CD_FLAGS_BITS_IN_CACHE);}
    VOID SetBitsInCache() {m_flags |= CD_FLAGS_BITS_IN_CACHE;}

    BOOL FakeSuccess() const {return (m_flags & CD_FLAGS_FAKE_SUCCESS);}
    VOID SetFakeSuccess() {m_flags |= CD_FLAGS_FAKE_SUCCESS;}

    HRESULT HandleUnSafeAbort();

    BOOL NeedObject() const {return (m_flags & CD_FLAGS_NEED_CLASSFACTORY);}
    VOID SetNeedObject(DWORD fl) { m_flags |= (fl & CD_FLAGS_NEED_CLASSFACTORY);}

    BOOL UseCodebaseOnly() const {return (m_flags & CD_FLAGS_USE_CODEBASE_ONLY);}
    VOID SetUseCodebaseOnly(DWORD fl) { m_flags |= (fl & CD_FLAGS_USE_CODEBASE_ONLY);}

    BOOL RelContextMk() {return (m_flags & CD_FLAGS_NEW_CONTEXT_MONIKER);}
    VOID MarkNewContextMoniker() {m_flags |= CD_FLAGS_NEW_CONTEXT_MONIKER;}

    BOOL WaitingForEXE() {return (m_flags & CD_FLAGS_WAITING_FOR_EXE);}
    VOID SetNotWaitingForEXE() {m_flags &= ~CD_FLAGS_WAITING_FOR_EXE;}

    BOOL UserCancelled() {return (m_flags & CD_FLAGS_USER_CANCELLED);}
    VOID SetUserCancelled() {m_flags |= CD_FLAGS_USER_CANCELLED;}

    BOOL IsOnStack() const {return (m_flags & CD_FLAGS_ONSTACK);}
    BOOL SetOnStack() {
            if (IsOnStack()) {
                return FALSE;
            } else {
                m_flags |= CD_FLAGS_ONSTACK;
                return TRUE;
            }
        }

    VOID ResetOnStack() {m_flags &= ~CD_FLAGS_ONSTACK;}

    BOOL SkipDeclinedListCheck() const {return (m_flags & CD_FLAGS_SKIP_DECLINED_LIST_CHECK);}

    BOOL DeleteEXEWhenDone() {return (m_flags & CD_FLAGS_USER_CANCELLED);}
    VOID SetDeleteEXEWhenDone() {m_flags |= CD_FLAGS_USER_CANCELLED;}
    VOID ResetDeleteEXEWhenDone() {m_flags &= ~CD_FLAGS_USER_CANCELLED;}

    HRESULT SetWaitingForEXE(LPCSTR szStatusText, BOOL bDeleteEXEWhenDone);
    VOID SetWaitingForEXEHandle(HANDLE hEXE) {m_pi.hProcess = hEXE;}

    LPSTR GetDestDirHint() const { return m_plci->lpDestDir;}
    BOOL LocalVersionPresent() const { return m_plci->IsPresent();}
    FILETIME * GetLastModifiedTime() { return m_plci->GetLastModifiedTime();}

    VOID InitLastModifiedFromDistUnit();

    IMoniker* GetContextMoniker() const {return m_pmkContext;}
    VOID SetContextMoniker(IMoniker* pmk) {m_pmkContext = pmk;}

    ICodeInstall* GetICodeInstall() const { 
        return GetClientBinding()->GetICodeInstall();
    }

    LPCWSTR GetMainURL() const { return m_url;}
    LPCWSTR GetMainDistUnit() const { return m_szDistUnit;}
    LPCWSTR GetMainType() const { return m_szType;}
    LPCWSTR GetMainExt() const { return m_szExt;}

    LPCSTR GetCacheDir() const { return m_szCacheDir;}
    HRESULT ResolveCacheDirNameConflicts();
#ifdef _ZEROIMPACT
    HRESULT SetupZeroImpactDir();
    void GetVersion(DWORD * pdwVersionMS, DWORD * pdwVersionLS){
        *pdwVersionMS = m_dwFileVersionMS;
        *pdwVersionLS = m_dwFileVersionLS;
    }
#endif

    void SetExactVersion(BOOL bExactVersion) { m_bExactVersion = bExactVersion;
                                               if (m_bExactVersion) {
                                                   m_bUninstallOld = TRUE; // implied
                                               }
                                             }

    VOID SetListCookie(LISTPOSITION pos) {m_ListCookie = pos;}
    LISTPOSITION GetListCookie() const {return m_ListCookie;}

    HRESULT AcquireSetupCookie();
    HRESULT RelinquishSetupCookie();

    HRESULT PiggybackDupRequest(
        IBindStatusCallback *pDupClientBSC,
        IBindCtx *pbc,
        REFCLSID rclsid, 
        DWORD dwClsContext,
        LPVOID pvReserved,
        REFIID riid);

    BOOL GenerateErrStrings(HRESULT hr, char **ppszErrMsg, WCHAR **ppwszError);

    static HRESULT CCodeDownload::AnyCodeDownloadsInThread();

    static HRESULT CCodeDownload::HasUserDeclined( 
        LPCWSTR szDistUnit, 
        LPCWSTR szType, 
        LPCWSTR szExt, 
        IBindStatusCallback *pClientBSC,
        IInternetHostSecurityManager *pHostSecurityManager);

    static HRESULT CCodeDownload::HandleDuplicateCodeDownloads( 
        LPCWSTR szURL, 
        LPCWSTR szType, 
        LPCWSTR szExt, 
        REFCLSID rclsid, 
        LPCWSTR szDistUnit, 
        DWORD dwClsContext,
        LPVOID pvReserved,
        REFIID riid,
        IBindCtx* pbc,
        IBindStatusCallback *pDupClientBSC,
        DWORD dwFlags,
        IInternetHostSecurityManager *pHostSecurityManager);

    HRESULT CCodeDownload::SetUserDeclined();

    HRESULT IsDuplicateHook(LPCSTR szHook);
    HRESULT IsDuplicateJavaSetup( LPCWSTR szPackage);

    HRESULT ProcessJavaManifest(IXMLElement *pJava, const char *szOSD, char *szOSDBaseName, CDownload *pdl);
    HRESULT ProcessDependency(CDownload *pdl, IXMLElement *pDepend);
    HRESULT ProcessNativeCode(CDownload *pdl, IXMLElement *pCode);
    HRESULT ExtractInnerCAB(CDownload *pdl, LPSTR szCABFile);
    HRESULT AddDistUnitList(LPWSTR szDistUnit);

    VOID DoSetup();

    HRESULT DealWithExistingFile(
        LPSTR szExistingFile,
        DWORD cbExistingFile,
        LPSTR pBaseExistingName,
        LPSTR *ppDestDir,
        LPDWORD pdwLocFVMS,
        LPDWORD pdwLocFVLS,
        FILETIME *pftLastModified = NULL);

    BOOL NeedLatestVersion() { 
        return (( m_dwFileVersionMS == -1) && (m_dwFileVersionLS == -1));
    }
    LPSTR GetEtag() { return m_pbEtag;}
    VOID SetEtag(LPSTR szEtag) { m_pbEtag = szEtag;}
    LPSTR GetLocalVersionEtag() { return m_plci->GetLocalVersionEtag();}

    LPSTR GetLastMod() { return m_szLastMod[0]?m_szLastMod:NULL;}
    VOID SetLastModifiedTime(LPCSTR szLastMod) {
        lstrcpy(m_szLastMod, szLastMod);
    }

    HRESULT DelayRegisterOCX(LPCSTR pszSrcFileName, FILEXTN extn);

    HRESULT InstallOCX(LPCSTR lpSrcFileName, FILEXTN extn, BOOL fLocalServer);
    HRESULT RegisterPEDll( LPCSTR lpSrcFileName);
    HRESULT RegisterOmniDll( LPCSTR lpSrcFileName);
#ifdef WX86
    HRESULT RegisterWx86Dll( LPCSTR lpSrcFileName);
    CMultiArch *GetMultiArch() { return &m_MultiArch; }
#endif

#ifdef _ZEROIMPACT
    HRESULT UpdateZeroImpactCache();
#endif

    // BUGBUG: put these three in a SearchPath class
    HRESULT SetupCODEUrl(LPWSTR *ppDownloadURL, FILEXTN *pextn);

    HRESULT GetNextComponent( LPSTR szList, LPSTR *ppCur);

    HRESULT GetNextOnInternetSearchPath(
                REFCLSID rclsid,
                HGLOBAL *phPostData,
                DWORD *pcbPostData,
                LPWSTR szURL,
                DWORD dwSize,
                LPWSTR *ppDownloadURL,
                FILEXTN *pextn);

    HRESULT SelfRegEXETimeout();

    HRESULT AbortBinding(CClBinding *pbinding);

    BOOL WeAreReadyToSetup();

    LPCSTR GetMainInf() { return m_szInf;}
    LPCSTR GetOSD() { return m_szOSD;}
    LCID GetLCID() { return m_lcid;}

    BOOL VersionFromManifest(LPSTR szVersionInManifest);

    HRESULT SetManifest(FILEXTN extn, LPCSTR szManifest);

    CLangInfo *GetLangInfo() { return &m_langinfo;}
    void CodeDownloadDebugOut(int iOption, BOOL fOperationFailed,
                              UINT iResId, ...);

    HRESULT IsPackageLocallyInstalled(LPCWSTR szPackageName, LPCWSTR szNameSpace, DWORD dwVersionMS, DWORD dwVersionLS);

    IJavaPackageManager * GetPackageManager() { return m_pPackageManager;}
    HRESULT SetCatalogFile(LPSTR szCatalogFile);
    LPSTR GetCatalogFile();
    HRESULT SetMainCABJavaTrustPermissions(PJAVA_TRUST pbJavaTrust);
    PJAVA_TRUST GetJavaTrust();
    void SetDebugLog(CDLDebugLog * debuglog);

#ifdef _ZEROIMPACT
    BOOL IsZeroImpact() {return m_bIsZeroImpact;}
    void SetZeroImpact(BOOL bZI) {m_bIsZeroImpact = bZI;}
#endif

    private:

    CDL_STATE GetState() const { return m_state;}
    VOID SetState(CDL_STATE state) { m_state = state;}

    HRESULT UpdateFileList(HKEY hKeyContains);
    HRESULT UpdateDependencyList(HKEY hKeyContains);
    HRESULT UpdateJavaList(HKEY hKeyContains);
    HRESULT UpdateDistUnit(CLocalComponentInfo *plci);
    HRESULT UpdateLanguageCheck(CLocalComponentInfo *plci);

    void DumpDebugLog(char *szCacheFileName, LPTSTR szErrorMsg, HRESULT hrError);
    void DestroyPCBHList(CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList);

    DWORD                m_cRef;

    CDL_STATE            m_state;               // state of code download

    LPWSTR               m_url;
    LPWSTR               m_szDistUnit;
    LPWSTR               m_szType;
    LPWSTR               m_szExt;
    LPSTR                m_szDisplayName;
    LPSTR                m_szVersionInManifest;

    IMoniker*            m_pmkContext;          // first download's pmk
                                                // becomes the context
                                                // for handling subsequent
                                                // relative URLs

    DWORD                m_dwFileVersionMS;     // little quirky that
    DWORD                m_dwFileVersionLS;     // this really the primary
                                                // control's fileversion
                                                // can really be something
                                                // else if deemed appropriate
                                                // eg: subversion of clsid

    LCID                 m_lcid;                // cache lcid client needs
                                                // this is pulled out of the
                                                // bindctx (BIND_OPTS2)

    CList<CModuleUsage*,CModuleUsage*>          // linked list of module usage
                         m_ModuleUsage;         // entries to add

    CList<CClBinding*,CClBinding*>              // linked list of client
                         m_pClientbinding;      // IBindings

    CList<CDownload*,CDownload*>                // linked list of CDownload
                         m_pDownloads;          // pieces

    CList<LPWSTR,LPWSTR>                      // linked list of dependent
                         m_pDependencies;       // distribution units

    DWORD                m_flags;               // provision for hacks :)
                                                // used internally now to
                                                // mark if we have an INF file
                                                // and if safe to abort

    LPSTR                m_szInf;               // INF filename if one exists
    LPSTR                m_szOSD;               // INF filename if one exists

    LPSTR                m_szCacheDir;          // OCCACHE dir that is setup
                                                // for each code download
                                                // is usually = g_szOCXCacheDir
                                                // but if a name conflict arises
                                                // it can be OCCACHE\CONFLICT.n



    LPSTR                m_pAddCodeSection;     // Add.Code section in INF
                                                // bunch of null terminated
                                                // strings, last one double
                                                // null terminated (like env)
    LPSTR                m_pCurCode;            // points at what ProcessInf
                                                // is pending processing

    HKEY                 m_hKeySearchPath;      // key to REGSTR_PATH_ISP

    LPSTR                m_pSearchPath;         // List of searchpath comps
    LPSTR                m_pSearchPathNextComp; // pointer into list above at
                                                // current component

    CLangInfo            m_langinfo;

    LPSTR                m_szWaitForEXE;        // str that points to name of
                                                // self registering EXE that we
                                                // are waiting now to complete
                                                // This is used to give
                                                // detailed CodeInstallProblem
                                                // when we get a ClientBinding::
                                                // Abort wahile waiting for an
                                                // EXE to complete setup/reg

    PROCESS_INFORMATION  m_pi;                  // PI for the currently self-
                                                // registering EXE that we
                                                // are running

    LISTPOSITION         m_ListCookie;          // cookie to remove this
                                                // download from the per-thread
                                                // list of CCodeDownload's in
                                                // progress (CList)

    HRESULT              m_hr;                  // hr to pass to CompleteAll
                                                // this store away the first
                                                // real failure code in a 
                                                // multipart code download


    char                 m_szLastMod[INTERNET_RFC1123_BUFSIZE+1];
                                                // last modified date of the
                                                // main URL (typically CODEBASE
                                                // or the URL redirected by
                                                // Object Index). We save this
                                                // with the dist unit DB to use
                                                // later for Get Latest.

    DWORD               m_dwExpire;             // number of days before DU eligible
                                                // for scavenging.

    DWORD               m_dwSystemComponent;

    char                *m_pbEtag;

    CLocalComponentInfo* m_plci;

    IJavaPackageManager* m_pPackageManager;     // Java pkg mgr

    DWORD                m_grfBINDF;
    CList<CCodeBaseHold *, CCodeBaseHold *> *m_pcbhList;
    LPSTR                m_szCatalogFile;
    PJAVA_TRUST          m_pbJavaTrust;
    CDLDebugLog      *   m_debuglog;
#ifdef _ZEROIMPACT
    BOOL                 m_bIsZeroImpact;
    LPWSTR               m_wszZIDll;
#endif

    BOOL  m_bUninstallOld;
    BOOL  m_bExactVersion;
    HMODULE m_hModSFC;

#ifdef WX86
    CMultiArch        m_MultiArch;
#endif
};

class DebugLogElement {
    public:
        DebugLogElement() : m_szMessage(NULL) {}
        DebugLogElement(LPSTR szMessage);
        DebugLogElement(const DebugLogElement &ref);
        virtual ~DebugLogElement();

    public:
        LPCSTR GetLogMessage() { return m_szMessage; }
        HRESULT SetLogMessage(LPSTR szMessage);

    private:
        LPSTR           m_szMessage;
};


// Class to make a debug log and pass out error messages

// Note reguarding wide strings vs. ANSI:
// For file-write compatiblity, the private strings of CDLDebugLog
// are stored as multibyte, but the primary accessors (the private
// variables in CCodeDownLoad) are wide characters.  Hence, the
// return values for the accesors are Multibyte, since these are used
// primarily by this class, while the set parameters are wide character
class CDLDebugLog {
    public:
        ~CDLDebugLog();

        // Name accessor functions
        // Return the current value for the requested name (may be "")
        LPCTSTR              GetMainClsid()  {return m_szMainClsid;}
        LPCTSTR              GetMainType()   {return m_szMainType;}
        LPCTSTR              GetMainExt()    {return m_szMainExt;}
        LPCTSTR              GetMainUrl()    {return m_szMainUrl;}
        LPCTSTR              GetFileName()   {return m_szFileName;}
        LPCTSTR              GetUrlName()    {return m_szUrlName;}

        // Functions to access or output error messages
        void                 DebugOut(int iOption, BOOL fOperationFailed,
                                      UINT iResId, ...);
        void                 DebugOutPreFormatted(int iOption, BOOL fOperationFailed,
                                                  LPTSTR szDebugString);
        void                 DumpDebugLog(LPTSTR pszCacheFileName, int cbBufLen, 
                                          LPTSTR szErrorMsg, HRESULT hrError);

        // Deletion and Initialization functions
        void                 Clear();
        BOOL                 Init(CCodeDownload * pcdl);
        BOOL                 Init(LPCWSTR wszMainClsid, LPCWSTR wszMainType, 
                                  LPCWSTR wszMainExt, LPCWSTR wszMainUrl);
        void                 MakeFile();


        // Static functions for storing and accessing stored debug logs
        static void          AddDebugLog(CDLDebugLog * dlog);
        static void          RemoveDebugLog(CDLDebugLog * dlog);
        static CDLDebugLog * GetDebugLog(LPCWSTR wszMainClsid, LPCWSTR wszMainType, 
                                         LPCWSTR wszMainExt, LPCWSTR wszMainUrl);
        // Static functions for saving an error message for the download                            
        static BOOL          SetSavedMessage(LPCTSTR szMessage, BOOL bOverwrite);                             
        static LPCTSTR       GetSavedMessage();

        // Static function for debug log construction
        static CDLDebugLog * MakeDebugLog();

        // Com-type add and release functions
        int AddRef();
        int Release();

    private:

        CDLDebugLog();
        CList<DebugLogElement *, DebugLogElement *> 
                               m_DebugLogList;
        BOOL                   m_fAddedDebugLogHead;
        TCHAR                  m_szFileName[INTERNET_MAX_URL_LENGTH];
        TCHAR                  m_szUrlName[INTERNET_MAX_URL_LENGTH];
        TCHAR                  m_szMainClsid[MAX_DEBUG_STRING_LENGTH];
        TCHAR                  m_szMainType[MAX_DEBUG_STRING_LENGTH];
        TCHAR                  m_szMainExt[MAX_DEBUG_STRING_LENGTH];
        TCHAR                  m_szMainUrl[INTERNET_MAX_URL_LENGTH];

        // Support addref and release
        int m_iRefCount;

        // List and mutex for stored CDLDebugLog's
        static CList<CDLDebugLog *, CDLDebugLog *>
                               s_dlogList;
        static CMutexSem       s_mxsDLogList;
        static CMutexSem       s_mxsMessage;

        // Saved error message
        static TCHAR           s_szMessage[];
        static BOOL            s_bMessage;

};



// values for CDownload::m_flags
typedef enum {
    DL_FLAGS_INIT =                     0x0,
    DL_FLAGS_TRUST_VERIFIED=            0x1,
    DL_FLAGS_EXTRACT_ALL=               0x2,
    DL_FLAGS_CDL_PROTOCOL=              0x4         // using cdl:// to kick off DL
} DL_FLAGS;

class CParentCDL {

    public:

    CParentCDL(CCodeDownload *pcdl) {m_pcdl = pcdl;m_bCompleteSignalled = FALSE;}

    CCodeDownload*  m_pcdl;
    BOOL            m_bCompleteSignalled;
};

class CBindStatusCallback;
class CSetupHook;

// one for each individual downloads
class CDownload {

  public:

    // constructor
    CDownload(LPCWSTR szURL, FILEXTN extn, HRESULT *phr);
    ~CDownload();

    void CDownload::CleanUp();

    HRESULT AddParent(CCodeDownload *pcdl);
    HRESULT ReleaseParent(CCodeDownload *pcdl);

    HRESULT CompleteSignal(HRESULT hrOSB, HRESULT hrStatus, HRESULT hrResponseHdr, LPCWSTR szError);

    HRESULT DoDownload(LPMONIKER *ppmkContext, DWORD grfBINDF,
                      CList<CCodeBaseHold *, CCodeBaseHold *> *pcbhList = NULL);

    HRESULT Abort(CCodeDownload *pcdl);

    BOOL IsSignalled(CCodeDownload *pcdl);

    //  for each in list CSetup::DoSetup
    HRESULT DoSetup();

    BOOL IsDuplicateSetup(LPCSTR pBaseFileName);

    // called by CBSC::OnStopBinding as soon as the binding completes
    VOID ProcessPiece();

    // for each in list CSetup::CheckForNameCollision
    HRESULT CheckForNameCollision(LPCSTR szCacheDir);

    HRESULT CleanupFiles();

    CDownload *GetNext() const { return m_pdlnext;}
    VOID SetNext(CDownload *pdlnext) { m_pdlnext = pdlnext;}

    CSetup* GetSetupHead() const {return m_pSetuphead;}
    VOID AddSetupToList(CSetup *pSetup);
    HRESULT RemoveSetupFromList(CSetup *pSetup);

    HRESULT AddHook(
        LPCSTR szHook,
        LPCSTR szInf,
        LPCSTR szInfSection,
        DWORD flags);

    HRESULT AddSetupToExistingCAB(
        char *lpCode,
        const char * szDestDir,
        DESTINATION_DIR dest,
        DWORD dwRegisterServer,
        DWORD dwCopyFlags);

    CCodeDownload* GetCodeDownload() const { return (m_ParentCDL.GetHead())->m_pcdl;}

    CBindStatusCallback* GetBSC() const { return  m_pbsc;}
    IBindCtx* GetBindCtx() const { return m_pbc;}

    VOID SetUnkForCacheFileRelease(IUnknown *pUnk) {m_pUnkForCacheFileRelease = pUnk;}

    LPCSTR GetFileName() const { return m_pFileName;}
    VOID SetFileName(LPSTR pFileName) { m_pFileName = pFileName;}

    HRESULT IsDownloadedVersionRequired();

    LPCWSTR GetURL() const { return m_url;}

    HRESULT GetFriendlyName(LPSTR szUrlPath, LPSTR *ppBaseFileName);

    IMoniker* GetMoniker() const {return m_pmk;}

    HRESULT SetURLAndExtn(LPCWSTR szURL, FILEXTN extn);

    HRESULT SniffType();

    FILEXTN    GetExtn() const {return m_extn;}
    PFNAME GetFilesToExtract() const { return m_pFilesToExtract;}

    DLSTATE GetDLState() const { return m_state;}
    VOID SetDLState(DLSTATE state) 
    {  
        m_state = state;
    }

    VOID SetProgress(ULONG ulProgress, ULONG ulProgressMax) { m_ulProgress = ulProgress; m_ulProgressMax = ulProgressMax;}
    VOID SumProgress(ULONG *pulProgress, ULONG *pulProgressMax) { *pulProgress += m_ulProgress; *pulProgressMax += m_ulProgressMax;}


    PSESSION GetSession() const { return m_psess;}
    VOID SetSession(PSESSION psess) { m_psess = psess;}

    HGLOBAL GetPostData(DWORD *pcbPostData) const {
            *pcbPostData = m_cbPostData;
            return m_hPostData;
            }

    VOID    SetPostData(HGLOBAL hPostData, DWORD cbPostData) {
            m_hPostData = hPostData;
            m_cbPostData = cbPostData;
            }
    BOOL DoPost() const { return (m_hPostData != NULL);}

    HRESULT GetResponseHeaderStatus() const {return m_hrResponseHdr;}
    VOID SetResponseHeaderStatus( HRESULT hrResponseHdr) {
            m_hrResponseHdr = hrResponseHdr;}

    VOID VerifyTrust();

    BOOL TrustVerified() const {return (m_flags & DL_FLAGS_TRUST_VERIFIED);}
    VOID SetTrustVerified() {m_flags |= DL_FLAGS_TRUST_VERIFIED;}

    BOOL NeedToExtractAllFiles() const {return(m_flags & DL_FLAGS_EXTRACT_ALL);}
    VOID SetNeedToExtractAll() {m_flags |= DL_FLAGS_EXTRACT_ALL;}

    BOOL UsingCdlProtocol() const {return(m_flags & DL_FLAGS_CDL_PROTOCOL);}
    HRESULT SetUsingCdlProtocol(LPWSTR szDistUnit);
    LPWSTR GetDistUnitName() const {return(m_wszDistUnit);}

    HRESULT ExtractManifest(FILEXTN extn, LPSTR szFileName, LPSTR& pBaseFileName);

    CSetupHook* FindHook(LPCSTR szHook);
    CJavaSetup* FindJavaSetup(LPCWSTR szPackageName);

    HRESULT AddJavaSetup(LPCWSTR szPackageName, LPCWSTR szNameSpace, IXMLElement *pPackage, DWORD dwVersionMS, DWORD dwVersionLS, DWORD flags);
    CList<CJavaSetup*,CJavaSetup*> *GetJavaSetupList() { return &m_JavaSetupList;}
    BOOL HasJavaPermissions();
    BOOL HasAllActiveXPermissions();
    PJAVA_TRUST GetJavaTrust() {return m_pbJavaTrust;}

    HRESULT PerformVirusScan(LPSTR szFileName);

    STDMETHODIMP DownloadRedundantCodeBase();
    HRESULT SetMainCABJavaTrustPermissions(PJAVA_TRUST pbJavaTrust);

#ifdef _ZEROIMPACT
    BOOL IsZeroImpact() {return m_bIsZeroImpact;}
    void SetZeroImpact(BOOL bZI) {m_bIsZeroImpact = bZI;}
#endif

    void SetExactVersion(BOOL bFlag) {m_bExactVersion = bFlag;}

  private:

    LPWSTR               m_url;
    FILEXTN              m_extn;
    LPSTR                m_pFileName;        // filename in temp once downloaded
    
    IMoniker*            m_pmk;
    IBindCtx*            m_pbc;

    IUnknown*            m_pUnkForCacheFileRelease;

    CBindStatusCallback* m_pbsc;

    CDownload*           m_pdlnext;

    CList<CParentCDL *,CParentCDL *>             // linked list of CCodeDownloads
                         m_ParentCDL;        // that have interest in us

    ULONG                m_ulProgress;
    ULONG                m_ulProgressMax;

    DLSTATE              m_state;

    PSESSION             m_psess;            // CAB extraction struc
    PFNAME               m_pFilesToExtract;  // applicable only for CAB objs

    CSetup*              m_pSetuphead;       // list of CSetup's for this dwld

    DWORD                m_flags;            // provision for hacks :)

    HGLOBAL              m_hPostData;        // has the query for the clsid

    DWORD                m_cbPostData;       // has size of post data

    BOOL                 m_bCompleteSignalled;

    LPWSTR               m_wszDistUnit;      // name of distribution for cdl:// dl

    HRESULT              m_hrOSB;            // this is the hr we got
                                             // from OnStopBinding
    HRESULT              m_hrStatus;         // this is the hr we got
                                             // URLMON for the binding
    HRESULT              m_hrResponseHdr;    // this is the hr we got
                                             // from the response headers
                                             // when querying for this clsid
                                             // this is to make sure we have the
                                             // right error status even when
                                             // urlmon does not pass back right
                                             // OnError.

    PJAVA_TRUST          m_pbJavaTrust;

    CList<CSetupHook*,CSetupHook*>           // linked list of setup hooks
                         m_SetupHooks;

    CList<CJavaSetup*,CJavaSetup*>           // linked list of Java setups
                         m_JavaSetupList;

    Cwvt                 m_wvt;              // WinVerifyTrust delay load
                                             // magic in this class

    CList<CCodeBaseHold *, CCodeBaseHold *> *m_pcbhList;
    DWORD               m_grfBINDF;
    LPMONIKER           *m_ppmkContext;
#ifdef _ZEROIMPACT
    BOOL                 m_bIsZeroImpact;
#endif

    BOOL m_bExactVersion;

};

// BSC for our indiv. CDownloads
class CBindStatusCallback : public IBindStatusCallback
                            ,public IHttpNegotiate
                            ,public IWindowForBindingUI
                            ,public IServiceProvider
                            ,public ICatalogFileInfo
{

  public:

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IBindStatusCallback methods
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pFmtetc, STGMEDIUM  __RPC_FAR *pstgmed);
    STDMETHODIMP    OnObjectAvailable( REFIID riid, IUnknown* punk);

    STDMETHODIMP    OnStartBinding(DWORD grfBSCOPTION,IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
                        LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);

    // *** IHttpNegotiate methods ***
    STDMETHOD(BeginningTransaction) (
        LPCWSTR szURL,
        LPCWSTR szHeaders,
        DWORD dwReserved,
        LPWSTR *pszAdditionalHeaders);

    STDMETHOD(OnResponse) (
        DWORD dwResponseCode,
        LPCWSTR szResponseHeaders,
        LPCWSTR szRequestHeaders,
        LPWSTR *pszAdditionalRequestHeaders);

    // *** IWindowForBindingUI methods ***
    STDMETHOD(GetWindow) (
        REFGUID rguidreason,
                HWND *phWnd);

    // *** IServiceProvider ***
    STDMETHOD(QueryService) (
        REFGUID guidService,
        REFIID riid,
        LPVOID *ppv);


    // *** ICatalogFileInfo ***
    STDMETHODIMP GetCatalogFile(LPSTR *ppszCatalogFile);
    STDMETHODIMP GetJavaTrust(void **ppJavaTrust);

    // constructor
    CBindStatusCallback(CDownload *pdl, DWORD grfBINDF);
    ~CBindStatusCallback();

    IBinding* GetBinding() const {return m_pbinding;}

    private:

    DWORD           m_cRef;
    IBinding*       m_pbinding;
    CDownload*      m_pdl;        // point up into download obj
    DWORD           m_grfBINDF;

    BYTE            m_cbBuffer[BUFFERMAX];

};

class CSetupHook {
    public:

    CSetupHook(
        CDownload *pdl,
        LPCSTR szHook,
        LPCSTR szInf,
        LPCSTR szInfSection,
        DWORD flags,
        HRESULT *phr);

    ~CSetupHook();

    HRESULT Run();

    static HRESULT ExpandCommandLine(
    LPSTR szSrc,
    LPSTR szBuf,
    DWORD cbBuffer,
    const char * szVars[],          // array of variable names eg. %EXTRACT_DIR%
    const char * szValues[]);       // corresponding values to expand of vars

    static HRESULT ExpandVar(
        LPSTR& pchSrc,          // passed by ref!
        LPSTR& pchOut,          // passed by ref!
        DWORD& cbLen,           // passed by ref!
        DWORD cbBuffer,         // size of out buffer
        const char * szVars[],  // array of variable names eg. %EXTRACT_DIR%
        const char * szValues[]);// corresponding values to expand of vars

    HRESULT TranslateString();

    INSTALL_STATE GetState() const { return m_state;}
    VOID SetState(INSTALL_STATE state) { m_state = state;}

    LPSTR GetHookName() { return m_szHook; }

    const char * GetObjectDir() { 

        if (m_pdl && m_pdl->GetCodeDownload())
            return m_pdl->GetCodeDownload()->GetCacheDir();
        else
            return NULL;
    }

    LPCWSTR GetSrcURL() {
        if (m_pdl) {
            return m_pdl->GetURL();
        }
        else {
            return NULL;
        }
    }

    LPSTR GetHookDir() { 

        if (m_pdl && m_pdl->GetSession())
            return m_pdl->GetSession()->achLocation;
        else
            return NULL;
    }

    private:

    INSTALL_STATE      m_state;            // state of install operation
    CDownload*         m_pdl;
    LPSTR              m_szHook;
    LPSTR              m_szInf;
    LPSTR              m_szInfSection;
    DWORD              m_flags;
};

HRESULT SetCodeDownloadTLSVars();

// private helpers

// from isctrl.cxx
HRESULT  IsControlLocallyInstalled(
    LPSTR lpCurCode,
    const LPCLSID lpclsid,
    LPCWSTR szDistUnit,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS,
    CLocalComponentInfo *plci,
    LPSTR szDestDirHint,
    BOOL bExactVersion = FALSE);

// from isctrl.cxx
HRESULT  IsCLSIDLocallyInstalled(
    LPSTR lpCurCode,
    const LPCLSID lpclsid,
    LPCWSTR szDistUnit,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS,
    CLocalComponentInfo *plci,
    LPSTR szDestDirHint,
    HRESULT *pHrExtra,
    BOOL bExactVersion
    );

HRESULT
IsDistUnitLocallyInstalled(
    LPCWSTR szDistUnit,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS,
    CLocalComponentInfo *plci,
    LPSTR szDestDirHint,
    LPBOOL pbParanoidCheck,
    DWORD flags);

HRESULT
IsPackageLocallyInstalled(
    IJavaPackageManager **ppPackageManager,
    LPCWSTR szPackageName,
    LPCWSTR szNameSpace,
    DWORD dwVersionMS,
    DWORD dwVersionLS);

HRESULT LocalVersionOK(
    HKEY hkeyClsid,
    CLocalComponentInfo *plci,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS,
    BOOL bExactVersion
    );


HRESULT GetFileVersion(
    CLocalComponentInfo *plci,
    LPDWORD pdwFileVersionMS,
    LPDWORD pdwFileVersionLS);

HRESULT GetVersionFromString(
    const char *szBuf,
    LPDWORD pdwFileVersionMS,
    LPDWORD pdwFileVersionLS,
    char cSeperator = ',');

#ifdef _ZEROIMPACT
LPSTR GetStringFromVersion(
    char *szBuf, 
    DWORD dwFileVersionMS, 
    DWORD dwFileVersionLS, 
    char cSeperator = ',');
#endif

BOOL AdviseForceDownload( const LPCLSID lpclsid, DWORD dwClsContext);

// flags for UpdateModuleUsage

#define    MU_CLIENT   0        // mark us as a client
#define    MU_OWNER    1        // mark us as the owner (iff no prev ver exists)

HRESULT
UpdateModuleUsage(
    LPCSTR szFileName,
    LPCSTR szClientName,
    LPCSTR szClientPath,
    LONG muFlags);

HRESULT UpdateSharedDlls( LPCSTR szFileName);

BOOL    SupportsSelfRegister(LPSTR szFileName);
BOOL    WantsAutoExpire(LPSTR szFileName, DWORD *pnExpireDays ); 

// from wvt.cxx
HRESULT GetActivePolicy(IInternetHostSecurityManager* pZoneManager, 
                        LPCWSTR pwszZone,
                        DWORD  dwUrlAction,
                        DWORD& dwPolicy);

// from peldr.cxx
HRESULT IsCompatibleFile(const char *szFileName, LPDWORD lpdwMachineType=NULL);
HRESULT IsRegisterableDLL(const char *szFileName);


// fro, softdist.cxx
HRESULT GetLangString(LCID localeID, char *szThisLang);
HRESULT InitBrowserLangStrings();

// from duman.cxx
HRESULT GetSoftDistFromOSD(LPCSTR szFile, IXMLElement **ppSoftDist);

HRESULT GetFirstChildTag(IXMLElement *pRoot, LPCWSTR szTag, IXMLElement **ppChildReq);
HRESULT GetNextChildTag(IXMLElement *pRoot, LPCWSTR szTag, IXMLElement **ppChildReq, int &nLastChild);

HRESULT GetAttribute(IXMLElement *pElem, LPWSTR szAttribName, VARIANT *pvProp);
HRESULT GetAttributeA(IXMLElement *pElem, LPWSTR szAttribName, LPSTR pAttribValue, DWORD dwBufferLen);
HRESULT DupAttributeA(IXMLElement *pElem, LPWSTR szAttribName, LPSTR *ppszRet);
HRESULT DupAttribute(IXMLElement *pElem, LPWSTR szAttribName, LPWSTR *ppszRet);
HRESULT GetTextContent(IXMLElement *pRoot, LPCWSTR szTag, LPWSTR *ppszContent);

// from jit.cxx
HRESULT
GetIEFeatureFromMime(LPWSTR *ppwszIEFeature, LPCWSTR pwszMimeType, QUERYCONTEXT *pQuery);
HRESULT
GetIEFeatureFromClass(LPWSTR *ppwszIEFeature, REFCLSID clsid, QUERYCONTEXT *pQuery);

// from client.cxx
IInternetHostSecurityManager* GetHostSecurityManager(IBindStatusCallback *pclientbsc);

// from helpers.cxx
HRESULT CheckFileImplementsCLSID(const char *pszFileName, REFCLSID rClsid);
FILEXTN GetExtnAndBaseFileName( char *szName, char **plpBaseFileName);
HRESULT MakeUniqueTempDirectory(LPCSTR szTempDir, LPSTR szUniqueTempDir);
HRESULT ComposeHackClsidFromMime(LPSTR szHackMimeType, LPCSTR szClsid);

HRESULT GetHeaderValue (
    LPCWSTR szResponseHeadersBuffer,
    DWORD   cbResponseHeadersBuffer,
    LPCWSTR lpcszHeaderName,
    LPWSTR  pHeaderValue,
    DWORD   cbHeaderValue);

HRESULT
GetClsidFromExtOrMime(
    REFCLSID rclsid,
    CLSID &clsidout,
    LPCWSTR szExt,
    LPCWSTR szTYPE,
    LPSTR *ppFileName);

STDAPI
AsyncGetClassBitsEx(
    REFCLSID rclsid,                      // CLSID
    CodeDownloadData * pcdd,              // Contains requested object's descriptors    
    IBindCtx *pbc,                        // bind ctx: should contain BSC
    DWORD dwClsContext,                   // CLSCTX flags
    LPVOID pvReserved,                    // Must be NULL
    REFIID riid);                         // Usually IID_IClassFactory

STDAPI
AsyncGetClassBits2Ex(
    LPCWSTR szClientID,                 // client ID, root object if NULL
    CodeDownloadData * pcdd,            // Contains requested object's descriptors
    IBindCtx *pbc,                      // bind ctx
    DWORD dwClsContext,                 // CLSCTX flags
    LPVOID pvReserved,                  // Must be NULL
    REFIID riid,                        // Usually IID_IClassFactory
    IUnknown **pUnk);


STDAPI
AsyncInstallDistributionUnitEx(
    CodeDownloadData * pcdd,            // Contains requested object's descriptors
    IBindCtx *pbc,                      // bind ctx
    REFIID riid,
    IUnknown **pUnk,
    LPVOID pvReserved);                 // Must be NULL


// backwards compatability
STDAPI
AsyncGetClassBits(
    REFCLSID rclsid,                      // CLSID
    LPCWSTR szType,                       // MIME type 
    LPCWSTR szExtension,                  // or Extension
                                          // as alternate
                                          // if CLSID == CLSID_NULL

    DWORD dwFileVersionMS,                // CODE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS,                // MAKEDWORD(c,b) of above
    LPCWSTR szURL,                        // CODEBASE= in OBJECT tag
    IBindCtx *pbc,                        // bind ctx: should contain BSC
    DWORD dwClsContext,                   // CLSCTX flags
    LPVOID pvReserved,                    // Must be NULL
    REFIID riid,                          // Usually IID_IClassFactory
    DWORD flags);
STDAPI
AsyncInstallDistributionUnit(
    LPCWSTR szDistUnit,
    LPCWSTR szTYPE,
    LPCWSTR szExt,
    DWORD dwFileVersionMS,              // CODEBASE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS,              // MAKEDWORD(c,b) of above
    LPCWSTR szURL,                      // CODEBASE
    IBindCtx *pbc,                      // bind ctx
    LPVOID pvReserved,                  // Must be NULL
    DWORD flags);
STDAPI
AsyncGetClassBits2(
    LPCWSTR szClientID,                 // client ID, root object if NULL
    LPCWSTR szDistUnit,                 // CLSID, can be an arbit unique str
    LPCWSTR szTYPE,
    LPCWSTR szExt,
    DWORD dwFileVersionMS,              // CODE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS,              // MAKEDWORD(c,b) of above
    LPCWSTR szURL,                      // CODE= in INSERT tag
    IBindCtx *pbc,                      // bind ctx
    DWORD dwClsContext,                 // CLSCTX flags
    LPVOID pvReserved,                  // Must be NULL
    REFIID riid,                        // Usually IID_IClassFactory
    DWORD flags);


#ifdef unix
extern "C"
#endif /* unix */
DWORD WINAPI
CDLGetLongPathNameA( 
    LPSTR lpszLong,
    LPCSTR lpszShort,
    DWORD cchBuffer
    );

#ifdef unix
extern "C"
#endif /* unix */
DWORD WINAPI
CDLGetLongPathNameW(
    LPWSTR lpszLongPath,
    LPCWSTR  lpszShortPath,
    DWORD    cchBuffer
    );

HRESULT
   GetActiveXSafetyProvider(
                            IActiveXSafetyProvider **ppProvider
                           );

extern int  g_CPUType;
extern BOOL g_fRunningOnNT;
extern BOOL g_bRunOnNT5;
#ifdef WX86
extern BOOL g_fWx86Present;
#endif

VOID
DetermineOSAndCPUVersion();

#ifdef UNICODE
#define CDLGetLongPathName  CDLGetLongPathNameW
#else
#define CDLGetLongPathName  CDLGetLongPathNameA
#endif // !UNICODE

#endif /* end hide classes from 'C' */
#endif // _CDL_H_
