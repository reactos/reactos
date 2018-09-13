/*++

Copyright (c) 1993-1998 Microsoft Corporation

Module Name:

    infinst.c

Abstract:

    High-level INF install section processing API.

Author:

    Ted Miller (tedm) 6-Mar-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#ifdef WX86
#include <wx86dll.h>
#endif

#ifndef UNICODE
#define MyMsgWaitForMultipleObjectsEx(nc,ph,dwms,dwwm,dwfl) MsgWaitForMultipleObjects(nc,ph,FALSE,dwms,dwwm)
#else
#define MyMsgWaitForMultipleObjectsEx MsgWaitForMultipleObjectsEx
#endif
//
// Define invalid flags for SetupInstallServicesFromInfSection(Ex)
//
#define SPSVCINST_ILLEGAL_FLAGS (~( SPSVCINST_TAGTOFRONT               \
                                  | SPSVCINST_ASSOCSERVICE             \
                                  | SPSVCINST_DELETEEVENTLOGENTRY      \
                                  | SPSVCINST_NOCLOBBER_DISPLAYNAME    \
                                  | SPSVCINST_NOCLOBBER_STARTTYPE      \
                                  | SPSVCINST_NOCLOBBER_ERRORCONTROL   \
                                  | SPSVCINST_NOCLOBBER_LOADORDERGROUP \
                                  | SPSVCINST_NOCLOBBER_DEPENDENCIES   \
                                  | SPSVCINST_STOPSERVICE              ))

//
// Flags for UpdateInis in INFs
//
#define FLG_MATCH_KEY_AND_VALUE 1
#ifdef INF_FLAGS
#define FLG_CONDITIONAL_ADD     2
#endif

//
// Flags for UpdateIniFields in INFs
//
#ifdef INF_FLAGS
#define FLG_INIFIELDS_WILDCARDS 1
#endif
#define FLG_INIFIELDS_USE_SEP2  2

#define TIME_SCALAR                   (1000)
#define REGISTER_WAIT_TIMEOUT_DEFAULT (60)
#define RUNONCE_TIMEOUT               (2*60*1000)
#define RUNONCE_THRESHOLD             (20) // * RUNONCE_TIMEOUT

#define DLLINSTALL      "DllInstall"
#define DLLREGISTER     "DllRegisterServer"
#define DLLUNREGISTER   "DllUnregisterServer"
#define EXEREGSVR       TEXT("/RegServer")
#define EXEUNREGSVR     TEXT("/UnRegServer")


typedef struct _INIFILESECTION {
    PTSTR IniFileName;
    PTSTR SectionName;
    PTSTR SectionData;
    int BufferSize;
    int BufferUsed;
    struct _INIFILESECTION *Next;
} INIFILESECTION, *PINIFILESECTION;

typedef struct _INISECTIONCACHE {
    //
    // Head of section list.
    //
    PINIFILESECTION Sections;
} INISECTIONCACHE, *PINISECTIONCACHE;

typedef struct _OLE_CONTROL_DATA {
    LPTSTR              FullPath;
    UINT                RegType;
    PSETUP_LOG_CONTEXT  LogContext;

    BOOL                Register; // or unregister
    UINT                Timeout;

    LPCTSTR             Argument;

    HANDLE              hContinue; // event used to indicate OLE_CONTROL_DATA can be trashed
    UINT                ThreadTimeout; // set prior to hContinue being signalled

#ifdef WX86
    HMODULE             hWx86Dll;
        //  Wx86 DLL handle.

    WX86LOADX86DLL_ROUTINE pWx86LoadX86Dll;
        //  Ptr to Wx86 equivalent LoadLibary() function.

    WX86EMULATEX86      pWx86EmulateX86;
        //  Ptr to function that calls x86 functions.

    BOOL                bWx86LoadMode;
        //  A flag that indicates whether the last DLL loaded was a
        //  native DLL or Wx86.  This helps us optimize the loader
        //  calls because we can group the Wx86 DLLs.
#endif

} OLE_CONTROL_DATA, *POLE_CONTROL_DATA;

CONST TCHAR pszUpdateInis[]      = SZ_KEY_UPDATEINIS,
            pszUpdateIniFields[] = SZ_KEY_UPDATEINIFIELDS,
            pszIni2Reg[]         = SZ_KEY_INI2REG,
            pszAddReg[]          = SZ_KEY_ADDREG,
            pszDelReg[]          = SZ_KEY_DELREG,
            pszBitReg[]          = SZ_KEY_BITREG,
            pszRegSvr[]          = SZ_KEY_REGSVR,
            pszUnRegSvr[]        = SZ_KEY_UNREGSVR,
            pszProfileItems[]    = SZ_KEY_PROFILEITEMS;

//
// Separator chars in an ini field
//
TCHAR pszIniFieldSeparators[] = TEXT(" ,\t");

//
// Mapping between registry key specs in an inf file
// and predefined registry handles.
//
STRING_TO_DATA InfRegSpecTohKey[] = {
    TEXT("HKEY_LOCAL_MACHINE"), (UINT)((UINT_PTR)HKEY_LOCAL_MACHINE),
    TEXT("HKLM")              , (UINT)((UINT_PTR)HKEY_LOCAL_MACHINE),
    TEXT("HKEY_CLASSES_ROOT") , (UINT)((UINT_PTR)HKEY_CLASSES_ROOT),
    TEXT("HKCR")              , (UINT)((UINT_PTR)HKEY_CLASSES_ROOT),
    TEXT("HKR")               , (UINT)((UINT_PTR)NULL),
    TEXT("HKEY_CURRENT_USER") , (UINT)((UINT_PTR)HKEY_CURRENT_USER),
    TEXT("HKCU")              , (UINT)((UINT_PTR)HKEY_CURRENT_USER),
    TEXT("HKEY_USERS")        , (UINT)((UINT_PTR)HKEY_USERS),
    TEXT("HKU")               , (UINT)((UINT_PTR)HKEY_USERS),
    NULL                      , (UINT)((UINT_PTR)NULL)
};

//
// Mapping between registry value names and CM device registry property (CM_DRP) codes
//
// These values must be in the exact ordering of the SPDRP codes, as defined in setupapi.h.
// This allows us to easily map between SPDRP and CM_DRP property codes.
//
 STRING_TO_DATA InfRegValToDevRegProp[] = { pszDeviceDesc,               (UINT)CM_DRP_DEVICEDESC,
                                            pszHardwareID,               (UINT)CM_DRP_HARDWAREID,
                                            pszCompatibleIDs,            (UINT)CM_DRP_COMPATIBLEIDS,
                                            TEXT(""),                    (UINT)CM_DRP_UNUSED0,
                                            pszService,                  (UINT)CM_DRP_SERVICE,
                                            TEXT(""),                    (UINT)CM_DRP_UNUSED1,
                                            TEXT(""),                    (UINT)CM_DRP_UNUSED2,
                                            pszClass,                    (UINT)CM_DRP_CLASS,
                                            pszClassGuid,                (UINT)CM_DRP_CLASSGUID,
                                            pszDriver,                   (UINT)CM_DRP_DRIVER,
                                            pszConfigFlags,              (UINT)CM_DRP_CONFIGFLAGS,
                                            pszMfg,                      (UINT)CM_DRP_MFG,
                                            pszFriendlyName,             (UINT)CM_DRP_FRIENDLYNAME,
                                            pszLocationInformation,      (UINT)CM_DRP_LOCATION_INFORMATION,
                                            TEXT(""),                    (UINT)CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                            pszCapabilities,             (UINT)CM_DRP_CAPABILITIES,
                                            pszUiNumber,                 (UINT)CM_DRP_UI_NUMBER,
                                            pszUpperFilters,             (UINT)CM_DRP_UPPERFILTERS,
                                            pszLowerFilters,             (UINT)CM_DRP_LOWERFILTERS,
                                            TEXT(""),                    (UINT)CM_DRP_BUSTYPEGUID,
                                            TEXT(""),                    (UINT)CM_DRP_LEGACYBUSTYPE,
                                            TEXT(""),                    (UINT)CM_DRP_BUSNUMBER,
                                            TEXT(""),                    (UINT)CM_DRP_ENUMERATOR_NAME,
                                            TEXT(""),                    (UINT)CM_DRP_SECURITY,
                                            pszDevSecurity,              (UINT)CM_DRP_SECURITY_SDS,
                                            pszDevType,                  (UINT)CM_DRP_DEVTYPE,
                                            pszExclusive,                (UINT)CM_DRP_EXCLUSIVE,
                                            pszCharacteristics,          (UINT)CM_DRP_CHARACTERISTICS,
                                            TEXT(""),                    (UINT)CM_DRP_ADDRESS,
                                            pszUiNumberDescFormat,       (UINT)CM_DRP_UI_NUMBER_DESC_FORMAT,
                                            NULL,                        (UINT)0
                                         };

//
// Mapping between registry value names and CM class registry property (CM_CRP) codes
//
// These values must be in the exact ordering of the SPCRP codes, as defined in setupapi.h.
// This allows us to easily map between SPCRP and CM_CRP property codes.
//
STRING_TO_DATA InfRegValToClassRegProp[] = { TEXT(""),                  (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)0,
                                        TEXT(""),                       (UINT)CM_CRP_SECURITY,
                                        pszDevSecurity,                 (UINT)CM_CRP_SECURITY_SDS,
                                        pszDevType,                     (UINT)CM_CRP_DEVTYPE,
                                        pszExclusive,                   (UINT)CM_CRP_EXCLUSIVE,
                                        pszCharacteristics,             (UINT)CM_CRP_CHARACTERISTICS,
                                        TEXT(""),                       (UINT)0,
                                        NULL,                           (UINT)0
                                     };

//
// Linked list of RunOnce entries encountered during INF processing while
// running in unattended mode.  The contents of this list are accessed by the
// caller via pSetupAccessRunOnceNodeList, and freed via
// pSetupDestroyRunOnceNodeList.
//
// ** NOTE -- THIS LIST IS NOT THREAD SAFE, AND IS FOR USE SOLELY BY THE **
// **         SINGLE THREAD IN UMPNPMGR THAT DOES DEVICE INSTALLATIONS.  **
//
PRUNONCE_NODE RunOnceListHead = NULL;


HKEY
pSetupInfRegSpecToKeyHandle(
    IN PCTSTR InfRegSpec,
    IN HKEY   UserRootKey
    );

DWORD
pSetupValidateDevRegProp(
    IN  ULONG   CmPropertyCode,
    IN  DWORD   ValueType,
    IN  PCVOID  Data,
    IN  DWORD   DataSize,
    OUT PVOID  *ConvertedBuffer,
    OUT PDWORD  ConvertedBufferSize
    );

DWORD
pSetupValidateClassRegProp(
    IN  ULONG   CmPropertyCode,
    IN  DWORD   ValueType,
    IN  PCVOID  Data,
    IN  DWORD   DataSize,
    OUT PVOID  *ConvertedBuffer,
    OUT PDWORD  ConvertedBufferSize
    );

//
// Internal ini file routines.
//
PINIFILESECTION
pSetupLoadIniFileSection(
    IN     PCTSTR           FileName,
    IN     PCTSTR           SectionName,
    IN OUT PINISECTIONCACHE SectionList
    );

DWORD
pSetupUnloadIniFileSections(
    IN PINISECTIONCACHE SectionList,
    IN BOOL             WriteToFile
    );

PTSTR
pSetupFindLineInSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,      OPTIONAL
    IN PCTSTR          RightHandSide OPTIONAL
    );

BOOL
pSetupReplaceOrAddLineInSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide,   OPTIONAL
    IN BOOL            MatchRHS
    );

BOOL
pSetupAppendLineToSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide    OPTIONAL
    );

BOOL
pSetupDeleteLineFromSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide    OPTIONAL
    );

VOID
pSetupSetSecurityForAddRegSection(
    IN HINF Inf,
    IN PCTSTR Section,
    IN PVOID  Context
    );

DWORD
LoadNtOnlyDll(
    IN  PCTSTR DllName,
    OUT HINSTANCE *Dll_Handle
    );

DWORD
pSetupEnumInstallationSections(
    IN PVOID  Inf,
    IN PCTSTR Section,
    IN PCTSTR Key,
    IN ULONG_PTR  (*EnumCallbackFunc)(PVOID,PINFCONTEXT,PVOID),
    IN PVOID  Context
    )

/*++

Routine Description:

    Iterate all values on a line in a given section with a given key,
    treating each as the name of a section, and then pass each of the lines
    in the referenced sections to a callback function.

Arguments:

    Inf - supplies a handle to an open inf file.

    Section - supplies the name of the section in which the line whose
        values are to be iterated resides.

    Key - supplies the key of the line whose values are to be iterated.

    EnumCallbackFunc - supplies a pointer to the callback function.
        Each line in each referenced section is passed to this function.

    Context - supplies a context value to be passes through to the
        callback function.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    BOOL b;
    INFCONTEXT LineContext;
    DWORD FieldCount;
    DWORD Field;
    DWORD d;
    PCTSTR SectionName;
    INFCONTEXT FirstLineContext;

    //
    // Find the relevent line in the given install section.
    // If not present then we're done -- report success.
    //
    b = SetupFindFirstLine(Inf,Section,Key,&LineContext);
    if(!b) {
        return(NO_ERROR);
    }

    do {
        //
        // Each value on the line in the given install section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; Field<=FieldCount; Field++) {

            if((SectionName = pSetupGetField(&LineContext,Field))
            && SetupFindFirstLine(Inf,SectionName,NULL,&FirstLineContext)) {
                //
                // Call the callback routine for every line in the section.
                //
                do {
                    d = (DWORD)EnumCallbackFunc(Inf,&FirstLineContext,Context);
                    if(d != NO_ERROR) {
                        pSetupLogSectionError(Inf,NULL,NULL,SectionName,MSG_LOG_SECT_ERROR,d,Key);
                        return(d);
                    }
                } while(SetupFindNextLine(&FirstLineContext,&FirstLineContext));
            }
        }
    } while(SetupFindNextMatchLine(&LineContext,Key,&LineContext));

    SetLastError(NO_ERROR);
    return(NO_ERROR);
}


VOID
pSetupSetSecurityForAddRegSection(
    IN HINF Inf,
    IN PCTSTR Section,
    IN PVOID  Context
    )
{
    //
    // BUGBUG (lonnym) -- This routine was seriously confused as to the
    // difference between a Win32 error code and a boolean.  I just made the
    // routine be a VOID return for now, since it was so broken.
    //

    BOOL b;
//    BOOL ret;
    DWORD error;
    INFCONTEXT LineContext;
    DWORD FieldCount, LoadStatus;
    DWORD Field;
    PCTSTR SectionName;
    INFCONTEXT FirstLineContext;
    PREGMOD_CONTEXT RegModContext;
    PCTSTR RootKeySpec, SubKeyName, SecDesc;
    FARPROC SceRegUpdate;
    HKEY RootKey;
    HINSTANCE Sce_Dll;
    DWORD slot_regop = 0;


    SecDesc = NULL;
//    ret = NO_ERROR;

    //
    // Find the relevent line in the given install section.
    // If not present then we're done -- report success.
    //
    b = SetupFindFirstLine(Inf,Section,pszAddReg,&LineContext);
    if(!b) {
//        return( ret );
        return;
    }


    slot_regop = AllocLogInfoSlot(((PLOADED_INF) Inf)->LogContext,FALSE);

    do {
        //
        // Each value on the line in the given install section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; Field<=FieldCount; Field++) {


            if( (SectionName = pSetupGetField(&LineContext,Field)) &&
                (SetupFindFirstLine(Inf,SectionName,NULL,&FirstLineContext)) ){

                //
                //If security section not present then don't bother and goto next section
                //
                if( !pSetupGetSecurityInfo( Inf, SectionName, &SecDesc ))
                    continue;
                else{

                    LoadStatus = LoadNtOnlyDll( (PCTSTR)SCE_DLL, &Sce_Dll );
                    if( !LoadStatus ){
                        if (slot_regop) {
                            ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
                        }
                        SetLastError( ERROR_DLL_INIT_FAILED );
                        // return( FALSE );
                        return;
                    }

                    //
                    //Win 95 case

                    if( LoadStatus == LD_WIN95 )
                        if (slot_regop) {
                            ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
                        }
                        // return( NO_ERROR );
                        return;

                }


                //
                // Call the callback routine for every line in the section.
                //
                do {
                    RegModContext = (PREGMOD_CONTEXT)Context;
                    if(RootKeySpec = pSetupGetField(&FirstLineContext,1)) {
                        RootKey = pSetupInfRegSpecToKeyHandle(RootKeySpec, RegModContext->UserRootKey);
                        if(!RootKey) {
                            if (slot_regop) {
                                ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
                            }
                            SetLastError( ERROR_BADKEY );
                            // return( FALSE );
                            return;
                        }

                        SubKeyName = pSetupGetField(&FirstLineContext,2);

                        if ((SceRegUpdate = GetProcAddress(Sce_Dll,"SceSetupUpdateSecurityKey"))){
                            //
                            // log the fact that we're setting the security...
                            //
                            WriteLogEntry(
                                ((PLOADED_INF) Inf)->LogContext,
                                slot_regop,
                                MSG_LOG_SETTING_SECURITY_ON_SUBKEY,
                                NULL,
                                RootKeySpec,
                                (SubKeyName ? TEXT("\\") : TEXT("")),
                                (SubKeyName ? SubKeyName : TEXT("")),
                                SecDesc);

                            if((error = (DWORD)SceRegUpdate( RootKey, (PWSTR)SubKeyName, 0, (PWSTR)SecDesc))) {
                                //
                                // Log the error before calling SetLastError,
                                // otherwise we could blow away the error
                                // with our own nonsense.
                                //
                                WriteLogError(
                                    ((PLOADED_INF) Inf)->LogContext,
                                    SETUP_LOG_ERROR,
                                    error);

                                SetLastError( error );
                                // ret = FALSE;
                            }
                        } else {
                            if (slot_regop) {
                                ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
                            }
                            SetLastError( ERROR_PROC_NOT_FOUND );
                            // return( FALSE );
                            return;
                        }
                    } else {
                        if (slot_regop) {
                            ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
                        }
                        SetLastError( ERROR_INVALID_DATA );
                        // ret = FALSE;
                    }


                } while(SetupFindNextLine(&FirstLineContext,&FirstLineContext));

            }

        }
    }while(SetupFindNextMatchLine(&LineContext,pszAddReg,&LineContext));

    if (slot_regop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
    }

    // return( ret );

    //BUGBUG - No FreeLibrary commented out to increase performance by keeping DLL loaded


}








DWORD_PTR
pSetupProcessUpdateInisLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line containing update-inis directives.

    The line is expected to be in the following format:

    <filename>,<section>,<old-entry>,<new-entry>,<flags>

    <filename> supplies the filename of the ini file.

    <section> supplies the section in the ini file.

    <old-entry> is optional and if specified supplies an entry to
        be removed from the section, in the form "key=val".

    <new-entry> is optional and if specified supplies an entry to
        be added to the section, in the form "key=val".

    <flags> are optional flags
        FLG_MATCH_KEY_AND_VALUE (1)
        FLG_CONDITIONAL_ADD     (2)

Arguments:

    Inf - supplies an INF handle so we can get a LogContext.

    InfLineContext - supplies context for current line in the section.

    Context - Supplies pointer to structure describing loaded ini file sections.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR File;
    PCTSTR Section;
    PCTSTR OldLine;
    PCTSTR NewLine;
    BOOL b;
    DWORD d;
    PTCHAR Key,Value;
    PTCHAR p;
    UINT Flags;
    PINIFILESECTION SectData;
    PTSTR LineData;
    PINISECTIONCACHE IniSectionCache;

    IniSectionCache = Context;

    //
    // Get fields from the line.
    //
    File = pSetupGetField(InfLineContext,1);
    Section = pSetupGetField(InfLineContext,2);

    OldLine = pSetupGetField(InfLineContext,3);
    if(OldLine && (*OldLine == 0)) {
        OldLine = NULL;
    }

    NewLine = pSetupGetField(InfLineContext,4);
    if(NewLine && (*NewLine == 0)) {
        NewLine = NULL;
    }

    if(!SetupGetIntField(InfLineContext,5,&Flags)) {
        Flags = 0;
    }

    //
    // File and section must be specified.
    //
    if(!File || !Section) {
        return(ERROR_INVALID_DATA);
    }

    //
    // If oldline and newline are both not specified, we're done.
    //
    if(!OldLine && !NewLine) {
        return(NO_ERROR);
    }

    //
    // Open the file and section.
    //
    SectData = pSetupLoadIniFileSection(File,Section,IniSectionCache);
    if(!SectData) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // If there's an old entry specified, delete it.
    //
    if(OldLine) {

        Key = DuplicateString(OldLine);
        if(!Key) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        p = Key;

        if(Value = _tcschr(Key,TEXT('='))) {
            //
            // Delete by key.
            //
            *Value = 0;
            Value = NULL;
        } else {
            //
            // Delete by value.
            //
            Value = Key;
            Key = NULL;
        }

        pSetupDeleteLineFromSection(SectData,Key,Value);

        MyFree(p);
    }

    //
    // If there's a new entry specified, add it.
    //
    if(NewLine) {

        Key = DuplicateString(NewLine);
        if(!Key) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        p = Key;

        if(Value = _tcschr(Key,TEXT('='))) {
            //
            // There is a key. Depending on flags, we want to match
            // key only or key and value.
            //
            *Value++ = 0;
            b = ((Flags & FLG_MATCH_KEY_AND_VALUE) != 0);

        } else {
            //
            // No key. match whole line. This is the same as matching
            // the RHS only, since no line with a key can match.
            //
            Value = Key;
            Key = NULL;
            b = TRUE;
        }

        if(!pSetupReplaceOrAddLineInSection(SectData,Key,Value,b)) {
            d = ERROR_NOT_ENOUGH_MEMORY;
        }

        MyFree(p);

    }

    return(NO_ERROR);
}


BOOL
pSetupFieldPresentInIniFileLine(
    IN  PTCHAR  Line,
    IN  PCTSTR  Field,
    OUT PTCHAR *Start,
    OUT PTCHAR *End
    )
{
    TCHAR c;
    PTCHAR p,q;
    BOOL b;

    //
    // Skip the key if there is one (there should be one since we use
    // GetPrivateProfileString to query the value!)
    //
    if(p = _tcschr(Line,TEXT('='))) {
        Line = p+1;
    }

    //
    // Skip ini field separators.
    //
    Line += _tcsspn(Line,pszIniFieldSeparators);

    while(*Line) {
        //
        // Locate the end of the field.
        //
        p = Line;
        while(*p && !_tcschr(pszIniFieldSeparators,*p)) {
            if(*p == TEXT('\"')) {
                //
                // Find terminating quote. If none, ignore the quote.
                //
                if(q = _tcschr(p,TEXT('\"'))) {
                    p = q;
                }
            }
            p++;
        }

        //
        // p now points to first char past end of field.
        // Make sure the field is 0-terminated and see if we have
        // what we're looking for.
        c = *p;
        *p = 0;
        b = (lstrcmpi(Line,Field) == 0);
        *p = c;
        //
        // Skip separators so p points to first char in next field,
        // or to the terminating 0.
        //
        p += _tcsspn(p,pszIniFieldSeparators);

        if(b) {
            *Start = Line;
            *End = p;
            return(TRUE);
        }

        Line = p;
    }

    return(FALSE);
}


DWORD_PTR
pSetupProcessUpdateIniFieldsLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line containing update-ini-fields directives. Such directives
    allow individual values in ini files to be removed, added, or replaced.

    The line is expected to be in the following format:

    <filename>,<section>,<key>,<old-field>,<new-field>,<flags>

    <filename> supplies the filename of the ini file.

    <section> supplies the section in the ini file.

    <key> supplies the keyname of the line in the section in the ini file.

    <old-field> supplies the field to be deleted, if specified.

    <new-field> supplies the field to be added to the line, if specified.

    <flags> are optional flags

Arguments:

    InfLineContext - supplies context for current line in the section.

    Context - Supplies pointer to structure describing loaded ini file sections.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR File;
    PCTSTR Section;
    PCTSTR Key;
    TCHAR Value[512];
    #define BUF_SIZE (sizeof(Value)/sizeof(TCHAR))
    TCHAR CONST *Old,*New;
    PTCHAR Start,End;
    BOOL b;
    DWORD d;
    DWORD Space;
    PCTSTR Separator;
    UINT Flags;
    PINISECTIONCACHE IniSectionCache;
    PINIFILESECTION SectData;
    PTSTR Line;

    IniSectionCache = Context;

    //
    // Get fields.
    //
    File = pSetupGetField(InfLineContext,1);
    Section = pSetupGetField(InfLineContext,2);
    Key = pSetupGetField(InfLineContext,3);

    Old = pSetupGetField(InfLineContext,4);
    if(Old && (*Old == 0)) {
        Old = NULL;
    }

    New = pSetupGetField(InfLineContext,5);
    if(New && (*New == 0)) {
        New = NULL;
    }

    if(!SetupGetIntField(InfLineContext,6,&Flags)) {
        Flags = 0;
    }

    //
    // Filename, section name, and key name are mandatory.
    //
    if(!File || !Section || !Key) {
        return(ERROR_INVALID_DATA);
    }

    //
    // If oldline and newline are both not specified, we're done.
    //
    if(!Old && !New) {
        return(NO_ERROR);
    }

    //
    // Open the file and section.
    //
    SectData = pSetupLoadIniFileSection(File,Section,IniSectionCache);
    if(!SectData) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Separator = (Flags & FLG_INIFIELDS_USE_SEP2) ? TEXT(", ") : TEXT(" ");

    if(Line = pSetupFindLineInSection(SectData,Key,NULL)) {
        lstrcpyn(Value, Line, BUF_SIZE);
    } else {
        *Value = TEXT('\0');
    }

    //
    // Look for the old field if specified and remove it.
    //
    if(Old) {
        if(pSetupFieldPresentInIniFileLine(Value,Old,&Start,&End)) {
            MoveMemory(Start,End,(lstrlen(End)+1)*sizeof(TCHAR));
        }
    }

    //
    // If a replacement/new field is specified, put it in there.
    //
    if(New) {
        //
        // Calculate the number of chars that can fit in the buffer.
        //
        Space = BUF_SIZE - (lstrlen(Value) + 1);

        //
        // If there's space, stick the new field at the end of the line.
        //
        if(Space >= (DWORD)lstrlen(Separator)) {
            lstrcat(Value,Separator);
            Space -= lstrlen(Separator);
        }

        if(Space >= (DWORD)lstrlen(New)) {
            lstrcat(Value,New);
        }
    }

    //
    // Write the line back out.
    //
    b = pSetupReplaceOrAddLineInSection(SectData,Key,Value,FALSE);
    d = b ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY;

    return(d);
    #undef BUF_SIZE
}


DWORD_PTR
pSetupProcessDelRegLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line in the registry that contains delete-registry instructions.
    The line is expected to be in the following form:

    <root-spec>,<subkey>,<value-name>

    <Root-spec> is one of HKR, HKLM, etc.

    <subkey> specifies the subkey relative to Root-spec.

    <value-name> is optional. If present if specifies a value entry to be deleted
        from the key. If not present the entire key is deleted. This routine
        cannot handle deleting subtrees; the key to be deleted must not have any
        subkeys or the routine will fail.

Arguments:

    Inf - supplies an INF handle so we can get a LogContext.

    InfLineContext - supplies inf line context for the line containing
        delete-registry instructions.

    Context - supplies the address of a registry modification context
        structure used in deleting the registry value.  The structure is
        defined as:

            typedef struct _REGMOD_CONTEXT {

                DWORD               Flags;          // indicates what fields are filled in
                HKEY                UserRootKey;    // HKR
                PGUID               ClassGuid;      // INF_PFLAG_CLASSPROP
                HMACHINE            hMachine;       // INF_PFLAG_CLASSPROP
                DWORD               DevInst;        // INF_PFLAG_DEVPROP

            } REGMOD_CONTEXT, *PREGMOD_CONTEXT;

        where UserRootKey is a handle to the open inf key to be used as
        the root when HKR is specified as the root for the operation, and
        DevInst is the optional device instance handle that is supplied when
        the DelReg section is for a hardware key (i.e., under the Enum branch).
        If this handle is supplied, then the value is checked to see whether it
        is the name of a Plug&Play device registry property, and if so, the
        registry property is deleted via a CM API _as well as_ via a registry API
        (the property is stored in a different location inaccessible to the registry
        APIs under Windows NT).

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR RootKeySpec,SubKeyName,ValueName;
    HKEY RootKey,Key;
    DWORD d;
    PREGMOD_CONTEXT RegModContext = (PREGMOD_CONTEXT)Context;
    ULONG CmPropertyCode;
    DWORD slot_regop = 0;

    //
    // We shouldn't be doing this against a remote machine.
    //
    MYASSERT(!(RegModContext->hMachine));

    //
    // Get root key spec, subkey name, and value name.
    //
    d = ERROR_INVALID_DATA;
    if((RootKeySpec = pSetupGetField(InfLineContext,1))
    && (SubKeyName = pSetupGetField(InfLineContext,2))) {

        ValueName = pSetupGetField(InfLineContext,3);

        RootKey = pSetupInfRegSpecToKeyHandle(RootKeySpec, RegModContext->UserRootKey);
        if(RootKey) {
            //
            // Make an information log entry saying we are deleting a key.
            // Note that we must allow for the fact that some parts of the
            // name may be missing.
            //
            if (slot_regop == 0) {
                slot_regop = AllocLogInfoSlot(((PLOADED_INF) Inf)->LogContext,FALSE);
            }
            WriteLogEntry(
                ((PLOADED_INF) Inf)->LogContext,
                slot_regop,
                MSG_LOG_DELETING_REG_KEY,
                NULL,
                RootKeySpec,
                (SubKeyName ? SubKeyName : TEXT("")),
                (SubKeyName && ValueName
                 && *SubKeyName && *ValueName ? TEXT("\\") : TEXT("")),
                (ValueName ? ValueName : TEXT("")));

            if(ValueName) {

                if(*ValueName && !(*SubKeyName)) {
                    //
                    // If the key being used is HKR with no subkey specified, and if we
                    // are doing the DelReg for a hardware key (i.e., DevInst is non-NULL,
                    // then we need to check to see whether the value entry is the name of
                    // a device registry property.
                    //
                    if ((RegModContext->Flags & INF_PFLAG_CLASSPROP) != 0 &&
                        LookUpStringInTable(InfRegValToClassRegProp, ValueName, &CmPropertyCode)) {
                        //
                        // This value is a class registry property--we must delete the property
                        // by calling a CM API.
                        //
                        CM_Set_Class_Registry_Property(RegModContext->ClassGuid,
                                                         CmPropertyCode,
                                                         NULL,
                                                         0,
                                                         0,
                                                         RegModContext->hMachine
                                                        );
                    } else if ((RegModContext->Flags & INF_PFLAG_DEVPROP) != 0 &&
                       LookUpStringInTable(InfRegValToDevRegProp, ValueName, &CmPropertyCode)) {
                        //
                        // This value is a device registry property--we must delete the property
                        // by calling a CM API.
                        //
                        CM_Set_DevInst_Registry_Property(RegModContext->DevInst,
                                                         CmPropertyCode,
                                                         NULL,
                                                         0,
                                                         0
                                                        );
                    }
                }

                //
                // Open subkey for delete.
                //
                d = RegOpenKeyEx(
                        RootKey,
                        SubKeyName,
                        0,
                        KEY_SET_VALUE,
                        &Key
                        );

                if(d == NO_ERROR) {
                    //
                    // Do delete.
                    //
                    d = RegDeleteValue(Key,ValueName);

                    RegCloseKey(Key);
                }

                if(d == ERROR_FILE_NOT_FOUND) {
                    d = NO_ERROR;
                }
            } else {
                d = RegistryDelnode(RootKey,SubKeyName);
            }

            if (d != NO_ERROR) {
                //
                // Log that an error occurred, but I don't think that it
                // matters if it was from open or delete.
                //
                WriteLogError(
                    ((PLOADED_INF) Inf)->LogContext,
                    SETUP_LOG_ERROR,
                    d);
            }
        } else {
            d = ERROR_BADKEY;
        }
    }

    if (slot_regop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
    }
    return(d);
}


DWORD_PTR
pSetupProcessAddRegLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line in the INF that contains add-registry instructions.
    The line is expected to be in the following form:

    <root-spec>,<subkey>,<value-name>,<flags>,<value>...

    <Root-spec> is one of HKR, HKLM, etc.

    <subkey> specifies the subkey relative to Root-spec.

    <value-name> is optional. If not present the default value is set.

    <flags> is optional and supplies flags, such as to indicate the data type.
        These are the FLG_ADDREG_* flags defined in setupapi.h, and are a
        superset of those defined for Win95 in setupx.h.

    <value> is one or more values used as the data. The format depends
        on the value type. This value is optional. For REG_DWORD, the
        default is 0. For REG_SZ, REG_EXPAND_SZ, the default is the
        empty string. For REG_BINARY the default is a 0-length entry.
        For REG_MULTI_SZ the default is a single empty string.

Arguments:

    Inf - supplies an INF handle so we can get a LogContext.

    InfLineContext - supplies inf line context for the line containing
        add-registry instructions.

    Context - supplies the address of a registry modification context
        structure used in adding the registry value.  The structure is
        defined as:

            typedef struct _REGMOD_CONTEXT {

                DWORD               Flags;          // indicates what fields are filled in
                HKEY                UserRootKey;    // HKR
                PGUID               ClassGuid;      // INF_PFLAG_CLASSPROP
                HMACHINE            hMachine;       // INF_PFLAG_CLASSPROP
                DWORD               DevInst;        // INF_PFLAG_DEVPROP

            } REGMOD_CONTEXT, *PREGMOD_CONTEXT;

        where UserRootKey is a handle to the open inf key to be used as
        the root when HKR is specified as the root for the operation, and
        DevInst is the optional device instance handle that is supplied when
        the AddReg section is for a hardware key (i.e., under the Enum branch).
        If this handle is supplied, then the value is checked to see whether it
        is the name of a Plug&Play device registry property, and if so, the
        registry property is set via a CM API instead of via the registry API
        (which doesn't refer to the same location on Windows NT).
        Flags indicates if DevInst should be used, or if ClassGuid/hMachine pair should be used

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR RootKeySpec,SubKeyName,ValueName;
    PCTSTR ValueTypeSpec;
    DWORD ValueType;
    HKEY RootKey,Key;
    DWORD d = NO_ERROR;
    BOOL b;
    INT IntVal;
    DWORD Size;
    PVOID Data;
    DWORD Disposition;
    UINT Flags = 0;
    PTSTR *Array;
    PREGMOD_CONTEXT RegModContext = (PREGMOD_CONTEXT)Context;
    ULONG CmPropertyCode;
    PVOID ConvertedBuffer;
    DWORD ConvertedBufferSize;
    CONFIGRET cr;
    DWORD slot_regop = 0;

    //
    // We shouldn't be doing this against a remote machine.
    //
    MYASSERT(!(RegModContext->hMachine));

    //
    // Get root key spec.  If we can't get the root key spec, we don't do anything and
    // return NO_ERROR.
    //
    if(RootKeySpec = pSetupGetField(InfLineContext,1)) {

        RootKey = pSetupInfRegSpecToKeyHandle(RootKeySpec, RegModContext->UserRootKey);
        if(!RootKey) {
            WriteLogEntry(
                ((PLOADED_INF) Inf)->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_ADDREG_NOROOT,
                NULL);
            return(ERROR_BADKEY);
        }

        //
        // SubKeyName is optional.
        //
        SubKeyName = pSetupGetField(InfLineContext,2);

        //
        // ValueName is optional. Either NULL or "" are acceptable
        // to pass to RegSetValueEx.
        //
        ValueName = pSetupGetField(InfLineContext,3);

        //
        // If we don't have a value name, the type is REG_SZ to force
        // the right behavior in RegSetValueEx. Otherwise get the data type.
        //
        ValueType = REG_SZ;
        if(ValueName) {
            if(!SetupGetIntField(InfLineContext,4,&Flags)) {
                Flags = 0;
            }
            switch(Flags & FLG_ADDREG_TYPE_MASK) {

                case FLG_ADDREG_TYPE_SZ :
                    ValueType = REG_SZ;
                    break;

                case FLG_ADDREG_TYPE_MULTI_SZ :
                    ValueType = REG_MULTI_SZ;
                    break;

                case FLG_ADDREG_TYPE_EXPAND_SZ :
                    ValueType = REG_EXPAND_SZ;
                    break;

                case FLG_ADDREG_TYPE_BINARY :
                    ValueType = REG_BINARY;
                    break;

                case FLG_ADDREG_TYPE_DWORD :
                    ValueType = REG_DWORD;
                    break;

                case FLG_ADDREG_TYPE_NONE :
                    ValueType = REG_NONE;
                    break;

                default :
                    //
                    // If the FLG_ADDREG_BINVALUETYPE is set, then the highword
                    // can contain just about any random reg data type ordinal value.
                    //
                    if(Flags & FLG_ADDREG_BINVALUETYPE) {
                        //
                        // Disallow the following reg data types:
                        //
                        //    REG_NONE, REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ
                        //
                        ValueType = (DWORD)HIWORD(Flags);

                        if((ValueType < REG_BINARY) || (ValueType == REG_MULTI_SZ)) {
                            d = ERROR_INVALID_DATA;
                            goto clean1;
                        }

                    } else {
                        d = ERROR_INVALID_DATA;
                        goto clean1;
                    }
            }
            //
            // Presently, the append behavior flag is only supported for
            // REG_MULTI_SZ values.
            //
            if((Flags & FLG_ADDREG_APPEND) && (ValueType != REG_MULTI_SZ)) {
                d = ERROR_INVALID_DATA;
                goto clean1;
            }
        }

        //
        // On Win9x setting the unnamed value to REG_EXPAND_SZ doesn't
        // work. So we convert to REG_SZ, assuming that anyone on Win9x trying
        // to do this has done the appropriate substitutions. This is a fix
        // for the IE guys, apparently advpack.dll does the right thing to
        // make the fix below viable.
        //
        if((OSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
        && (!ValueName || (*ValueName == 0))
        && (ValueType == REG_EXPAND_SZ)) {

            ValueType = REG_SZ;
        }

        //
        // Get the data based on type.
        //
        switch(ValueType) {

        case REG_MULTI_SZ:
            if(Flags & FLG_ADDREG_APPEND) {
                //
                // This is MULTI_SZ_APPEND, which means to append the string value to
                // an existing multi_sz if it's not already there.
                //
                if(SetupGetStringField(InfLineContext,5,NULL,0,&Size)) {
                    Data = MyMalloc(Size*sizeof(TCHAR));
                    if(!Data) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean1;
                    }
                    if(SetupGetStringField(InfLineContext,5,Data,Size,NULL)) {
                        REGMOD_CONTEXT NewContext = *RegModContext;
                        if(NewContext.UserRootKey != RootKey) {
                            NewContext.Flags = 0;               // if new root, clear context flags
                            NewContext.UserRootKey = RootKey;
                        }

                        d = _AppendStringToMultiSz(
                                SubKeyName,
                                ValueName,
                                (PCTSTR)Data,
                                FALSE,           // don't allow duplicates.
                                &NewContext
                                );
                    } else {
                        d = GetLastError();
                    }
                    MyFree(Data);
                } else {
                    d = ERROR_INVALID_DATA;
                }
                goto clean1;

            } else {

                if(SetupGetMultiSzField(InfLineContext, 5, NULL, 0, &Size)) {
                    Data = MyMalloc(Size*sizeof(TCHAR));
                    if(!Data) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean1;
                    }
                    if(!SetupGetMultiSzField(InfLineContext, 5, Data, Size, NULL)) {
                        d = GetLastError();
                        MyFree(Data);
                        goto clean1;
                    }
                    Size *= sizeof(TCHAR);
                } else {
                    Size = sizeof(TCHAR);
                    Data = MyMalloc(Size);
                    if(!Data) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                        goto clean1;
                    }
                    *((PTCHAR)Data) = TEXT('\0');
                }
                break;
            }

        case REG_DWORD:
            //
            // Since the old SetupX APIs only allowed REG_BINARY, INFs had to specify REG_DWORD
            // by listing all 4 bytes separately.  Support the old format here, by checking to
            // see whether the line has 4 bytes, and if so, combine those to form the DWORD.
            //
            Size = sizeof(DWORD);
            Data = MyMalloc(sizeof(DWORD));
            if(!Data) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto clean1;
            }

            if(SetupGetFieldCount(InfLineContext) == 8) {
                //
                // Then the DWORD is specified as a list of its constituent bytes.
                //
                if(!SetupGetBinaryField(InfLineContext,5,Data,Size,NULL)) {
                    d = GetLastError();
                    MyFree(Data);
                    goto clean1;
                }
            } else {
                if(!SetupGetIntField(InfLineContext,5,(PINT)Data)) {
                    *(PINT)Data = 0;
                }
            }
            break;

        case REG_SZ:
        case REG_EXPAND_SZ:
            if(SetupGetStringField(InfLineContext,5,NULL,0,&Size)) {
                Data = MyMalloc(Size*sizeof(TCHAR));
                if(!Data) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean1;
                }
                if(!SetupGetStringField(InfLineContext,5,Data,Size,NULL)) {
                    d = GetLastError();
                    MyFree(Data);
                    goto clean1;
                }
                Size *= sizeof(TCHAR);
            } else {
                Size = sizeof(TCHAR);
                Data = DuplicateString(TEXT(""));
                if(!Data) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean1;
                }
            }
            break;

        case REG_BINARY:
        default:
            //
            // All other values are specified in REG_BINARY form (i.e., one byte per field).
            //
            if(SetupGetBinaryField(InfLineContext, 5, NULL, 0, &Size)) {
                Data = MyMalloc(Size);
                if(!Data) {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean1;
                }
                if(!SetupGetBinaryField(InfLineContext, 5, Data, Size, NULL)) {
                    d = GetLastError();
                    MyFree(Data);
                    goto clean1;
                }
            } else {
                //
                // error occurred
                // bugbug!! (jamiehun) this used to be treated as empty binary string
                // if inf's break because of this change, those inf's were bogus
                //
                d = GetLastError();
                goto clean1;
            }
            break;
        }

        //
        // Set this variable to TRUE only if this value should not be set later on in a call to
        // RegSetValueEx (e.g., if this value is a DevReg Property).
        //
        b = FALSE;

        //
        // Open/create the key.
        //
        if(SubKeyName && *SubKeyName) {
#ifdef UNICODE
            //
            // Warning--extreme hack ahead!!!
            //
            // If we're running in non-interactive mode, we cannot allow
            // RunOnce processing to happen in that context (typically, in TCB)
            // because we have no control over what crap gets registered for
            // RunOnce.  Also, we can't kick off RunOnce in the context of a
            // logged-on user, because if it's a non-administrator, some of
            // the operations may fail, and be lost forever (SWENUM is a prime
            // example of this).
            //
            // Therefore, when we're in non-interactive mode, we "swallow" any
            // AddReg directives for RunOnce entries that use rundll32, and
            // squirrel them away in a global list.  The caller (i.e.,
            // umpnpmgr) can retrieve this list and do the job of rundll32
            // manually for these entries.  If we encounter any other kinds of
            // runonce entries, we bail.
            //
            if((GlobalSetupFlags & PSPGF_NONINTERACTIVE) &&
               (RootKey == HKEY_LOCAL_MACHINE) &&
               !lstrcmpi(SubKeyName, pszPathRunOnce) &&
               ((ValueType == REG_SZ) || (ValueType == REG_EXPAND_SZ))) {

                TCHAR szBuffer[MAX_PATH];
                PTSTR p, q, DontCare;
                DWORD DllPathSize;
                PTSTR DllFullPath, DllParams;
                PSTR  DllEntryPointName;
                PRUNONCE_NODE CurNode, NewNode;
                BOOL NodeAlreadyInList;

                //
                // We're looking at a RunOnce entry--see if it's rundll32-based.
                //
                p = _tcschr((PTSTR)Data, TEXT(' '));

                if(p) {

                    *p = TEXT('\0'); // separate 1st part of string for comparison

                    if(!lstrcmpi((PTSTR)Data, TEXT("rundll32.exe")) ||
                       !lstrcmpi((PTSTR)Data, TEXT("rundll32"))) {
                        //
                        // We have ourselves a rundll32 entry!  The next
                        // component (up until we hit a comma) is the name of
                        // the DLL we're supposed to load/run.
                        //
                        // NOTE--we don't deal with the (highly unlikely) case
                        // where the path has an embedded comma in it,
                        // surrounded by quotes.  Oh well.
                        //
                        p++;

                        q = _tcschr(p, TEXT(','));

                        if(q) {

                            *(q++) = TEXT('\0'); // separate 2nd part of string

                            if(ValueType == REG_EXPAND_SZ) {
                                ExpandEnvironmentStrings(p, szBuffer, SIZECHARS(szBuffer));
                            } else {
                                lstrcpyn(szBuffer, p, (size_t)(q - p));
                            }

                            p = (PTSTR)MyGetFileTitle(szBuffer);
                            if(!_tcschr(p, TEXT('.'))) {
                                //
                                // The file doesn't have an extension--assume
                                // it's a DLL.
                                //
                                _tcscat(p, TEXT(".dll"));
                            }

                            p = DuplicateString(szBuffer);
                            if(!p) {
                                d = ERROR_NOT_ENOUGH_MEMORY;
                                goto clean0;
                            }

                            if(p == MyGetFileTitle(p)) {
                                //
                                // The filename is a simple filename--assume it
                                // exists in %windir%\system32.
                                //
                                lstrcpy(szBuffer, SystemDirectory);
                                ConcatenatePaths(szBuffer, p, SIZECHARS(szBuffer), NULL);

                            } else {
                                //
                                // The filename contains path information--get
                                // the fully-qualified path.
                                //
                                DllPathSize = GetFullPathName(
                                                  p,
                                                  SIZECHARS(szBuffer),
                                                  szBuffer,
                                                  &DontCare
                                                 );

                                if(!DllPathSize || (DllPathSize >= SIZECHARS(szBuffer))) {
                                    //
                                    // If we start failing because MAX_PATH
                                    // isn't big enough anymore, we wanna know
                                    // about it!
                                    //
                                    MYASSERT(DllPathSize < SIZECHARS(szBuffer));
                                    MyFree(p);
                                    d = GetLastError();
                                    goto clean0;
                                }
                            }

                            //
                            // No longer need temp string copy.
                            //
                            MyFree(p);

                            DllFullPath = DuplicateString(szBuffer);
                            if(!DllFullPath) {
                                d = ERROR_NOT_ENOUGH_MEMORY;
                                goto clean0;
                            }

                            //
                            // OK, now that we have the full path of the DLL,
                            // verify its digital signature.
                            //
                            d = VerifyFile(((PLOADED_INF)Inf)->LogContext,
                                           NULL,
                                           NULL,
                                           0,
                                           MyGetFileTitle(DllFullPath),
                                           DllFullPath,
                                           NULL,
                                           NULL,
                                           FALSE,
                                           NULL,
                                           NULL,
                                           NULL
                                          );

                            if(d != NO_ERROR) {
                                MyFree(DllFullPath);
                                goto clean0;
                            }

                            //
                            // The DLL seems acceptable to be loaded/run in the
                            // context of the caller.  Retrieve the entrypoint
                            // name (in ANSI), as well as the argument string
                            // to be passed to the DLL.
                            //
                            p = _tcschr(q, TEXT(' '));
                            if(p) {
                                *(p++) = TEXT('\0');
                                DllParams = DuplicateString(p);
                            } else {
                                DllParams = DuplicateString(TEXT(""));
                            }

                            if(!DllParams) {
                                d = ERROR_NOT_ENOUGH_MEMORY;
                                MyFree(DllFullPath);
                                goto clean0;
                            }

                            DllEntryPointName = UnicodeToAnsi(q);

                            if(!DllEntryPointName) {
                                d = ERROR_NOT_ENOUGH_MEMORY;
                                MyFree(DllFullPath);
                                MyFree(DllParams);
                                goto clean0;
                            }

                            //
                            // If we get to this point, we have the full DLL
                            // path, the DLL entrypoint (always in ANSI, since
                            // that's what GetProcAddress wants), and the DLL
                            // argument string.  Before we create a new node
                            // to add to our global list, scan the list to see
                            // if the node is already in there (if so, we don't
                            // need to add it again).
                            //
                            NodeAlreadyInList = FALSE;

                            if(RunOnceListHead) {

                                CurNode = NULL;

                                do {
                                    if(CurNode) {
                                        CurNode = CurNode->Next;
                                    } else {
                                        CurNode = RunOnceListHead;
                                    }

                                    if(!lstrcmpi(DllFullPath, CurNode->DllFullPath) &&
                                       !lstrcmpiA(DllEntryPointName, CurNode->DllEntryPointName) &&
                                       !lstrcmpi(DllParams, CurNode->DllParams)) {
                                        //
                                        // We have a duplicate--no need to do
                                        // the same RunOnce operation twice.
                                        //
                                        NodeAlreadyInList = TRUE;
                                        break;
                                    }

                                } while(CurNode->Next);
                            }

                            //
                            // Now create a new rundll32 node and stick it in
                            // our global list (unless it's already in there).
                            //
                            if(NodeAlreadyInList) {
                                NewNode = NULL;
                            } else {
                                NewNode = MyMalloc(sizeof(RUNONCE_NODE));
                                if(!NewNode) {
                                    d = ERROR_NOT_ENOUGH_MEMORY;
                                }
                            }

                            if(NewNode) {

                                NewNode->Next = NULL;
                                NewNode->DllFullPath = DllFullPath;
                                NewNode->DllEntryPointName = DllEntryPointName;
                                NewNode->DllParams = DllParams;

                                //
                                // Add our new node to the end of the list (we
                                // already found the end of the list above when
                                // doing our duplicate search.
                                //
                                if(RunOnceListHead) {
                                    CurNode->Next = NewNode;
                                } else {
                                    RunOnceListHead = NewNode;
                                }

                            } else {
                                //
                                // Either we couldn't allocate a new node entry
                                // (i.e., due to out-of-memory), or we didn't
                                // need to (because the node was already in the
                                // list.
                                //
                                MyFree(DllFullPath);
                                MyFree(DllEntryPointName);
                                MyFree(DllParams);
                            }

                            goto clean0;

                        } else {
                            //
                            // Improperly-formatted rundll32 entry.
                            //
                            d = ERROR_INVALID_DATA;
                            goto clean0;
                        }

                    } else {
                        //
                        // We don't know how to deal with anything else--abort!
                        //
                        d = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
                        goto clean0;
                    }

                } else {
                    //
                    // We don't know how to deal with anything else--abort!
                    //
                    d = ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION;
                    goto clean0;
                }
            }
#endif // UNICODE

            if(Flags & FLG_ADDREG_OVERWRITEONLY) {

                d = RegOpenKeyEx(
                        RootKey,
                        SubKeyName,
                        0,
                        KEY_QUERY_VALUE | KEY_SET_VALUE,
                        &Key
                        );

                Disposition = REG_OPENED_EXISTING_KEY;

            } else {
                d = RegCreateKeyEx(
                        RootKey,
                        SubKeyName,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_QUERY_VALUE | KEY_SET_VALUE,
                        NULL,
                        &Key,
                        &Disposition
                        );
            }

            if(d == NO_ERROR) {

                if(Disposition == REG_OPENED_EXISTING_KEY) {

                    //
                    // Work around hacked nonsense on Win95 where the unnamed value
                    // behaves differently.
                    //
                    if((Flags & FLG_ADDREG_NOCLOBBER)
                    && ((ValueName == NULL) || (*ValueName == 0))
                    && (OSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)) {

                        d = 0;
                        if(RegQueryValueEx(Key,TEXT(""),NULL,NULL,(BYTE *)&Disposition,&d) == NO_ERROR) {
                            //
                            // The unnamed value entry is not set.
                            //
                            Flags &= ~FLG_ADDREG_NOCLOBBER;
                        }

                        d = NO_ERROR;
                    }

                    if(Flags & FLG_ADDREG_DELVAL) {
                        //
                        // Added for compatibility with Setupx (lonnym):
                        //     If this flag is present, then the data for this value is ignored, and
                        //     the value entry is deleted.
                        //
                        b = TRUE;
                        RegDeleteValue(Key, ValueName);
                    }
                } else {
                    //
                    // Win9x gets confused and thinks the nonamed value is there
                    // so we never overwrite if noclobber is set. Turn it off here.
                    //
                    Flags &= ~FLG_ADDREG_NOCLOBBER;
                }
            }

        } else {

            d = NO_ERROR;

            //
            // If the key being used is HKR with no subkey specified, and if we
            // are doing the AddReg for a hardware key or for a ClassInstall32
            // entry, then we need to check to see whether the value entry we
            // have is the name of a device or class registry property.
            //
            if((RegModContext->Flags & INF_PFLAG_CLASSPROP) != 0 && ValueName && *ValueName &&
                LookUpStringInTable(InfRegValToClassRegProp, ValueName, &CmPropertyCode)) {

                ULONG ExistingPropDataSize = 0;

                b = TRUE;   // we're handling this name here

                //
                // This value is a class registry property--if noclobber flag
                // is set, we must verify that the property doesn't currently
                // exist.
                //

                if((!(Flags & FLG_ADDREG_NOCLOBBER)) ||
                   (CM_Get_Class_Registry_Property(RegModContext->ClassGuid,
                                                     CmPropertyCode,
                                                     NULL,
                                                     NULL,
                                                     &ExistingPropDataSize,
                                                     0,
                                                      RegModContext->hMachine) == CR_NO_SUCH_VALUE)) {
                    //
                    // Next, make sure the data is valid (doing conversion if
                    // necessary and possible).
                    //
                    if((d = pSetupValidateClassRegProp(CmPropertyCode,
                                                     ValueType,
                                                     Data,
                                                     Size,
                                                     &ConvertedBuffer,
                                                     &ConvertedBufferSize)) == NO_ERROR) {

                        if((cr = CM_Set_Class_Registry_Property(RegModContext->ClassGuid,
                                                                  CmPropertyCode,
                                                                  ConvertedBuffer ? ConvertedBuffer
                                                                                  : Data,
                                                                  ConvertedBuffer ? ConvertedBufferSize
                                                                                  : Size,
                                                                  0,
                                                                  RegModContext->hMachine)) != CR_SUCCESS) {

                            d = (cr == CR_INVALID_DEVINST) ? ERROR_NO_SUCH_DEVINST
                                                           : ERROR_INVALID_DATA;
                        }

                        if(ConvertedBuffer) {
                            MyFree(ConvertedBuffer);
                        }
                    }
                }

            } else if((RegModContext->Flags & INF_PFLAG_DEVPROP) != 0 && ValueName && *ValueName &&
               LookUpStringInTable(InfRegValToDevRegProp, ValueName, &CmPropertyCode)) {

                ULONG ExistingPropDataSize = 0;

                b = TRUE;   // we're handling this name here

                //
                // This value is a device registry property--if noclobber flag
                // is set, we must verify that the property doesn't currently
                // exist.
                //
                if((!(Flags & FLG_ADDREG_NOCLOBBER)) ||
                   (CM_Get_DevInst_Registry_Property(RegModContext->DevInst,
                                                     CmPropertyCode,
                                                     NULL,
                                                     NULL,
                                                     &ExistingPropDataSize,
                                                     0) == CR_NO_SUCH_VALUE)) {
                    //
                    // Next, make sure the data is valid (doing conversion if
                    // necessary and possible).
                    //
                    if((d = pSetupValidateDevRegProp(CmPropertyCode,
                                                     ValueType,
                                                     Data,
                                                     Size,
                                                     &ConvertedBuffer,
                                                     &ConvertedBufferSize)) == NO_ERROR) {

                        if((cr = CM_Set_DevInst_Registry_Property(RegModContext->DevInst,
                                                                  CmPropertyCode,
                                                                  ConvertedBuffer ? ConvertedBuffer
                                                                                  : Data,
                                                                  ConvertedBuffer ? ConvertedBufferSize
                                                                                  : Size,
                                                                  0)) != CR_SUCCESS) {

                            d = (cr == CR_INVALID_DEVINST) ? ERROR_NO_SUCH_DEVINST
                                                           : ERROR_INVALID_DATA;
                        }

                        if(ConvertedBuffer) {
                            MyFree(ConvertedBuffer);
                        }
                    }
                }
            }

            //
            // Regardless of whether this value is a devinst registry property,
            // we need to set the Key equal to the RootKey (So we won't think
            // it's a newly-opened key and try to close it later.
            //
            Key = RootKey;
        }

        if(d == NO_ERROR) {

            if(!b) {
                //
                // If noclobber flag is set, then make sure that the value entry doesn't already exist.
                // Also respect the keyonly flag.
                //
                if(!(Flags & FLG_ADDREG_KEYONLY)) {

                    if(Flags & FLG_ADDREG_NOCLOBBER) {
                        b = (RegQueryValueEx(Key,ValueName,NULL,NULL,NULL,NULL) != NO_ERROR);
                    } else {
                        if(Flags & FLG_ADDREG_OVERWRITEONLY) {
                            b = (RegQueryValueEx(Key,ValueName,NULL,NULL,NULL,NULL) == NO_ERROR);
                        } else {
                            b = TRUE;
                        }
                    }

                    //
                    // Set the value. Note that at this point d is NO_ERROR.
                    //
                    if(b) {
                        d = RegSetValueEx(Key,ValueName,0,ValueType,Data,Size);
                    }
                }
            }

            if(Key != RootKey) {
                RegCloseKey(Key);
            }
        } else {
            if(Flags & FLG_ADDREG_OVERWRITEONLY) {
                d = NO_ERROR;
            }
        }

#ifdef UNICODE

clean0:

#endif

        MyFree(Data);
    }

clean1:

    if(d != NO_ERROR) {
        //
        // Log that an error occurred
        //
        WriteLogEntry(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_SETTING_REG_KEY,
            NULL,
            RootKeySpec,
            (SubKeyName ? SubKeyName : TEXT("")),
            (SubKeyName && ValueName
             && *SubKeyName && *ValueName ? TEXT("\\") : TEXT("")),
            (ValueName ? ValueName : TEXT("")));

        WriteLogError(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            d);
    }

    if (slot_regop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
    }

    return d;
}

DWORD_PTR
pSetupProcessBitRegLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )

/*++

Routine Description:

    Process a line in the registry that contains bit-registry instructions.
    The line is expected to be in the following form:

    <root-spec>,<subkey>,<value-name>,<flags>,<byte-mask>,<byte-to-modify>


    <Root-spec> is one of HKR, HKLM, etc.

    <subkey> specifies the subkey relative to Root-spec.

    <value-name> is optional. If not present the default value is set.

    <flags> is optional and supplies flags, such as whether to set bits or clear
        bits.  Value    Meaning
               0        (Default) Clear bits.   (FLG_BITREG_CLEARBITS)
               1        Set bits.                   (FLG_BITREG_SETBITS)

        These are the FLG_BITREG_* flags defined in setupapi.h, and are a
        superset of those defined for Win95 in setupx.h.

    <byte-mask> is a 1-byte hexadecimal value specifying which bits to operate on.

    <byte-to-modify> is the zero based index of the byte number to modify


Arguments:

    InfLineContext - supplies inf line context for the line containing
        add-registry instructions.

    Context - supplies the address of a registry modification context
        structure used in adding the registry value.  The structure is
        defined as:

            typedef struct _REGMOD_CONTEXT {

                HKEY UserRootKey;

                DEVINST DevInst;

            } REGMOD_CONTEXT, *PREGMOD_CONTEXT;

        where UserRootKey is a handle to the open inf key to be used as
        the root when HKR is specified as the root for the operation, and
        DevInst is the optional device instance handle that is supplied when
        the BitReg section is for a hardware key (i.e., under the Enum branch).
        If this handle is supplied, then the value is checked to see whether it
        is the name of a Plug&Play device registry property, and if so, the
        registry property is set via a CM API instead of via the registry API
        (which doesn't refer to the same location on Windows NT).

Return Value:

    Win32 error code indicating outcome.

--*/

{
    PCTSTR RootKeySpec,SubKeyName,ValueName;
    PCTSTR ValueTypeSpec;
    DWORD ValueType;
    HKEY RootKey,Key;
    DWORD d = NO_ERROR;
    DWORD cb;
    BOOL b;
    INT IntVal;
    DWORD Size;
    PBYTE Data = NULL;
    BYTE Mask;
    DWORD Disposition;
    UINT Flags = 0, BitMask = 0, ByteNumber = 0;
    PREGMOD_CONTEXT RegModContext = (PREGMOD_CONTEXT)Context;
    BOOL DevOrClassProp = FALSE;
    CONFIGRET cr;
    ULONG CmPropertyCode;

    //
    // We shouldn't be doing this against a remote machine.
    //
    MYASSERT(!(RegModContext->hMachine));

    //
    // Get root key spec.  If we can't get the root key spec, we don't do anything and
    // return NO_ERROR.
    //
    if(RootKeySpec = pSetupGetField(InfLineContext,1)) {

        RootKey = pSetupInfRegSpecToKeyHandle(RootKeySpec, RegModContext->UserRootKey);
        if(!RootKey) {
            return(ERROR_BADKEY);
        }

        //
        // SubKeyName is optional.
        //
        SubKeyName = pSetupGetField(InfLineContext,2);

        //
        // ValueName is optional. Either NULL or "" are acceptable
        // to pass to RegSetValueEx.
        //
        ValueName = pSetupGetField(InfLineContext,3);

        //
        // get the flags
        //
        SetupGetIntField(InfLineContext,4,&Flags);

        //
        // get the bitmask
        //
        SetupGetIntField(InfLineContext,5,&BitMask);
        if (BitMask > 0xFF) {
            d = ERROR_INVALID_DATA;
            goto exit;
        }

        //
        // get the byte number to modify
        //
        SetupGetIntField(InfLineContext,6,&ByteNumber);


        //
        // Open the key.
        //
        if(SubKeyName && *SubKeyName) {

            d = RegOpenKeyEx(
                        RootKey,
                        SubKeyName,
                        0,
                        KEY_QUERY_VALUE | KEY_SET_VALUE,
                        &Key
                        );


            if(d == NO_ERROR) {

                //
                // ??? BugBug can I get rid of this
                //

                //
                // Work around hacked nonsense on Win95 where the unnamed value
                // behaves differently.
                //
                if( ((ValueName == NULL) || (*ValueName == 0))
                    && (OSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)) {

                    d = 0;
                    if(RegQueryValueEx(Key,TEXT(""),NULL,&ValueType,(BYTE *)&Disposition,&d) == NO_ERROR) {
                        //
                        // The unnamed value entry is not set.
                        //
                        d = ERROR_INVALID_DATA;
                    }

                }

            }
        } else {
            //
            // If the key being used is HKR with no subkey specified, and if we
            // are doing the BitReg for a hardware key or for a ClassInstall32
            // entry, then we need to check to see whether the value entry we
            // have is the name of a device or class registry property.
            //
            if((RegModContext->Flags & INF_PFLAG_CLASSPROP) && ValueName && *ValueName &&
                LookUpStringInTable(InfRegValToClassRegProp, ValueName, &CmPropertyCode)) {
                //
                // Retrieve the existing class property.
                //
                // BUGBUG (lonnym)--There's presently a bug in
                // CM_Get_Class_Registry_Property that succeeds a call with a
                // null buffer of zero-length (the proper behavior is to fail
                // the call with CR_BUFFER_SMALL).  We'll handle both cases
                // here, in case that bug doesn't get fixed.
                //
                cb = 0;
                cr = CM_Get_Class_Registry_Property(RegModContext->ClassGuid,
                                                    CmPropertyCode,
                                                    &ValueType,
                                                    NULL,
                                                    &cb,
                                                    0,
                                                    RegModContext->hMachine
                                                   );

                if((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL)) {
                    //
                    // cb contains the required size for the buffer, in bytes.
                    //
                    if(cb) {
                        Data = (PBYTE)MyMalloc(cb) ;
                        if(!Data) {
                            d = ERROR_NOT_ENOUGH_MEMORY;
                        }

                        if(d == NO_ERROR) {

                            cr = CM_Get_Class_Registry_Property(RegModContext->ClassGuid,
                                                                CmPropertyCode,
                                                                &ValueType,
                                                                Data,
                                                                &cb,
                                                                0,
                                                                RegModContext->hMachine
                                                               );

                            if(cr == CR_SUCCESS) {
                                DevOrClassProp = TRUE;
                            } else {
                                d = MapCrToSpError(cr, ERROR_INVALID_DATA);
                                MyFree(Data);
                                Data = NULL;
                            }
                        }

                    } else {
                        d = ERROR_INVALID_DATA;
                    }

                } else {
                    //
                    // We can't access the property (probably because it doesn't
                    // exist.  We return ERROR_INVALID_DATA for consistency with
                    // the return code used by SetupDiGetDeviceRegistryProperty.
                    //
                    d = ERROR_INVALID_DATA;
                }

            } else if((RegModContext->Flags & INF_PFLAG_DEVPROP) && ValueName && *ValueName &&
               (b = LookUpStringInTable(InfRegValToDevRegProp, ValueName, &CmPropertyCode))) {
                //
                // Retrieve the existing device property.
                //
                cb = 0;
                cr = CM_Get_DevInst_Registry_Property(RegModContext->DevInst,
                                                      CmPropertyCode,
                                                      &ValueType,
                                                      NULL,
                                                      &cb,
                                                      0
                                                     );

                if(cr == CR_BUFFER_SMALL) {
                    //
                    // cb contains the required size for the buffer, in bytes.
                    //
                    MYASSERT(cb);

                    Data = (PBYTE)MyMalloc(cb) ;
                    if(!Data) {
                        d = ERROR_NOT_ENOUGH_MEMORY;
                    }

                    if(d == NO_ERROR) {

                        cr = CM_Get_DevInst_Registry_Property(RegModContext->DevInst,
                                                              CmPropertyCode,
                                                              &ValueType,
                                                              Data,
                                                              &cb,
                                                              0
                                                             );
                        if(cr == CR_SUCCESS) {
                            DevOrClassProp = TRUE;
                        } else {
                            d = MapCrToSpError(cr, ERROR_INVALID_DATA);
                            MyFree(Data);
                            Data = NULL;
                        }
                    }

                } else {
                    //
                    // We can't access the property (probably because it doesn't
                    // exist.  We return ERROR_INVALID_DATA for consistency with
                    // the return code used by SetupDiGetDeviceRegistryProperty.
                    //
                    d = ERROR_INVALID_DATA;
                }
            }

            //
            // Regardless of whether this value is a device or class registry
            // property, we need to set the Key equal to the RootKey (So we
            // won't think it's a newly-opened key and try to close it later.
            //
            Key = RootKey;
        }

        if(d == NO_ERROR) {

            if(!DevOrClassProp) {

                d = RegQueryValueEx(Key,ValueName,NULL,&ValueType,NULL,&cb);
                if (d == NO_ERROR) {
                    if (cb != 0 ) {
                        Data = (PBYTE) MyMalloc( cb ) ;
                        if (!Data) {
                            d  = ERROR_NOT_ENOUGH_MEMORY;
                        }

                        if (d == NO_ERROR) {
                            d = RegQueryValueEx(Key,ValueName,NULL,&ValueType,(PBYTE)Data,&cb);
                        }
                    } else {
                        d = ERROR_INVALID_DATA;
                    }
                }
            }

            //
            // byte number is zero-based where-as "cb" isn't
            //
            if(d == NO_ERROR) {
                switch (ValueType) {
                    case REG_BINARY:
                        if (ByteNumber > (cb-1)) {
                            d = ERROR_INVALID_DATA;
                        }
                        break;
                    case REG_DWORD:
                        if (ByteNumber > 3) {
                            d = ERROR_INVALID_DATA;
                        }
                        break;

                    default:
                        d = ERROR_INVALID_DATA;
                };
            }

            if (d == NO_ERROR) {
                //
                // set the target byte based on input flags
                //
                if (Flags == FLG_BITREG_SETBITS) {
                    Data[ByteNumber] |= BitMask;
                } else {
                    Data[ByteNumber] &= ~(BitMask);
                }

                if(DevOrClassProp) {

                    if(RegModContext->Flags & INF_PFLAG_CLASSPROP) {

                        cr = CM_Set_Class_Registry_Property(RegModContext->ClassGuid,
                                                            CmPropertyCode,
                                                            Data,
                                                            cb,
                                                            0,
                                                            RegModContext->hMachine
                                                           );
                        if(cr != CR_SUCCESS) {
                            d = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        }

                    } else {

                        MYASSERT(RegModContext->Flags & INF_PFLAG_DEVPROP);

                        cr = CM_Set_DevInst_Registry_Property(RegModContext->DevInst,
                                                              CmPropertyCode,
                                                              Data,
                                                              cb,
                                                              0
                                                             );
                        if(cr != CR_SUCCESS) {
                            d = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        }
                    }

                } else {
                    d = RegSetValueEx(Key,ValueName,0,ValueType,Data,cb);
                }
            }

            if (Data) {
                MyFree(Data);
            }
        }

        if(Key != RootKey) {
            RegCloseKey(Key);
        }

    }

exit:
    return d;
}



DWORD_PTR
pSetupProcessIni2RegLine(
    IN PVOID       Inf,
    IN PINFCONTEXT InfLineContext,
    IN PVOID       Context
    )
{
    PCTSTR Filename,Section;
    PCTSTR Key,RegRootSpec,SubkeyPath;
    PTCHAR key,value;
    HKEY UserRootKey,RootKey,hKey;
    DWORD Disposition;
    PTCHAR Line;
    PTCHAR Buffer;
    DWORD d;
    TCHAR val[512];
    #define BUF_SIZE (sizeof(val)/sizeof(TCHAR))
#ifdef INF_FLAGS
    UINT Flags;
#endif
    DWORD slot_regop = 0;
    DWORD slot_subop = 0;

    UserRootKey = (HKEY)Context;

    //
    // Get filename and section name of ini file.
    //
    Filename = pSetupGetField(InfLineContext,1);
    Section = pSetupGetField(InfLineContext,2);
    if(!Filename || !Section) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Get the ini file key. If not specified,
    // use the whole section.
    //
    Key = pSetupGetField(InfLineContext,3);

    //
    // Get the reg root spec and the subkey path.
    //
    RegRootSpec = pSetupGetField(InfLineContext,4);
    SubkeyPath = pSetupGetField(InfLineContext,5);
    if(SubkeyPath && (*SubkeyPath == 0)) {
        SubkeyPath = NULL;
    }

    //
    // Translate the root key spec into an hkey
    //
    RootKey = pSetupInfRegSpecToKeyHandle(RegRootSpec,UserRootKey);
    if(!RootKey) {
        return(ERROR_BADKEY);
    }

#ifdef INF_FLAGS
    //
    // Get the flags value.
    //
    if(!SetupGetIntField(InfLineContext,6,&Flags)) {
        Flags = 0;
    }
#endif

    //
    // Get the relevent line or section in the ini file.
    //
    if(Key = pSetupGetField(InfLineContext,3)) {

        Buffer = MyMalloc(
                    (  lstrlen(Key)
                     + GetPrivateProfileString(Section,Key,TEXT(""),val,BUF_SIZE,Filename)
                     + 3)
                     * sizeof(TCHAR)
                    );

        if(!Buffer) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        Buffer[wsprintf((PTSTR)Buffer,TEXT("%s=%s"),Key,val)+1] = 0;

    } else {
        Buffer = MyMalloc(32768);
        if(!Buffer) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        if(!GetPrivateProfileSection(Section,Buffer,32768,Filename)) {
            *Buffer = 0;
        }
    }

    //
    // Open/create the relevent key.
    //
    d = NO_ERROR;
    //
    // Make an information log entry saying we are adding values.
    // Note that we must allow for the fact that the subkey
    // name may be missing.
    //
    if (slot_regop == 0) {
        slot_regop = AllocLogInfoSlot(((PLOADED_INF) Inf)->LogContext,FALSE);
    }
    WriteLogEntry(
        ((PLOADED_INF) Inf)->LogContext,
        slot_regop,
        MSG_LOG_SETTING_VALUES_IN_KEY,
        NULL,
        RegRootSpec,
        (SubkeyPath ? TEXT("\\") : TEXT("")),
        (SubkeyPath ? SubkeyPath : TEXT("")));

    if(SubkeyPath) {
        d = RegCreateKeyEx(
                RootKey,
                SubkeyPath,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_SET_VALUE,
                NULL,
                &hKey,
                &Disposition
                );
    } else {
        hKey = RootKey;
    }

    if (slot_subop == 0) {
        slot_subop = AllocLogInfoSlot(((PLOADED_INF) Inf)->LogContext,FALSE);
    }
    for(Line=Buffer; (d==NO_ERROR) && *Line; Line+=lstrlen(Line)+1) {

        //
        // Line points to the key=value pair.
        //
        key = Line;
        if(value = _tcschr(key,TEXT('='))) {
            *value++ = 0;
        } else {
            key = TEXT("");
            value = Line;
        }

        WriteLogEntry(
            ((PLOADED_INF) Inf)->LogContext,
            slot_subop,
            MSG_LOG_SETTING_REG_VALUE,
            NULL,
            key,
            value);

        //
        // Now key points to the value name and value to the value.
        //
        d = RegSetValueEx(
                hKey,
                key,
                0,
                REG_SZ,
                (CONST BYTE *)value,
                (lstrlen(value)+1)*sizeof(TCHAR)
                );
    }

    if (d != NO_ERROR) {
        //
        // Log that an error occurred, but I don't think that it
        // matters if it was from create or set.
        //
        WriteLogError(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            d);
    }

    if(hKey != RootKey) {
        RegCloseKey(hKey);
    }

    MyFree(Buffer);

    if (slot_regop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_regop);
    }
    if (slot_subop) {
        ReleaseLogInfoSlot(((PLOADED_INF) Inf)->LogContext,slot_subop);
    }

    return(d);
    #undef BUF_SIZE
}


DWORD
pSetupInstallUpdateIniFiles(
    IN HINF   Inf,
    IN PCTSTR SectionName
    )

/*++

Routine Description:

    Locate the UpdateInis= and UpdateIniField= lines in an install section
    and process each section listed therein.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD d,x;
    INISECTIONCACHE IniSectionCache;

    ZeroMemory(&IniSectionCache,sizeof(INISECTIONCACHE));

    d = pSetupEnumInstallationSections(
            Inf,
            SectionName,
            pszUpdateInis,
            pSetupProcessUpdateInisLine,
            &IniSectionCache
            );

    if(d == NO_ERROR) {

        d = pSetupEnumInstallationSections(
                Inf,
                SectionName,
                pszUpdateIniFields,
                pSetupProcessUpdateIniFieldsLine,
                &IniSectionCache
                );
    }

    x = pSetupUnloadIniFileSections(&IniSectionCache,(d == NO_ERROR));

    return((d == NO_ERROR) ? x : d);
}


DWORD
pSetupInstallRegistry(
    IN HINF            Inf,
    IN PCTSTR          SectionName,
    IN PREGMOD_CONTEXT RegContext
    )

/*++

Routine Description:

    Look for AddReg= and DelReg= directives within an inf section
    and parse them.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

    RegContext - supplies context passed into AddReg and DelReg callbacks.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD d;

    d = pSetupEnumInstallationSections(Inf,
                                       SectionName,
                                       pszDelReg,
                                       pSetupProcessDelRegLine,
                                       RegContext
                                      );

    if(d == NO_ERROR) {

        d = pSetupEnumInstallationSections(Inf,
                                           SectionName,
                                           pszAddReg,
                                           pSetupProcessAddRegLine,
                                           RegContext
                                          );

        //
        //Set Security on Keys that were created
        //
        if(d == NO_ERROR) {
            pSetupSetSecurityForAddRegSection(Inf, SectionName, RegContext);
        }
    }

    return d;
}


DWORD
pSetupInstallBitReg(
    IN HINF            Inf,
    IN PCTSTR          SectionName,
    IN PREGMOD_CONTEXT RegContext
    )

/*++

Routine Description:

    Look for BitReg= directives within an inf section and parse them.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

    RegContext - supplies context passed into AddReg and DelReg callbacks.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    return pSetupEnumInstallationSections(Inf,
                                          SectionName,
                                          pszBitReg,
                                          pSetupProcessBitRegLine,
                                          RegContext
                                         );
}


DWORD
pSetupInstallIni2Reg(
    IN HINF   Inf,
    IN PCTSTR SectionName,
    IN HKEY   UserRootKey
    )

/*++

Routine Description:


Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

Return Value:

    Win32 error code indicatinh outcome.

--*/

{
    DWORD d;

    d = pSetupEnumInstallationSections(
            Inf,
            SectionName,
            pszIni2Reg,
            pSetupProcessIni2RegLine,
            (PVOID)UserRootKey
            );

    return(d);
}

DWORD
pSetupRegisterDllInstall(
    IN POLE_CONTROL_DATA OleControlData,
    IN HMODULE ControlDll
    )
/*++

Routine Description:

    call the "DllInstall" entrypoint for the specified dll

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered

    ControlDll - module handle to the dll to be registered


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HRESULT (__stdcall *InstallRoutine) (BOOL bInstall, LPCTSTR pszCmdLine);
    HRESULT InstallStatus;

    DWORD d = NO_ERROR;

    //
    // parameter validation
    //
    if (!ControlDll) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get function pointer to "DllInstall" entrypoint
    //
    try {
        (FARPROC)InstallRoutine = GetProcAddress(
            ControlDll, DLLINSTALL );
    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ExceptionPointers) {
        //
        // something went wrong...record an error
        //
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_REGISTER_EXCEPTION,
            NULL,
            OleControlData->FullPath,
            d,
            ExceptionPointers->ExceptionRecord->ExceptionAddress,
            TEXT("GetProcAddress")
            );

        DebugPrint(TEXT("SETUP: ...exception in GetProcAddress handled\n"));

    } else if(InstallRoutine) {
        //
        // now call the function
        //
        DebugPrint(TEXT("SETUP: installing...\n"));
        try {
#ifdef WX86
            //
            //  If this is an x86 DLL then call the x86 function through the emulator.
            //
            if (OleControlData->bWx86LoadMode) {
                DWORD_PTR dwArgs[ 2 ];
                dwArgs[ 0 ] = (DWORD_PTR) OleControlData->Register;
                dwArgs[ 1 ] = (DWORD_PTR) OleControlData->Argument;
                InstallStatus = OleControlData->pWx86EmulateX86(
                    InstallRoutine,
                    2,
                    dwArgs
                    );
            } else {
                InstallStatus = InstallRoutine(OleControlData->Register, OleControlData->Argument);
            }
#else
            InstallStatus = InstallRoutine(OleControlData->Register, OleControlData->Argument);
#endif
            if(FAILED(InstallStatus)) {

                d = InstallStatus;

                WriteLogEntry(
                    OleControlData->LogContext,
                    SETUP_LOG_ERROR,
                    MSG_LOG_OLE_CONTROL_NOT_REGISTERED,
                    NULL,
                    OleControlData->FullPath,
                    TEXT(DLLINSTALL),
                    InstallStatus
                    );

            }
        } except (
            ExceptionPointers = GetExceptionInformation(),
            EXCEPTION_EXECUTE_HANDLER) {

            d = ExceptionPointers->ExceptionRecord->ExceptionCode;

            WriteLogEntry(
                OleControlData->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_OLE_CONTROL_REGISTER_EXCEPTION,
                NULL,
                OleControlData->FullPath,
                d,
                ExceptionPointers->ExceptionRecord->ExceptionAddress,
                TEXT(DLLINSTALL)
                );

            DebugPrint(TEXT("SETUP: ...exception in DllInstall handled\n"));

        }

        DebugPrint(TEXT("SETUP: ...installed\n"));
    }

    return d;

}

DWORD
pSetupRegisterDllRegister(
    IN POLE_CONTROL_DATA OleControlData,
    IN HMODULE ControlDll
    )
/*++

Routine Description:

    call the "DllRegisterServer" or "DllUnregisterServer" entrypoint for the
    specified dll

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered
    This is a copy of OleControlData from calling thread
    Inf specified is locked, but not native to this thread

    ControlDll - module handle to the dll to be registered


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HRESULT (__stdcall *RegisterRoutine) (VOID);
    HRESULT RegisterStatus;

    DWORD d = NO_ERROR;

    //
    // parameter validation
    //
    if (!ControlDll) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get the function pointer to the actual routine we want to call
    //
    try {
        (FARPROC)RegisterRoutine = GetProcAddress(
            ControlDll, OleControlData->Register ? DLLREGISTER : DLLUNREGISTER);
    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ExceptionPointers) {

        //
        // something went wrong, horribly wrong
        //
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_REGISTER_EXCEPTION,
            NULL,
            OleControlData->FullPath,
            d,
            ExceptionPointers->ExceptionRecord->ExceptionAddress,
            TEXT("GetProcAddress")
            );

        DebugPrint(TEXT("SETUP: ...exception in GetProcAddress handled\n"));

    } else if(RegisterRoutine) {

        DebugPrint(TEXT("SETUP: registering...\n"));

        try {
#ifdef WX86
            //
            //  If this is an x86 DLL then call the x86 function through the emulator.
            //
            if (OleControlData->bWx86LoadMode) {
                RegisterStatus = OleControlData->pWx86EmulateX86(
                    RegisterRoutine,
                    0,
                    NULL
                    );
            } else {
                RegisterStatus = RegisterRoutine();
            }
#else
            RegisterStatus = RegisterRoutine();
#endif
            if(FAILED(RegisterStatus)) {

                WriteLogEntry(
                    OleControlData->LogContext,
                    SETUP_LOG_ERROR,
                    MSG_LOG_OLE_CONTROL_NOT_REGISTERED,
                    NULL,
                    OleControlData->FullPath,
                    OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER) ,
                    RegisterStatus
                    );

                d = RegisterStatus;

            }

        } except (
            ExceptionPointers = GetExceptionInformation(),
            EXCEPTION_EXECUTE_HANDLER) {

            d = ExceptionPointers->ExceptionRecord->ExceptionCode;

            WriteLogEntry(
                OleControlData->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_OLE_CONTROL_REGISTER_EXCEPTION,
                NULL,
                OleControlData->FullPath,
                d,
                ExceptionPointers->ExceptionRecord->ExceptionAddress,
                OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                );

            DebugPrint(TEXT("SETUP: ...exception in DllRegisterServer handled\n"));

        }

        DebugPrint(TEXT("SETUP: ...registered\n"));

    } else {

        d = GetLastError();

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_NOT_REGISTERED_PARAM,
            NULL,
            OleControlData->FullPath,
            TEXT("GetProcAddress"),
            d,
            OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
            );

    }

    return d;
}

DWORD
pSetupRegisterLoadDll(
    IN  POLE_CONTROL_DATA OleControlData,
    OUT HMODULE *ControlDll
    )
/*++

Routine Description:

    get the module handle to the specified dll

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered

    ControlDll - module handle for the dll


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;

    DWORD d = NO_ERROR;

    DebugPrint(TEXT("SETUP: loading dll...\n"));

    try {
#ifdef WX86
        //  Try to load the DLL based on the architecture of the previous DLL
        //  loaded.
        if (OleControlData->bWx86LoadMode) {
            *ControlDll = OleControlData->pWx86LoadX86Dll(
                OleControlData->FullPath,
                LOAD_WITH_ALTERED_SEARCH_PATH
                );
        } else {
            *ControlDll = LoadLibrary(OleControlData->FullPath);
        }

        //
        //  If the load fails try the other architecture
        //
        if (!*ControlDll && (GetLastError() != ERROR_FILE_NOT_FOUND)) {
            if (OleControlData->bWx86LoadMode)
            {
                *ControlDll = LoadLibrary(OleControlData->FullPath);
            } else {
                if (OleControlData->hWx86Dll != NULL) {
                    *ControlDll = OleControlData->pWx86LoadX86Dll(
                        OleControlData->FullPath,
                        LOAD_WITH_ALTERED_SEARCH_PATH
                        );

                    if (!*ControlDll) {
                        DebugPrint(TEXT("SETUP: Wx86LoadX86Dll() could not load %ws.\n"), OleControlData->FullPath);
                    }

                }
            }

            if (*ControlDll) {
                //  If this load worked toggle the architecture flag for next time.
                OleControlData->bWx86LoadMode = !OleControlData->bWx86LoadMode;
            }
        }
#else
        *ControlDll = LoadLibrary(OleControlData->FullPath);
#endif
    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ExceptionPointers) {

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_REGISTER_EXCEPTION,
            NULL,
            OleControlData->FullPath,
            ExceptionPointers->ExceptionRecord->ExceptionCode,
            ExceptionPointers->ExceptionRecord->ExceptionAddress,
            TEXT("LoadLibrary")
            );

        DebugPrint(TEXT("SETUP: ...exception in LoadLibrary handled\n"));
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

    } else if (!*ControlDll) {
        d = GetLastError();

        //
        // LoadLibrary failed.
        // File not found is not an error. We want to know about
        // other errors though.
        //

        d = GetLastError();

        DebugPrint(TEXT("SETUP: ...dll not loaded (%u)\n"),d);

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_NOT_REGISTERED,
            NULL,
            OleControlData->FullPath,
            TEXT("LoadLibrary"),
            d
            );

    } else {
        DebugPrint(TEXT("SETUP: ...dll loaded\n"));
    }

    return d;

}

DWORD
pSetupRegisterExe(
    POLE_CONTROL_DATA OleControlData
    )
/*++

Routine Description:

    register an exe by passing it the specified cmdline

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered

Return Value:

    Win32 error code indicating outcome.

--*/
{
    TCHAR CmdLine[MAX_PATH *2];
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;

    DWORD d = NO_ERROR;

    //
    // parameter validation
    //
    if (!OleControlData) {
        return ERROR_INVALID_DATA;
    }

    //
    // get the cmdline for the executable
    //
    wsprintf( CmdLine, TEXT("%s %s"),
              OleControlData->FullPath,
              (OleControlData->Argument ? OleControlData->Argument :
                                                (OleControlData->Register ? EXEREGSVR : EXEUNREGSVR)) );

    //
    // no UI
    //
    GetStartupInfo(&StartupInfo);
    StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    if (! CreateProcess(NULL,
                        CmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        DETACHED_PROCESS|NORMAL_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation)) {

        d = GetLastError() ;

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_NOT_REGISTERED,
            NULL,
            OleControlData->FullPath,
            TEXT("CreateProcess"),
            d
            );

    } else {

        CloseHandle( ProcessInformation.hThread );

        //
        // wait the specified timeout period for the process to complete...
        // note that this is not occuring in the mainline registration thread
        // so we don't have to pump messages.
        //
        d = WaitForSingleObject( (HANDLE) ProcessInformation.hProcess , OleControlData->Timeout );

        if (d == WAIT_OBJECT_0) {
            d = NO_ERROR;
        } else  {
            WriteLogEntry(
                OleControlData->LogContext,
                SETUP_LOG_ERROR,
                MSG_OLE_REGISTRATION_HUNG,
                NULL,
                OleControlData->FullPath
                );

            d = WAIT_TIMEOUT;
        }


        CloseHandle( ProcessInformation.hProcess );

    }

    return d;

}



DWORD
__stdcall
pSetupRegisterUnregisterDll(
    VOID *Param
    )
/*++

Routine Description:

    main registration routine for registering exe's and dlls.

Arguments:

    Param - pointer to OLE_CONTROL_DATA structure indicating file to
            be processed

Return Value:

    Win32 error code indicating outcome.

--*/
{
    POLE_CONTROL_DATA OleControlData = (POLE_CONTROL_DATA) Param;
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HMODULE ControlDll = NULL;
    PTSTR Extension;
    DWORD d = NO_ERROR;
    BOOL CanAbandon = FALSE;

    if (!OleControlData) {
        d = ERROR_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // check if we are dealing with an executable.  If so we need to
    // do something different
    //
    Extension = _tcsrchr(OleControlData->FullPath, TEXT('.'));
    if (Extension == NULL) {
        d = ERROR_INVALID_PARAMETER;
        goto clean0;
    }
    Extension++;

    //
    // could use CoInitializeEx as an optimization as OleInitialize is
    // probably overkill...but this is probably just a perf hit at
    // worst
    //
    DebugPrint(TEXT("SETUP: calling OleInitialize\n"));

    d = (DWORD)OleInitialize(NULL);
    if (d!= NO_ERROR) {
        goto clean0;
    }

    DebugPrint(TEXT("SETUP: back from OleInitialize\n"));

    if (!lstrcmpi(Extension, TEXT("EXE"))) {

        d = pSetupRegisterExe( OleControlData );

    } else {

        //
        // in DLL case, we protect ourselves from DLL's that don't return
        // amongst other things
        //
        // we make a copy of OleControlData
        // so we can tell caller that it's safe to abandon us
        //
        OLE_CONTROL_DATA OleControlDataCopy = {0};

        if (OleControlData->FullPath) {
            OleControlDataCopy.FullPath = DuplicateString(OleControlData->FullPath);
            if (OleControlDataCopy.FullPath == NULL) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto clean2;
            }
        }
        if (OleControlData->Argument) {
            OleControlDataCopy.Argument = DuplicateString(OleControlData->Argument);
            if (OleControlDataCopy.Argument == NULL) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto clean2;
            }
        }

        d = InheritLogContext(OleControlData->LogContext,&OleControlDataCopy.LogContext);
        if (d != NO_ERROR) {
            goto clean2;
        }

        OleControlDataCopy.RegType = OleControlData->RegType;
        OleControlDataCopy.Register = OleControlData->Register;
        OleControlDataCopy.Timeout = OleControlData->Timeout;
        OleControlDataCopy.hContinue = OleControlData->hContinue;
#ifdef WX86
        OleControlDataCopy.hWx86Dll = OleControlData->hWx86Dll;
        OleControlDataCopy.pWx86LoadX86Dll = OleControlData->pWx86LoadX86Dll;
        OleControlDataCopy.pWx86EmulateX86 = OleControlData->pWx86EmulateX86;
        OleControlDataCopy.bWx86LoadMode = OleControlData->bWx86LoadMode;
#endif

        //
        // if we get here, we no longer depend on OleControlData and thread can be abondoned
        //
        SetEvent(OleControlDataCopy.hContinue);
        CanAbandon = TRUE;

        try {
            //
            // protect everything in TRY-EXCEPT, we're calling unknown code (DLL's)
            //
            d = pSetupRegisterLoadDll( &OleControlDataCopy, &ControlDll );

            if (d == NO_ERROR) {

                //
                // We successfully loaded it.  Now call the appropriate routines.
                //
                //
                // On register, do DLLREGISTER, then DLLINSTALL
                // On unregister, do DLLINSTALL, then DLLREGISTER
                //
                if (OleControlDataCopy.Register) {

                    if (OleControlDataCopy.RegType & FLG_REGSVR_DLLREGISTER && (d == NO_ERROR) ) {

                        d = pSetupRegisterDllRegister( &OleControlDataCopy, ControlDll );

                    }

                    if (OleControlDataCopy.RegType & FLG_REGSVR_DLLINSTALL && (d == NO_ERROR) ) {

                        d = pSetupRegisterDllInstall( &OleControlDataCopy, ControlDll );
                    }

                } else {

                    if (OleControlDataCopy.RegType & FLG_REGSVR_DLLINSTALL && (d == NO_ERROR) ) {

                        d = pSetupRegisterDllInstall( &OleControlDataCopy, ControlDll );
                    }

                    if (OleControlDataCopy.RegType & FLG_REGSVR_DLLREGISTER && (d == NO_ERROR) ) {

                        d = pSetupRegisterDllRegister( &OleControlDataCopy, ControlDll );

                    }


                }

            } else {
                ControlDll = NULL;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // If our exception was an AV, then use Win32 invalid param error, otherwise, assume it was
            // an inpage error dealing with a mapped-in file.
            //
            d = ERROR_INVALID_DATA;
        }
clean2:
        if (ControlDll) {
            FreeLibrary(ControlDll);
        }
        if (OleControlDataCopy.FullPath) {
            MyFree(OleControlDataCopy.FullPath);
        }
        if (OleControlDataCopy.Argument) {
            MyFree(OleControlDataCopy.Argument);
        }
        if(OleControlDataCopy.LogContext) {
            DeleteLogContext(OleControlDataCopy.LogContext);
        }
    }

//clean1:

    OleUninitialize();

    DebugPrint(TEXT("SETUP: back from OleUninitialize, exit RegisterOlecontrols\n"));

clean0:

    if(!CanAbandon) {
        //
        // we made calling thread wait for us thus far, exit in such a way caller will wait for us to exit
        //
        OleControlData->ThreadTimeout = INFINITE;
        SetEvent(OleControlData->hContinue);
    }

    return d;

}


DWORD
pSetupProcessRegSvrSection(
    IN HINF   Inf,
    IN PCTSTR Section,
    IN BOOL   Register
    )
/*++

Routine Description:

    process all of the registration directives in the specefied RegisterDlls
    section

    each line is expected to be in the following format:

    <dirid>,<subdir>,<filename>,<registration flags>,<optional timeout>,<arguments>

    <dirid> supplies the base directory id of the file.

    <subdir> if specified, specifies the subdir from the base directory
             where the file resides

    <filename> specifies the name of the file to be registered

    <registration flags> specifies the registration action to be taken

        FLG_REGSVR_DLLREGISTER      ( 0x00000001 )
        FLG_REGSVR_DLLINSTALL       ( 0x00000002 )

    <optional timeout> specifies how long to wait for the registration to
                       complete.  if not specified, use the default timeout

    <arguments>  if specified, contains the cmdline to pass to an executable
                 if we're not handling an EXE, this argument is ignored

Arguments:

    Inf        - Inf handle for the section to be processed
    Section    - name of the section to be processed
    Register   - if TRUE, we are registering items, if FALSE, we are
                 unregistering.  this allows the inf to share one section
                 for install and uninstall

Return Value:

    Win32 error code indicating outcome.

--*/
{
    //
    // BUGBUG!!!! (jamiehun 8/31/99) this code needs to be re-written post 5.0
    //
    DWORD dircount,d = NO_ERROR;
    DWORD Line,LineCount;
    INFCONTEXT InfLine;
    PCTSTR DirId,Subdir,FileName, Args;
    UINT RegType, Timeout;
    PCTSTR FullPathTemp;
    TCHAR FullPath[MAX_PATH];
    TCHAR pwd[MAX_PATH];
    OLE_CONTROL_DATA OleControlData = {0};
    intptr_t Thread;
    unsigned ThreadId;
    DWORD WaitResult;
    BOOL PartialCleanup = FALSE;

    //
    // save the current directory so we can restore it later on
    //
    dircount = GetCurrentDirectory(MAX_PATH,pwd);
    if(!dircount || (dircount >= MAX_PATH)) {
        pwd[0] = 0;
    }

    Line = 0;

    OleControlData.Register = Register;
    try {
        if(Inf == NULL || Inf == (HINF)INVALID_HANDLE_VALUE || !LockInf((PLOADED_INF)Inf)) {
            d = ERROR_INVALID_PARAMETER;
            leave;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }
    if (d!=NO_ERROR) {
        MYASSERT(d==NO_ERROR);
        goto clean0;
    }
    d = InheritLogContext(((PLOADED_INF)Inf)->LogContext,&OleControlData.LogContext);
    UnlockInf((PLOADED_INF)Inf);
    if(d!=NO_ERROR) {
        goto clean0;
    }
    OleControlData.hContinue = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (!OleControlData.hContinue) {
        d = GetLastError();
        goto clean0;
    }

    LineCount = SetupGetLineCount(Inf, Section);
    //
    // caller should make sure we have a non-empty section
    //
    MYASSERT( LineCount > 0 );

    //
    // retrieve the items from section and process them one at a time
    //
    if(SetupFindFirstLine(Inf,Section,NULL,&InfLine)) {

#ifdef WX86
        //
        //  Load Wx86 and get pointers to our DLL loader and thunking functions.
        //   -- note that we only have to do this once.
        //
        OleControlData.bWx86LoadMode = FALSE;

        OleControlData.hWx86Dll = LoadLibrary(TEXT("wx86.dll"));
        if (OleControlData.hWx86Dll != NULL) {
            OleControlData.pWx86LoadX86Dll = (WX86LOADX86DLL_ROUTINE) GetProcAddress(
                OleControlData.hWx86Dll,
                "Wx86LoadX86Dll"
                );

            OleControlData.pWx86EmulateX86 = ( WX86EMULATEX86 ) GetProcAddress(
                OleControlData.hWx86Dll,
                "Wx86EmulateX86"
                );

            if ((OleControlData.pWx86LoadX86Dll == NULL)
                || (OleControlData.pWx86EmulateX86 == NULL)) {
                DebugPrint( TEXT("SETUP: Failed to locate one or more Wx86 on-the-fly functions.  X86 DLLs will not be properly registered!!"));
                FreeLibrary(OleControlData.hWx86Dll);
                OleControlData.hWx86Dll = NULL;
            }
        } else {
            DebugPrint( TEXT("SETUP: Failed to load Wx86 on-the-fly.  X86 DLLs will not be properly registered!!"));
        }
#endif

        do {
            //
            // retrieve pointers to the parameters for this file
            //

            DirId = pSetupGetField(&InfLine,1);
            Subdir = pSetupGetField(&InfLine,2);
            FileName = pSetupGetField(&InfLine,3);
            RegType = 0;
            SetupGetIntField(&InfLine,4,&RegType);
            Timeout = 0;
            SetupGetIntField(&InfLine,5,&Timeout);
            Args = pSetupGetField(&InfLine,6);
            Line++;

            if (!Timeout) {
                Timeout = REGISTER_WAIT_TIMEOUT_DEFAULT;
            }

            //
            // timeout is specified in seconds, we need to convert to millseconds
            //
            Timeout = Timeout * TIME_SCALAR;

            if(DirId && FileName) {

                if(Subdir && (*Subdir == 0)) {
                    Subdir = NULL;
                }

                DebugPrint(TEXT("SETUP: filename for file to register is %ws\n"),FileName);

                //
                // Get full path to the file
                //
                if(FullPathTemp = pGetPathFromDirId(DirId,Subdir,Inf)) {

                    lstrcpyn(FullPath,FullPathTemp,MAX_PATH);
                    SetCurrentDirectory(FullPath);
                    ConcatenatePaths(FullPath,FileName,MAX_PATH,NULL);

                    OleControlData.FullPath = FullPath;
                    OleControlData.RegType = RegType;
                    OleControlData.Argument = Args;
                    OleControlData.Timeout = Timeout;
                    OleControlData.ThreadTimeout = Timeout;
                    ResetEvent(OleControlData.hContinue);

                    //
                    // handle the file in another thread incase is deadlocks
                    // or takes a long time
                    //
                    Thread = _beginthreadex(
                                   NULL,
                                   0,
                                   pSetupRegisterUnregisterDll,
                                   &OleControlData,
                                   0,
                                   &ThreadId
                                   );

                    if(!Thread) {
                        //
                        // assume OOM
                        //
                        return(ERROR_NOT_ENOUGH_MEMORY);
                    } else {

                        HANDLE hEvents[2];
                        int CurEvent = 0;

                        //
                        // wait until thread has done a minimal amount of work
                        // when this work is done, we can re-use or trash this structure
                        // and we know timeout for this thread
                        //
                        hEvents[0] = (HANDLE)OleControlData.hContinue;  // INFINITE
                        hEvents[1] = (HANDLE)Thread;                    // ThreadTimeout

                        do {
                            WaitResult = MyMsgWaitForMultipleObjectsEx(
                                1,
                                &hEvents[CurEvent],
                                CurEvent ? OleControlData.ThreadTimeout : INFINITE,
                                QS_ALLINPUT,
                                MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
                            switch (WaitResult) {
                                case WAIT_OBJECT_0 + 1: { // Process gui messages
                                    MSG msg;

                                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                                        TranslateMessage(&msg);
                                        DispatchMessage(&msg);
                                    }

                                    break;
                                }

                                case WAIT_IO_COMPLETION:
                                    break;

                                case WAIT_OBJECT_0:
                                    CurEvent++; // wait for next object
                                    break;

                                case WAIT_TIMEOUT:
                                default:
                                    MYASSERT(CurEvent==1);
                                    CurEvent = 2; // early termination
                                    break;
                            }
                        }
                        while(CurEvent<2);

                        if (WaitResult == WAIT_TIMEOUT) {
                            //
                            // the ole registration is hung
                            //  log an error
                            //
                            WriteLogEntry(
                                OleControlData.LogContext,
                                SETUP_LOG_ERROR,
                                MSG_OLE_REGISTRATION_HUNG,
                                NULL,
                                FullPath
                                );

                            d = WAIT_TIMEOUT;
                            //
                            // we can't guarentee everything
                            //
                            PartialCleanup = TRUE;

                        } else {

                            GetExitCodeThread((HANDLE)Thread,&d);

                        }

                        CloseHandle( (HANDLE)Thread );

                    }

                    MyFree(FullPathTemp);
                }

            } else {
                DebugPrint(TEXT("SETUP: dll skipped, bad dirid\n"));
                WriteLogEntry(
                    OleControlData.LogContext,
                    SETUP_LOG_ERROR,
                    MSG_LOG_CANT_OLE_CONTROL_DIRID,
                    NULL,
                    FileName,
                    DirId
                    );

                d = ERROR_INVALID_DATA;

            }

        } while(SetupFindNextLine(&InfLine,&InfLine)  && d == NO_ERROR );
    } else {

            WriteLogEntry(
                OleControlData.LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_OLE_CONTROL_LINE_ERROR,
                NULL,
                Line,
                Section,
                ((PLOADED_INF) Inf)->OriginalInfName
                );

            d = ERROR_INVALID_DATA;
    }

    //
    // cleanup
    //

clean0:

#ifdef WX86
    if (OleControlData.hWx86Dll && !PartialCleanup) {
        FreeLibrary(OleControlData.hWx86Dll);
    }
#endif

    if(OleControlData.LogContext) {
        DeleteLogContext(OleControlData.LogContext); // this is ref-counted
    }

    //
    // put back the current working directory
    //
    if (pwd && pwd[0]) {
        SetCurrentDirectory(pwd);
    }

    return d;

}


DWORD
pSetupInstallRegisterUnregisterDlls(
    IN HINF   Inf,
    IN PCTSTR SectionName,
    IN BOOL   Register
    )

/*++

Routine Description:

    Locate the RegisterDlls= lines in an install section
    and process each section listed therein.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.

    Register - TRUE if register, FALSE if unregister

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD d = NO_ERROR;
    INFCONTEXT LineContext;
    DWORD Field, FieldCount;
    PCTSTR SectionSpec;

    //
    // Find the RegisterDlls line in the given install section.
    // If not present then we're done with this operation.
    //


    if(!SetupFindFirstLine(  Inf
                           , SectionName
                           , Register? pszRegSvr : pszUnRegSvr
                           , &LineContext )) {
        //
        // we were successful in doing nothing
        //

        SetLastError(d);
        return d;
    }

    do {
        //
        // Each value on the line in the given install section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; d == NO_ERROR && (Field<=FieldCount); Field++) {

            if(SectionSpec = pSetupGetField(&LineContext,Field)) {

                if(SetupGetLineCount(Inf,SectionSpec) > 0) {
                    //
                    // The section exists and is not empty.
                    // So process it.
                    //
                    d = pSetupProcessRegSvrSection(Inf,SectionSpec, Register);
                    if(d!=NO_ERROR) {
                        pSetupLogSectionError(Inf,NULL,NULL,SectionSpec,MSG_LOG_SECT_ERROR,d,Register? pszRegSvr : pszUnRegSvr);
                    }
                }

            }
        }
    } while(SetupFindNextMatchLine(  &LineContext
                                   , Register? pszRegSvr : pszUnRegSvr
                                   , &LineContext));

    SetLastError(d);

    return d;

}

#ifndef ANSI_SETUPAPI

BOOL
pSetupProcessProfileSection(
    IN HINF   Inf,
    IN PCTSTR Section
    )
/*

Routine Description :

    process all the directives specified in this single ProfileItems section . This section can have the following
    directives in the listed format

    [SectionX]
    Name = <Name> (as appears in Start Menu), <Flags>, <CSIDL>
    SubDir = <subdir>
    CmdLine = <dirid>,<subdirectory>,<filename>, <args>
    IconPath = <dirid>,<subdirectory>,<filename>
    IconIndex = <index>
    WorkingDir = <dirid>,<subdirectory>
    HotKey = <hotkey>
    InfoTip = <infotip>

     Comments on the various parameters -

        By default all links are created under Start Menu\Programs. This can be over-ridden by using CSIDLs.

        Flags  - can be specified by ORing the necessary flags - OPTIONAL
                     FLG_PROFITEM_CURRENTUSER ( 0x00000001 ) - Operates on item in the current user's profile (Default is All User)
                     FLG_PROFITEM_DELETE      ( 0x00000002 ) - Operation is to delete the item (Default is to add)
                     FLG_PROFITEM_GROUP       ( 0x00000004 ) - Operation is on a group (Default is on a item)
                     FLG_PROFITEM_CSIDL       ( 0x00000008 ) - Don't default to Start Menu and use CSIDL specified - default CSIDL is 0

        CSIDL  - Used with FLG_PROFITEM_CSIDL and should be in decimal. OPTIONAL
                 Note: Will not work with FLG_PROFITEM_CURRENTUSER or FLG_PROFITEM_GROUP.
        subdir - Specify a subdirectory relative to the CSIDL group (default CSIDL group is Programs/StartMenu. OPTIONAL
        CmdLine - Required in the case of add operations but not for delete.
            dirid  - supplies the base directory id of the file. (Required if CmdLine exists)
            subdir - if specified, is the sub directory off the base directory where the file resides (Optional)
            filename - specifies the name of the binary that we are creating a link for. (Required if CmdLine exists)
            args     - If we need to specif a binary that contains spaces in its name then this can be used for args. (Optional)
        IconPath - Optional. If not specified will default to NULL
            dirid  - supplies the base directory id of the file that contains the icon. (Required if IconPath exists)
            subdir - if specified, is the sub directory off the base directory where the file resides (Optional)
            filename - specifies the name of the binary that contains the icon. (Required if IconPath exists)
        IconIndex - Optional, defaults to 0
            index - index of the icon in the executable.  Default is 0. (Optional)
        WorkingDir - Optional
            dirid  - supplies the base directory id of the working directory as needed by the shell. (Required if WorkingDir exists)
            subdir - if specified, is the sub directory off the base working directory (Optional)
        HotKey - Optional
            hotkey - hotkey code (optional)
        Infotip - Optional
            infotip - String that contains description of the link

*/
{
    PCTSTR Keys[8] = { TEXT("Name"), TEXT("SubDir"), TEXT("CmdLine"), TEXT("IconPath"), \
                        TEXT("IconIndex"), TEXT("WorkingDir"),TEXT("HotKey"), \
                        TEXT("InfoTip") };
    INFCONTEXT InfLine;
    UINT Flags, Opt_csidl, i, j, Apply_csidl;
    TCHAR CmdLine[MAX_PATH+2], IconPath[MAX_PATH+2];
    PCTSTR Name = NULL, SubDir=NULL;
    PCTSTR WorkingDir=NULL, InfoTip=NULL, Temp_Args=NULL, BadInf;
    PCTSTR Temp_DirId = NULL, Temp_Subdir = NULL, Temp_Filename = NULL, FullPathTemp = NULL;
    UINT IconIndex = 0, t=0;
    DWORD HotKey = 0;
    BOOL ret, space;
    DWORD LineCount,Err;
    PTSTR ptr;
    PCTSTR OldFileName;

    CmdLine[0]=0;
    IconPath[0]=0;

    // Get the correct name of the inf to use while logging

    BadInf = ((PLOADED_INF) Inf)->OriginalInfName ? ((PLOADED_INF) Inf)->OriginalInfName :
               ((PLOADED_INF) Inf)->VersionBlock.Filename;


    if(SetupFindFirstLine(Inf,Section,NULL,&InfLine)) {

        LineCount = SetupGetLineCount(Inf, Section);
        //
        // caller should make sure we have a non-empty section
        //
        MYASSERT( LineCount > 0 );

        ret = FALSE;

        for( i=0; LineCount && (i < 8 ); i++ ){

            if( !SetupFindFirstLine( Inf, Section, Keys[i], &InfLine ) )
                continue;

            switch( i ){
                                                                 // Name
                case 0:
                    Name = pSetupGetField( &InfLine, 1 );
                    Flags = 0x0;
                    SetupGetIntField( &InfLine, 2, &Flags );
                    Opt_csidl = 0x0;
                    if(Flags & FLG_PROFITEM_CSIDL)
                        SetupGetIntField( &InfLine, 3, &Opt_csidl );
                    break;

                case 1:                                         // SubDir
                    SubDir = pSetupGetField( &InfLine, 1 );
                    break;
                                                                // CmdLine
                case 2:
                    Temp_DirId = pSetupGetField( &InfLine, 1 );
                    Temp_Subdir = pSetupGetField( &InfLine, 2 );
                    Temp_Filename = pSetupGetField( &InfLine, 3 );
                    OldFileName = NULL;
                    Temp_Args = pSetupGetField( &InfLine, 4 );    //Not published - useful in the case of spaces in filename itself
                    if( Temp_DirId && Temp_Filename ){
                        if( Temp_Subdir && (*Temp_Subdir == 0))
                            Temp_Subdir = NULL;
                    }
                    else
                        break;

                    // Do the "quote or not to quote" to make shell happy in the different cases

                    FullPathTemp = pGetPathFromDirId(Temp_DirId,Temp_Subdir,Inf);


                    if( FullPathTemp && Temp_Filename ){
                        space = FALSE;
                        if(_tcschr(FullPathTemp, TEXT(' ')) || Temp_Args )    //Check for space in path or if args specified as seperate parameter
                           space = TRUE;

                        if( space ){
                            CmdLine[0] = TEXT('\"');
                            t = 1;
                        }
                        else
                            t = 0;
                        lstrcpyn(CmdLine+t, FullPathTemp, MAX_PATH);
                        if( space ){
                            if( Temp_Args )
                                ptr = (PTSTR)Temp_Args;
                            else{
                                ptr = NULL;
                                //
                                // Temp_Filename is a constant string.  we
                                // make a copy of it so we can manipulate it
                                //
                                //
                                OldFileName = Temp_Filename;
                                Temp_Filename = DuplicateString( OldFileName );
                                if( ptr = _tcschr(Temp_Filename, TEXT(' ')) ){   //in case of space in path look for the filename part (not argument)
                                    *ptr = 0;
                                    ptr++;
                                }
                            }
                        }
                        ConcatenatePaths(CmdLine,Temp_Filename,MAX_PATH,NULL);

                        if( space ){
                            lstrcat( CmdLine, TEXT("\""));      //put the last quote
                            if( ptr ){                          //If there is an argument concatenate it
                                lstrcat( CmdLine, TEXT(" ") );
                                lstrcat( CmdLine, ptr );
                            }

                        }
                        MyFree( FullPathTemp );
                        if (OldFileName) {
                            MyFree( Temp_Filename );
                            Temp_Filename = OldFileName;
                        }
                    }
                    break;
                                                               //Icon Path
                case 3:
                    Temp_DirId = pSetupGetField( &InfLine, 1 );
                    Temp_Subdir = pSetupGetField( &InfLine, 2 );
                    Temp_Filename = pSetupGetField( &InfLine, 3 );
                    if( Temp_DirId && Temp_Filename ){
                        if( Temp_Subdir && (*Temp_Subdir == 0))
                            Temp_Subdir = NULL;
                    }
                    else
                        break;
                    FullPathTemp = pGetPathFromDirId(Temp_DirId,Temp_Subdir,Inf);
                    if( FullPathTemp && Temp_Filename ){
                        lstrcpyn(IconPath, FullPathTemp, MAX_PATH);
                        ConcatenatePaths(IconPath,Temp_Filename,MAX_PATH,NULL);
                        MyFree( FullPathTemp );
                    }
                    break;


                case 4:                                        //Icon Index
                    SetupGetIntField( &InfLine, 1, &IconIndex );
                    break;

                case 5:                                        // Working Dir
                    Temp_DirId = pSetupGetField( &InfLine, 1 );
                    Temp_Subdir = pSetupGetField( &InfLine, 2 );
                    if( Temp_DirId ){
                        if( Temp_Subdir && (*Temp_Subdir == 0))
                            Temp_Subdir = NULL ;
                    }
                    else
                        break;
                    WorkingDir = pGetPathFromDirId(Temp_DirId,Temp_Subdir,Inf);
                    break;

                case 6:                                       // Hot Key
                    HotKey = 0;
                    SetupGetIntField( &InfLine, 1, &HotKey );
                    break;

                case 7:                                      //Info Tip
                    InfoTip = pSetupGetField( &InfLine, 1 );
                    break;

            }//switch

        }//for


        if( Name && (*Name != 0) ){

            if( Flags & FLG_PROFITEM_GROUP ){

                if( Flags & FLG_PROFITEM_DELETE ){
                    ret = DeleteGroup( Name, ((Flags & FLG_PROFITEM_CURRENTUSER) ? FALSE : TRUE) );
                    if( !ret && ( (GetLastError() == ERROR_FILE_NOT_FOUND) ||
                                  (GetLastError() == ERROR_PATH_NOT_FOUND) )){
                        ret = TRUE;
                        SetLastError( NO_ERROR );
                    }
                }else
                    ret = CreateGroup( Name, ((Flags & FLG_PROFITEM_CURRENTUSER) ? FALSE : TRUE) );
            }
            else{

                if( Flags & FLG_PROFITEM_CSIDL )
                    Apply_csidl = Opt_csidl;
                else
                    Apply_csidl = (Flags & FLG_PROFITEM_CURRENTUSER) ? CSIDL_PROGRAMS : CSIDL_COMMON_PROGRAMS;



                if( SubDir && (*SubDir == 0 ))
                    SubDir = NULL;

                if( Flags & FLG_PROFITEM_DELETE ){

                    ret = DeleteLinkFile(
                            Apply_csidl,
                            SubDir,
                            Name,
                            TRUE
                            );

                    if( !ret && ( (GetLastError() == ERROR_FILE_NOT_FOUND) ||
                                  (GetLastError() == ERROR_PATH_NOT_FOUND) )){
                        ret = TRUE;
                        SetLastError( NO_ERROR );
                    }


                }
                else{

                     if( CmdLine && (*CmdLine != 0)){

                         ret = CreateLinkFile(
                                    Apply_csidl,
                                    SubDir,
                                    Name,
                                    CmdLine,
                                    IconPath,
                                    IconIndex,
                                    WorkingDir,
                                    (WORD)HotKey,
                                    SW_SHOWNORMAL,
                                    InfoTip
                                    );
                     }else{
                        WriteLogEntry(
                            ((PLOADED_INF) Inf)->LogContext,
                            SETUP_LOG_ERROR,
                            MSG_LOG_PROFILE_BAD_CMDLINE,
                            NULL,
                            Section,
                            BadInf
                            );


                        ret = FALSE;
                        SetLastError(ERROR_INVALID_DATA);


                     }
                }





            }

            if( !ret ){

                Err = GetLastError();
                WriteLogEntry(
                    ((PLOADED_INF) Inf)->LogContext,
                    SETUP_LOG_ERROR,
                    MSG_LOG_PROFILE_OPERATION_ERROR,
                    NULL,
                    Err,
                    Section,
                    BadInf
                    );

            }



        }else{
            WriteLogEntry(
                ((PLOADED_INF) Inf)->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_PROFILE_BAD_NAME,
                NULL,
                Section,
                BadInf
                );


            ret = FALSE;
            Err = ERROR_INVALID_DATA;
        }


    }else{
        ret = FALSE;
        Err = GetLastError();
        WriteLogEntry(
            ((PLOADED_INF) Inf)->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_PROFILE_LINE_ERROR,
            NULL,
            Section,
            BadInf
            );


    }

    if( WorkingDir )
        MyFree( WorkingDir );

    if(ret)
        SetLastError( NO_ERROR );
    else
        SetLastError( Err );

    return ret;



}


DWORD
pSetupInstallProfileItems(
    IN HINF   Inf,
    IN PCTSTR SectionName
    )

/*++

Routine Description:

    Locate the ProfileItems= lines in an install section
    and process each section listed therein. Each section specified here
    will point to a section that lists the needed directives for a single
    profile item.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    SectionName - supplies name of install section.


Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD d = NO_ERROR;
    INFCONTEXT LineContext;
    DWORD Field, FieldCount;
    PCTSTR SectionSpec;

    //
    // Find the ProfileItems line in the given install section.
    // If not present then we're done with this operation.
    //


    if(!SetupFindFirstLine(  Inf,
                             SectionName,
                             pszProfileItems,
                             &LineContext )) {
        //
        // we were successful in doing nothing
        //
        SetLastError( d );
        return d;
    }

    do {
        //
        // Each value on the line in the given install section
        // is the name of another section.
        //
        FieldCount = SetupGetFieldCount(&LineContext);
        for(Field=1; d == NO_ERROR && (Field<=FieldCount); Field++) {

            if(SectionSpec = pSetupGetField(&LineContext,Field)) {

                if(SetupGetLineCount(Inf,SectionSpec) > 0) {
                    //
                    // The section exists and is not empty.
                    // So process it.
                    //
                    if(!pSetupProcessProfileSection(Inf,SectionSpec )) {
                        d = GetLastError();
                        pSetupLogSectionError(Inf,NULL,NULL,SectionSpec,MSG_LOG_SECT_ERROR,d,pszProfileItems);
                    }
                }

            }
        }
    } while(SetupFindNextMatchLine(  &LineContext,
                                     pszProfileItems,
                                     &LineContext));

    SetLastError( d );

    return d;

}
#endif

DWORD
pSetupInstallFiles(
    IN HINF              Inf,
    IN HINF              LayoutInf,         OPTIONAL
    IN PCTSTR            SectionName,
    IN PCTSTR            SourceRootPath,    OPTIONAL
    IN PSP_FILE_CALLBACK MsgHandler,        OPTIONAL
    IN PVOID             Context,           OPTIONAL
    IN UINT              CopyStyle,
    IN HWND              Owner,             OPTIONAL
    IN HSPFILEQ          UserFileQ,         OPTIONAL
    IN BOOL              IsMsgHandlerNativeCharWidth
    )

/*++

Routine Description:

    Look for file operation lines in an install section and process them.

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    LayoutInf - optionally, supplies a separate INF handle containing source
        media information about the files to be installed.  If this value is
        NULL or INVALID_HANDLE_VALUE, then it is assumed that this information
        is in the INF(s) whose handle was passed to us in the Inf parameter.

    SectionName - supplies name of install section.

    MsgHandler - supplies a callback to be used when the file queue is
        committed. Not used if UserFileQ is specified.

    Context - supplies context for callback function. Not used if UserFileQ
        is specified.

    Owner - supplies the window handle of a window to be the parent/owner
        of any dialogs that are created. Not used if UserFileQ is specified.

    UserFileQ - if specified, then this routine neither created nor commits the
        file queue. File operations are queued on this queue and it is up to the
        caller to flush the queue when it so desired. If this parameter is not
        specified then this routine creates a file queue and commits it
        before returning.

    IsMsgHandlerNativeCharWidth - indicates whether any message handler callback
        expects native char width args (or ansi ones, in the unicode build
        of this dll).

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD Field;
    unsigned i;
    PCTSTR Operations[3] = { TEXT("Copyfiles"),TEXT("Renfiles"),TEXT("Delfiles") };
    BOOL b;
    INFCONTEXT LineContext;
    DWORD FieldCount;
    PCTSTR SectionSpec;
    INFCONTEXT SectionLineContext;
    HSPFILEQ FileQueue;
    DWORD rc = NO_ERROR;
    BOOL FreeSourceRoot;
    DWORD InfSourceMediaType;

    if(!LayoutInf || (LayoutInf == INVALID_HANDLE_VALUE)) {
        LayoutInf = Inf;
    }

    //
    // Create a file queue.
    //
    if(UserFileQ) {
        FileQueue = UserFileQ;
    } else {
        FileQueue = SetupOpenFileQueue();
        if(FileQueue == INVALID_HANDLE_VALUE) {
            return(GetLastError());
        }
    }


    //
    // BugBug: The following code is broken because it implies one default source root path per INF
    // file.  While this is correct for an OEM install, it's broken for os based installs
    //
    FreeSourceRoot = FALSE;
    if(!SourceRootPath) {
        if(SourceRootPath = pSetupGetDefaultSourcePath(Inf, 0, &InfSourceMediaType)) {
            //
            // BUGBUG (lonnym): For now, if the INF is from the internet, just use
            // the default OEM source path (A:\) instead.
            //
            if(InfSourceMediaType == SPOST_URL) {
                MyFree(SourceRootPath);
            //
            // Fall back to default OEM source path.
            //
            SourceRootPath = pszOemInfDefaultPath;
            } else {
                FreeSourceRoot = TRUE;
            }
        } else {
            //
            // lock this!
            //
            if (LockInf((PLOADED_INF)Inf)) {

                if (InfIsFromOemLocation(((PLOADED_INF)Inf)->VersionBlock.Filename, TRUE)) {
                    SourceRootPath = DuplicateString(((PLOADED_INF)Inf)->VersionBlock.Filename);
                    if (SourceRootPath) {
                        PTSTR p;
                        p = _tcsrchr( SourceRootPath, TEXT('\\') );
                        if (p) *p = TEXT('\0');
                        FreeSourceRoot = TRUE;
                    }
                }

                UnlockInf((PLOADED_INF)Inf);
            }
            
        }
    }

    b = TRUE;
    for(i=0; b && (i<3); i++) {

        //
        // Find the relevent line in the given install section.
        // If not present then we're done with this operation.
        //
        if(!SetupFindFirstLine(Inf,SectionName,Operations[i],&LineContext)) {
            continue;
        }

        do {
            //
            // Each value on the line in the given install section
            // is the name of another section.
            //
            FieldCount = SetupGetFieldCount(&LineContext);
            for(Field=1; b && (Field<=FieldCount); Field++) {

                if(SectionSpec = pSetupGetField(&LineContext,Field)) {

                    //
                    // Handle single-file copy specially.
                    //
                    if((i == 0) && (*SectionSpec == TEXT('@'))) {

                        b = SetupQueueDefaultCopy(
                                FileQueue,
                                LayoutInf,
                                SourceRootPath,
                                SectionSpec + 1,
                                SectionSpec + 1,
                                CopyStyle
                                );
                        if (!b) {
                            rc = GetLastError();
                            pSetupLogSectionError(Inf,NULL,NULL,SectionSpec+1,MSG_LOG_COPYSECT_ERROR,rc,Operations[i]);
                        }

                    } else if(SetupGetLineCount(Inf,SectionSpec) > 0) {
                        //
                        // The section exists and is not empty.
                        // Add it to the copy/delete/rename queue.
                        //
                        switch(i) {
                        case 0:
                            b = SetupQueueCopySection(
                                    FileQueue,
                                    SourceRootPath,
                                    LayoutInf,
                                    Inf,
                                    SectionSpec,
                                    CopyStyle
                                    );
                            break;

                        case 1:
                            b = SetupQueueRenameSection(FileQueue,Inf,NULL,SectionSpec);
                            break;

                        case 2:
                            b = SetupQueueDeleteSection(FileQueue,Inf,NULL,SectionSpec);
                            break;
                        }
                        if (!b) {
                            rc = GetLastError();
                            pSetupLogSectionError(Inf,NULL,NULL,SectionSpec,MSG_LOG_SECT_ERROR,rc,Operations[i]);
                        }
                    }
                }
            }
        } while(SetupFindNextMatchLine(&LineContext,Operations[i],&LineContext));
    }

    if(b && (FileQueue != UserFileQ)) {
        //
        // Perform the file operations.
        //
        b = _SetupCommitFileQueue(
                Owner,
                FileQueue,
                MsgHandler,
                Context,
                IsMsgHandlerNativeCharWidth
                );
        rc = b ? NO_ERROR : GetLastError();
    }

    if(FileQueue != UserFileQ) {
        SetupCloseFileQueue(FileQueue);
    }

    if(FreeSourceRoot) {
        MyFree(SourceRootPath);
    }

    return(rc);
}


BOOL
_SetupInstallFromInfSection(
    IN HWND             Owner,              OPTIONAL
    IN HINF             InfHandle,
    IN PCTSTR           SectionName,
    IN UINT             Flags,
    IN HKEY             RelativeKeyRoot,    OPTIONAL
    IN PCTSTR           SourceRootPath,     OPTIONAL
    IN UINT             CopyFlags,
    IN PVOID            MsgHandler,
    IN PVOID            Context,            OPTIONAL
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN BOOL             IsMsgHandlerNativeCharWidth,
    IN PREGMOD_CONTEXT  RegContext          OPTIONAL
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    PDEVINFO_ELEM DevInfoElem;
    DWORD d = NO_ERROR;
    BOOL CloseRelativeKeyRoot;
    REGMOD_CONTEXT DefRegContext;

    //
    // Validate the flags passed in.
    //
    if(Flags & ~(SPINST_ALL | SPINST_SINGLESECTION | SPINST_LOGCONFIG_IS_FORCED | SPINST_LOGCONFIGS_ARE_OVERRIDES)) {
        d = ERROR_INVALID_FLAGS;
        goto clean1;
    }

    //
    // If the caller wants us to run a specific section, then they'd better
    // have told us what kind of section it is (i.e., one and only one of
    // the install action types must be specified).
    //
    // Presently, only LogConfig sections are allowed, since the other
    // flags flags encompass multiple actions (e.g., AddReg _and_ DelReg).
    //
    if((Flags & SPINST_SINGLESECTION) && ((Flags & SPINST_ALL) != SPINST_LOGCONFIG)) {
        d = ERROR_INVALID_FLAGS;
        goto clean1;
    }

    //
    // You can (optionally) specify SPINST_LOGCONFIG_IS_FORCED or SPINST_LOGCONFIGS_ARE_OVERRIDES,
    // but not both.
    //
    if((Flags & (SPINST_LOGCONFIG_IS_FORCED | SPINST_LOGCONFIGS_ARE_OVERRIDES)) ==
       (SPINST_LOGCONFIG_IS_FORCED | SPINST_LOGCONFIGS_ARE_OVERRIDES)) {

        d = ERROR_INVALID_FLAGS;
        goto clean1;
    }

    //
    // We only want to acquire the HDEVINFO lock if we're supposed to do some install
    // actions against a device instance.
    //
    if((Flags & (SPINST_REGISTRY | SPINST_BITREG | SPINST_INI2REG | SPINST_LOGCONFIG)) &&
       DeviceInfoSet && (DeviceInfoSet != INVALID_HANDLE_VALUE) && DeviceInfoData) {

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            d = ERROR_INVALID_HANDLE;
            goto clean1;
        }

    } else {
        //This is ok in the remote case, since to call pSetupInstallLogConfig
        //We won't really get here (we'll take the if case)
        pDeviceInfoSet = NULL;
    }

    d = NO_ERROR;
    DevInfoElem = NULL;
    CloseRelativeKeyRoot = FALSE;

    try {
        //
        // Get a pointer to the element for the specified device
        // instance.
        //
        if(pDeviceInfoSet) {

            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                d = ERROR_INVALID_PARAMETER;
                goto RegModsDone;
            }
        }

        if((Flags & (SPINST_REGISTRY | SPINST_BITREG | SPINST_INI2REG)) && DevInfoElem) {
            //
            // If the caller supplied a device information set and element, then this is
            // a device installation, and the registry modifications should be made to the
            // device instance's hardware registry key.
            //
            if((RelativeKeyRoot = SetupDiCreateDevRegKey(DeviceInfoSet,
                                                         DeviceInfoData,
                                                         DICS_FLAG_GLOBAL,
                                                         0,
                                                         DIREG_DEV,
                                                         NULL,
                                                         NULL)) == INVALID_HANDLE_VALUE) {
                d = GetLastError();
                goto RegModsDone;
            }

            CloseRelativeKeyRoot = TRUE;
        }

        if((Flags & SPINST_LOGCONFIG) && DevInfoElem) {

            d = pSetupInstallLogConfig(InfHandle,
                                       SectionName,
                                       DevInfoElem->DevInst,
                                       Flags,
                                       pDeviceInfoSet->hMachine);
            if(d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & SPINST_INIFILES) {
            d = pSetupInstallUpdateIniFiles(InfHandle,SectionName);
            if(d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & (SPINST_REGISTRY | SPINST_BITREG)) {

            if(!RegContext) {

                ZeroMemory(&DefRegContext, sizeof(DefRegContext));

                if(DevInfoElem) {
                    DefRegContext.Flags = INF_PFLAG_DEVPROP;
                    DefRegContext.DevInst = DevInfoElem->DevInst;
                }
                RegContext = &DefRegContext;
            }

            //
            // We check for the INF_PFLAG_HKR flag in case the caller supplied
            // us with a context that included a UserRootKey they wanted us to
            // use (i.e., instead of the RelativeKeyRoot)...
            //
            if(!(RegContext->Flags & INF_PFLAG_HKR)) {
                RegContext->UserRootKey = RelativeKeyRoot;
            }

            if(Flags & SPINST_REGISTRY) {
                d = pSetupInstallRegistry(InfHandle,
                                          SectionName,
                                          RegContext
                                         );
            }

            if((d == NO_ERROR) && (Flags & SPINST_BITREG)) {
                d = pSetupInstallBitReg(InfHandle,
                                        SectionName,
                                        RegContext
                                       );
            }

            if(d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & SPINST_INI2REG) {
            d = pSetupInstallIni2Reg(InfHandle,SectionName,RelativeKeyRoot);
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & SPINST_REGSVR) {
            d = pSetupInstallRegisterUnregisterDlls(InfHandle,SectionName, TRUE);
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }

        if(Flags & SPINST_UNREGSVR) {
            d = pSetupInstallRegisterUnregisterDlls(InfHandle,SectionName, FALSE);
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }

#ifndef ANSI_SETUPAPI

        if(Flags & SPINST_PROFILEITEMS) {
            d = pSetupInstallProfileItems(InfHandle,SectionName);
            if (d != NO_ERROR) {
                goto RegModsDone;
            }
        }
#endif


RegModsDone:

        ;       // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
        //
        // Access the following variable, so that the compiler will respect statement
        // ordering w.r.t. its assignment.
        //
        CloseRelativeKeyRoot = CloseRelativeKeyRoot;
    }

    if(CloseRelativeKeyRoot) {
        RegCloseKey(RelativeKeyRoot);
    }

    if(d == NO_ERROR) {

        if(Flags & SPINST_FILES) {

            d = pSetupInstallFiles(
                    InfHandle,
                    NULL,
                    SectionName,
                    SourceRootPath,
                    MsgHandler,
                    Context,
                    CopyFlags,
                    Owner,
                    NULL,
                    IsMsgHandlerNativeCharWidth
                    );
        }
    }

clean1:

    pSetupLogSectionError(InfHandle,DeviceInfoSet,DeviceInfoData,SectionName,MSG_LOG_INSTALLSECT_ERROR,d,NULL);

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(d);
    return(d==NO_ERROR);
}

DWORD
pSetupLogSectionError(
    IN HINF             InfHandle,
    IN HDEVINFO         DeviceInfoSet,      OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData,     OPTIONAL
    IN PCTSTR           SectionName,
    IN DWORD            MsgID,
    IN DWORD            Err,
    IN PCTSTR           KeyName             OPTIONAL
)
/*++

Routine Description:

    Log error with section context
    error will be logged at SETUP_LOG_ERROR or DRIVER_LOG_ERROR depending if DeviceInfoSet/Data given
    error will contain inf name & section name used (%2 & %1 respectively)

Arguments:

    Inf - supplies inf handle for inf containing the section indicated
        by SectionName.

    DeviceInfoSet, DeviceInfoData - supplies driver install context

    SectionName - supplies name of install section.

    MsgID - supplies ID of message string to display

    Err - supplies error

    KeyName - passed as 3rd parameter

Return Value:

    Err.

--*/

{
    DWORD d = NO_ERROR;
    BOOL inf_locked = FALSE;
    DWORD level = SETUP_LOG_ERROR;
    PDEVICE_INFO_SET pDeviceInfoSet = NULL;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    PCTSTR szInfName = NULL;

    if (Err == NO_ERROR) {
        return Err;
    }

    //
    // determine LogContext/Level
    //
    try {

        if (DeviceInfoSet != NULL) {
            if((pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))!=NULL) {
                level = DRIVER_LOG_ERROR;
                LogContext = pDeviceInfoSet->InstallParamBlock.LogContext;
                if (DeviceInfoData) {
                    if((DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                                 DeviceInfoData,
                                                                 NULL))!=NULL) {
                        LogContext = DevInfoElem->InstallParamBlock.LogContext;
                    }
                }
            }
        }
        if(InfHandle == NULL || InfHandle == INVALID_HANDLE_VALUE || !LockInf((PLOADED_INF)InfHandle)) {
            leave;
        }
        inf_locked = TRUE;
        if (LogContext == NULL) {
            LogContext = ((PLOADED_INF)InfHandle)->LogContext;
        }
        //
        // ideally we want the inf file name
        //
        szInfName = ((PLOADED_INF)InfHandle)->VersionBlock.Filename;

    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    //
    // indicate install failed, display error
    //
    WriteLogEntry(
        LogContext,
        level | SETUP_LOG_BUFFER,
        MsgID,
        NULL,
        SectionName?SectionName:TEXT("-"),
        szInfName?szInfName:TEXT("-"),
        KeyName
        );
    WriteLogError(
        LogContext,
        level,
        Err
        );

    if (inf_locked) {
        UnlockInf((PLOADED_INF)InfHandle);
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return Err;
}

#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupInstallFromInfSectionA(
    IN HWND                Owner,             OPTIONAL
    IN HINF                InfHandle,
    IN PCSTR               SectionName,
    IN UINT                Flags,
    IN HKEY                RelativeKeyRoot,   OPTIONAL
    IN PCSTR               SourceRootPath,    OPTIONAL
    IN UINT                CopyFlags,
    IN PSP_FILE_CALLBACK_A MsgHandler,
    IN PVOID               Context,           OPTIONAL
    IN HDEVINFO            DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA    DeviceInfoData     OPTIONAL
    )
{
    PCWSTR sectionName;
    PCWSTR sourceRootPath;
    BOOL b;
    DWORD d;

    sectionName = NULL;
    sourceRootPath = NULL;
    d = NO_ERROR;

    if(SectionName) {
        d = CaptureAndConvertAnsiArg(SectionName,&sectionName);
    }
    if((d == NO_ERROR) && SourceRootPath) {
        d = CaptureAndConvertAnsiArg(SourceRootPath,&sourceRootPath);
    }

    if(d == NO_ERROR) {

        b = _SetupInstallFromInfSection(
                Owner,
                InfHandle,
                sectionName,
                Flags,
                RelativeKeyRoot,
                sourceRootPath,
                CopyFlags,
                MsgHandler,
                Context,
                DeviceInfoSet,
                DeviceInfoData,
                FALSE,
                NULL
                );

        d = GetLastError();
    } else {
        b = FALSE;
    }

    if(sectionName) {
        MyFree(sectionName);
    }
    if(sourceRootPath) {
        MyFree(sourceRootPath);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupInstallFromInfSectionW(
    IN HWND                Owner,             OPTIONAL
    IN HINF                InfHandle,
    IN PCWSTR              SectionName,
    IN UINT                Flags,
    IN HKEY                RelativeKeyRoot,   OPTIONAL
    IN PCWSTR              SourceRootPath,    OPTIONAL
    IN UINT                CopyFlags,
    IN PSP_FILE_CALLBACK_W MsgHandler,
    IN PVOID               Context,           OPTIONAL
    IN HDEVINFO            DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA    DeviceInfoData     OPTIONAL
    )
{
    UNREFERENCED_PARAMETER(Owner);
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(RelativeKeyRoot);
    UNREFERENCED_PARAMETER(SourceRootPath);
    UNREFERENCED_PARAMETER(CopyFlags);
    UNREFERENCED_PARAMETER(MsgHandler);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupInstallFromInfSection(
    IN HWND              Owner,             OPTIONAL
    IN HINF              InfHandle,
    IN PCTSTR            SectionName,
    IN UINT              Flags,
    IN HKEY              RelativeKeyRoot,   OPTIONAL
    IN PCTSTR            SourceRootPath,    OPTIONAL
    IN UINT              CopyFlags,
    IN PSP_FILE_CALLBACK MsgHandler,
    IN PVOID             Context,           OPTIONAL
    IN HDEVINFO          DeviceInfoSet,     OPTIONAL
    IN PSP_DEVINFO_DATA  DeviceInfoData     OPTIONAL
    )
{
    BOOL b;

    b = _SetupInstallFromInfSection(
            Owner,
            InfHandle,
            SectionName,
            Flags,
            RelativeKeyRoot,
            SourceRootPath,
            CopyFlags,
            MsgHandler,
            Context,
            DeviceInfoSet,
            DeviceInfoData,
            TRUE,
            NULL
            );

    return(b);
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
SetupInstallFilesFromInfSectionA(
    IN HINF              InfHandle,
    IN HINF              LayoutInfHandle,   OPTIONAL
    IN HSPFILEQ          FileQueue,
    IN PCSTR             SectionName,
    IN PCSTR             SourceRootPath,    OPTIONAL
    IN UINT              CopyFlags
    )
{
    PCWSTR sectionName;
    PCWSTR sourceRootPath;
    BOOL b;
    DWORD d;


    d = CaptureAndConvertAnsiArg(SectionName,&sectionName);
    if((d == NO_ERROR) && SourceRootPath) {
        d = CaptureAndConvertAnsiArg(SourceRootPath,&sourceRootPath);
    } else {
        sourceRootPath = NULL;
    }

    if(d == NO_ERROR) {

        b = SetupInstallFilesFromInfSectionW(
                InfHandle,
                LayoutInfHandle,
                FileQueue,
                sectionName,
                sourceRootPath,
                CopyFlags
                );

        d = GetLastError();

    } else {
        b = FALSE;
    }

    if(sectionName) {
        MyFree(sectionName);
    }
    if(sourceRootPath) {
        MyFree(sourceRootPath);
    }

    SetLastError(d);
    return(b);
}
#else
//
// Unicode stub
//
BOOL
SetupInstallFilesFromInfSectionW(
    IN HINF              InfHandle,
    IN HINF              LayoutInfHandle,   OPTIONAL
    IN HSPFILEQ          FileQueue,
    IN PCWSTR            SectionName,
    IN PCWSTR            SourceRootPath,    OPTIONAL
    IN UINT              CopyFlags
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(LayoutInfHandle);
    UNREFERENCED_PARAMETER(FileQueue);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(SourceRootPath);
    UNREFERENCED_PARAMETER(CopyFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}
#endif

BOOL
SetupInstallFilesFromInfSection(
    IN HINF     InfHandle,
    IN HINF     LayoutInfHandle,    OPTIONAL
    IN HSPFILEQ FileQueue,
    IN PCTSTR   SectionName,
    IN PCTSTR   SourceRootPath,     OPTIONAL
    IN UINT     CopyFlags
    )
{
    DWORD d;

    d = pSetupInstallFiles(
            InfHandle,
            LayoutInfHandle,
            SectionName,
            SourceRootPath,
            NULL,
            NULL,
            CopyFlags,
            NULL,
            FileQueue,
            TRUE        // not used by pSetupInstallFiles with this combo of args
            );

    SetLastError(d);
    return(d == NO_ERROR);
}


HKEY
pSetupInfRegSpecToKeyHandle(
    IN PCTSTR InfRegSpec,
    IN HKEY   UserRootKey
    )
{
    BOOL b;

    // make sure the whole handle is NULL as LookUpStringTable only
    // returns 32 bits

    HKEY h = NULL;

    return(LookUpStringInTable(InfRegSpecTohKey, InfRegSpec, (PUINT)&h)
           ? (h ? h : UserRootKey)
           : NULL
          );
}


//////////////////////////////////////////////////////////////////////////////
//
// Ini file support stuff.
//
// In Win95, the UpdateIni stuff is supported by a set of TpXXX routines.
// Those routines directly manipulate the ini file, which is bad news for us
// because inis can be mapped into the registry.
//
// Thus we want to use the profile APIs. However the profile APIs make it hard
// to manipulate lines without keys, so we have to manipulate whole sections
// at a time.
//
//      [Section]
//      a
//
// There is no way to get at the line "a" with the profile APIs. But the
// profile section APIs do let us get at it.
//
//////////////////////////////////////////////////////////////////////////////

PINIFILESECTION
pSetupLoadIniFileSection(
    IN     PCTSTR           FileName,
    IN     PCTSTR           SectionName,
    IN OUT PINISECTIONCACHE SectionList
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    DWORD d;
    PTSTR SectionData;
    PVOID p;
    DWORD BufferSize;
    PINIFILESECTION Desc;
    #define BUF_GROW 4096

    //
    // See if this section is already loaded.
    //
    for(Desc=SectionList->Sections; Desc; Desc=Desc->Next) {
        if(!lstrcmpi(Desc->IniFileName,FileName) && !lstrcmpi(Desc->SectionName,SectionName)) {
            return(Desc);
        }
    }

    BufferSize = 0;
    SectionData = NULL;

    //
    // Read the entire section. We don't know how big it is
    // so keep growing the buffer until we succeed.
    //
    do {
        BufferSize += BUF_GROW;
        if(SectionData) {
            p = MyRealloc(SectionData,BufferSize*sizeof(TCHAR));
        } else {
            p = MyMalloc(BufferSize*sizeof(TCHAR));
        }
        if(p) {
            SectionData = p;
        } else {
            if(SectionData) {
                MyFree(SectionData);
            }
            return(NULL);
        }

        //
        // Attempt to get the entire section.
        //
        d = GetPrivateProfileSection(SectionName,SectionData,BufferSize,FileName);

    } while(d == (BufferSize-2));

    if(Desc = MyMalloc(sizeof(INIFILESECTION))) {
        if(Desc->IniFileName = DuplicateString(FileName)) {
            if(Desc->SectionName = DuplicateString(SectionName)) {
                Desc->SectionData = SectionData;
                Desc->BufferSize = BufferSize;
                Desc->BufferUsed = d + 1;

                Desc->Next = SectionList->Sections;
                SectionList->Sections = Desc;
            } else {
                MyFree(SectionData);
                MyFree(Desc->IniFileName);
                MyFree(Desc);
                Desc = NULL;
            }
        } else {
            MyFree(SectionData);
            MyFree(Desc);
            Desc = NULL;
        }
    } else {
        MyFree(SectionData);
    }

    return(Desc);
}


PTSTR
pSetupFindLineInSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,      OPTIONAL
    IN PCTSTR          RightHandSide OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PTSTR p,q,r;
    BOOL b1,b2;

    if(!KeyName && !RightHandSide) {
        return(NULL);
    }

    for(p=Section->SectionData; *p; p+=lstrlen(p)+1) {

        //
        // Locate key separator if present.
        //
        q = _tcschr(p,TEXT('='));

        //
        // If we need to match by key, attempt that here.
        // If the line has no key then it can't match.
        //
        if(KeyName) {
            if(q) {
                *q = 0;
                b1 = (lstrcmpi(KeyName,p) == 0);
                *q = TEXT('=');
            } else {
                b1 = FALSE;
            }
        } else {
            b1 = TRUE;
        }

        //
        // If we need to match by right hand side, attempt
        // that here.
        //
        if(RightHandSide) {
            //
            // If we have a key, then the right hand side is everything
            // after. If we have no key, then the right hand side is
            // the entire line.
            //
            if(q) {
                r = q + 1;
            } else {
                r = p;
            }
            b2 = (lstrcmpi(r,RightHandSide) == 0);
        } else {
            b2 = TRUE;
        }

        if(b1 && b2) {
            //
            // Return pointer to beginning of line.
            //
            return(p);
        }
    }

    return(NULL);
}


BOOL
pSetupReplaceOrAddLineInSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide,   OPTIONAL
    IN BOOL            MatchRHS
    )
{
    PTSTR LineInBuffer,NextLine;
    int CurrentCharsInBuffer;
    int ExistingLineLength,NewLineLength,BufferUsedDelta;
    PVOID p;

    //
    // Locate the line.
    //
    LineInBuffer = pSetupFindLineInSection(
                        Section,
                        KeyName,
                        MatchRHS ? RightHandSide : NULL
                        );

    if(LineInBuffer) {

        //
        // Line is in the section. Replace.
        //

        CurrentCharsInBuffer = Section->BufferUsed;

        ExistingLineLength = lstrlen(LineInBuffer)+1;

        NewLineLength = (KeyName ? (lstrlen(KeyName) + 1) : 0)         // key=
                      + (RightHandSide ? lstrlen(RightHandSide) : 0)   // RHS
                      + 1;                                             // terminating nul

        //
        // Empty lines not allowed but not error either.
        //
        if(NewLineLength == 1) {
            return(TRUE);
        }

        //
        // Figure out whether we need to grow the buffer.
        //
        BufferUsedDelta = NewLineLength - ExistingLineLength;
        if((BufferUsedDelta > 0) && ((Section->BufferSize - Section->BufferUsed) < BufferUsedDelta)) {

            p = MyRealloc(
                    Section->SectionData,
                    (Section->BufferUsed + BufferUsedDelta)*sizeof(TCHAR)
                    );

            if(p) {
                (PUCHAR)LineInBuffer += (PUCHAR)p - (PUCHAR)Section->SectionData;

                Section->SectionData = p;
                Section->BufferSize = Section->BufferUsed + BufferUsedDelta;
            } else {
                return(FALSE);
            }
        }

        NextLine = LineInBuffer + lstrlen(LineInBuffer) + 1;
        Section->BufferUsed += BufferUsedDelta;

        MoveMemory(

            //
            // Leave exactly enough space for the new line. Since the new line
            // will start at the same place the existing line is at now, the
            // target for the move is simply the first char past what will be
            // copied in later as the new line.
            //
            LineInBuffer + NewLineLength,

            //
            // The rest of the buffer past the line as it exists now must be
            // preserved. Thus the source for the move is the first char of
            // the next line as it is now.
            //
            NextLine,

            //
            // Subtract out the chars in the line as it exists now, since we're
            // going to overwrite it and are making room for the line in its
            // new form. Also subtract out the chars in the buffer that are
            // before the start of the line we're operating on.
            //
            ((CurrentCharsInBuffer - ExistingLineLength) - (LineInBuffer - Section->SectionData))*sizeof(TCHAR)

            );

        if(KeyName) {
            lstrcpy(LineInBuffer,KeyName);
            lstrcat(LineInBuffer,TEXT("="));
        }
        if(RightHandSide) {
            if(KeyName) {
                lstrcat(LineInBuffer,RightHandSide);
            } else {
                lstrcpy(LineInBuffer,RightHandSide);
            }
        }

        return(TRUE);

    } else {
        //
        // Line is not already in the section. Add it to the end.
        //
        return(pSetupAppendLineToSection(Section,KeyName,RightHandSide));
    }
}


BOOL
pSetupAppendLineToSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide    OPTIONAL
    )
{
    int LineLength;
    PVOID p;
    int EndOffset;

    LineLength = (KeyName ? (lstrlen(KeyName) + 1) : 0)         // Key=
               + (RightHandSide ? lstrlen(RightHandSide) : 0)   // RHS
               + 1;                                             // terminating nul

    //
    // Empty lines not allowed but not error either.
    //
    if(LineLength == 1) {
        return(TRUE);
    }

    if((Section->BufferSize - Section->BufferUsed) < LineLength) {

        p = MyRealloc(
                Section->SectionData,
                (Section->BufferUsed + LineLength) * sizeof(WCHAR)
                );

        if(p) {
            Section->SectionData = p;
            Section->BufferSize = Section->BufferUsed + LineLength;
        } else {
            return(FALSE);
        }
    }

    //
    // Put new text at end of section, remembering that the section
    // is termianted with an extra nul character.
    //
    if(KeyName) {
        lstrcpy(Section->SectionData + Section->BufferUsed - 1,KeyName);
        lstrcat(Section->SectionData + Section->BufferUsed - 1,TEXT("="));
    }
    if(RightHandSide) {
        if(KeyName) {
            lstrcat(Section->SectionData + Section->BufferUsed - 1,RightHandSide);
        } else {
            lstrcpy(Section->SectionData + Section->BufferUsed - 1,RightHandSide);
        }
    }

    Section->BufferUsed += LineLength;
    Section->SectionData[Section->BufferUsed-1] = 0;

    return(TRUE);
}


BOOL
pSetupDeleteLineFromSection(
    IN PINIFILESECTION Section,
    IN PCTSTR          KeyName,         OPTIONAL
    IN PCTSTR          RightHandSide    OPTIONAL
    )
{
    int LineLength;
    PTSTR Line;

    if(!KeyName && !RightHandSide) {
        return(TRUE);
    }

    //
    // Locate the line.
    //
    if(Line = pSetupFindLineInSection(Section,KeyName,RightHandSide)) {

        LineLength = lstrlen(Line) + 1;

        MoveMemory(
            Line,
            Line + LineLength,
            ((Section->SectionData + Section->BufferUsed) - (Line + LineLength))*sizeof(TCHAR)
            );

        Section->BufferUsed -= LineLength;
    }

    return(TRUE);
}


DWORD
pSetupUnloadIniFileSections(
    IN PINISECTIONCACHE SectionList,
    IN BOOL             WriteToFile
    )
{
    DWORD d;
    BOOL b;
    PINIFILESECTION Section,Temp;

    d = NO_ERROR;
    for(Section=SectionList->Sections; Section; Section=Temp) {

        Temp = Section->Next;

        if(WriteToFile) {

            //
            // Delete the existing section first and then recreate it.
            //
            b = WritePrivateProfileString(
                    Section->SectionName,
                    NULL,
                    NULL,
                    Section->IniFileName
                    );

            if(b) {
                b = WritePrivateProfileSection(
                        Section->SectionName,
                        Section->SectionData,
                        Section->IniFileName
                        );
            }

            if(!b && (d == NO_ERROR)) {
                d = GetLastError();
                //
                // Allow invalid param because sometime we have problems
                // when ini files are mapped into the registry.
                //
                if(d == ERROR_INVALID_PARAMETER) {
                    d = NO_ERROR;
                }
            }
        }

        MyFree(Section->SectionData);
        MyFree(Section->SectionName);
        MyFree(Section->IniFileName);
        MyFree(Section);
    }

    return(d);
}


DWORD
pSetupValidateDevRegProp(
    IN  ULONG   CmPropertyCode,
    IN  DWORD   ValueType,
    IN  PCVOID  Data,
    IN  DWORD   DataSize,
    OUT PVOID  *ConvertedBuffer,
    OUT PDWORD  ConvertedBufferSize
    )
/*++

Routine Description:

    This routine validates the data buffer passed in with respect to the specified
    device registry property code.  If the code is not of the correct form, but can
    be converted (e.g., REG_EXPAND_SZ -> REG_SZ), then the conversion is done and placed
    into a new buffer, that is returned to the caller.

Arguments:

    CmPropertyCode - Specifies the CM_DRP code indentifying the device registry property
        with which this data buffer is associated.

    ValueType - Specifies the registry data type for the supplied buffer.

    Data - Supplies the address of the data buffer.

    DataSize - Supplies the size, in bytes, of the data buffer.

    ConvertedBuffer - Supplies the address of a variable that receives a newly-allocated
        buffer containing a converted form of the supplied data.  If the data needs no
        conversion, this parameter will be set to NULL on return.

    ConvertedBufferSize - Supplies the address of a variable that receives the size, in
        bytes, of the converted buffer, or 0 if no conversion was required.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is an ERROR_* code.

--*/
{
    //
    // Initialize ConvertedBuffer output params to indicate that no conversion was necessary.
    //
    *ConvertedBuffer = NULL;
    *ConvertedBufferSize = 0;

    //
    // Group all properties expecting the same data type together.
    //
    switch(CmPropertyCode) {
        //
        // REG_SZ properties. No other data type is supported.
        //
        case CM_DRP_DEVICEDESC :
        case CM_DRP_SERVICE :
        case CM_DRP_CLASS :
        case CM_DRP_CLASSGUID :
        case CM_DRP_DRIVER :
        case CM_DRP_MFG :
        case CM_DRP_FRIENDLYNAME :
        case CM_DRP_LOCATION_INFORMATION :
        case CM_DRP_SECURITY_SDS :
        case CM_DRP_UI_NUMBER_DESC_FORMAT:

            if(ValueType != REG_SZ) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // REG_MULTI_SZ properties.  Allow REG_SZ as well, by simply double-terminating
        // the string (i.e., make it a REG_MULTI_SZ with only one string).
        //
        case CM_DRP_HARDWAREID :
        case CM_DRP_COMPATIBLEIDS :
        case CM_DRP_UPPERFILTERS:
        case CM_DRP_LOWERFILTERS:

            if(ValueType == REG_SZ) {

                if(*ConvertedBuffer = MyMalloc(*ConvertedBufferSize = DataSize + sizeof(TCHAR))) {
                    CopyMemory(*ConvertedBuffer, Data, DataSize);
                    *((PTSTR)((PBYTE)(*ConvertedBuffer) + DataSize)) = TEXT('\0');
                } else {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

            } else if(ValueType != REG_MULTI_SZ) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // REG_DWORD properties.  Also allow REG_BINARY, as long as the size is right.
        //
        case CM_DRP_CONFIGFLAGS :
        case CM_DRP_CAPABILITIES :
        case CM_DRP_UI_NUMBER :
        case CM_DRP_DEVTYPE :
        case CM_DRP_EXCLUSIVE :
        case CM_DRP_CHARACTERISTICS :
        case CM_DRP_ADDRESS:

            if(((ValueType != REG_DWORD) && (ValueType != REG_BINARY)) || (DataSize != sizeof(DWORD))) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // No other properties are supported.  Save the trouble of calling a CM API and
        // return failure now.
        //
        default :

            return ERROR_INVALID_REG_PROPERTY;
    }

    return NO_ERROR;
}

DWORD
pSetupValidateClassRegProp(
    IN  ULONG   CmPropertyCode,
    IN  DWORD   ValueType,
    IN  PCVOID  Data,
    IN  DWORD   DataSize,
    OUT PVOID  *ConvertedBuffer,
    OUT PDWORD  ConvertedBufferSize
    )
/*++

Routine Description:

    This routine validates the data buffer passed in with respect to the specified
    class registry property code.

Arguments:

    CmPropertyCode - Specifies the CM_CRP code indentifying the device registry property
        with which this data buffer is associated.

    ValueType - Specifies the registry data type for the supplied buffer.

    Data - Supplies the address of the data buffer.

    DataSize - Supplies the size, in bytes, of the data buffer.

    ConvertedBuffer - Supplies the address of a variable that receives a newly-allocated
        buffer containing a converted form of the supplied data.  If the data needs no
        conversion, this parameter will be set to NULL on return.

    ConvertedBufferSize - Supplies the address of a variable that receives the size, in
        bytes, of the converted buffer, or 0 if no conversion was required.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is an ERROR_* code.

--*/
{
    //
    // Initialize ConvertedBuffer output params to indicate that no conversion was necessary.
    //
    *ConvertedBuffer = NULL;
    *ConvertedBufferSize = 0;

    //
    // Group all properties expecting the same data type together.
    //
    switch(CmPropertyCode) {
        //
        // REG_SZ properties. No other data type is supported.
        //
        case CM_CRP_SECURITY_SDS :

            if(ValueType != REG_SZ) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // REG_DWORD properties.  Also allow REG_BINARY, as long as the size is right.
        //
        case CM_CRP_DEVTYPE :
        case CM_CRP_EXCLUSIVE :
        case CM_CRP_CHARACTERISTICS :

            if(((ValueType != REG_DWORD) && (ValueType != REG_BINARY)) || (DataSize != sizeof(DWORD))) {
                return ERROR_INVALID_REG_PROPERTY;
            }

            break;

        //
        // No other properties are supported.  Save the trouble of calling a CM API and
        // return failure now.
        //
        default :

            return ERROR_INVALID_REG_PROPERTY;
    }

    return NO_ERROR;
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupInstallServicesFromInfSectionA(
    IN HINF   InfHandle,
    IN PCSTR  SectionName,
    IN DWORD  Flags
    )
{
    PCWSTR UnicodeSectionName;
    BOOL b;
    DWORD d;

    if((d = CaptureAndConvertAnsiArg(SectionName, &UnicodeSectionName)) == NO_ERROR) {

        b = SetupInstallServicesFromInfSectionExW(InfHandle,
                                                  UnicodeSectionName,
                                                  Flags,
                                                  INVALID_HANDLE_VALUE,
                                                  NULL,
                                                  NULL,
                                                  NULL
                                                 );

        d = GetLastError();

        MyFree(UnicodeSectionName);

    } else {
        b = FALSE;
    }

    SetLastError(d);
    return b;
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupInstallServicesFromInfSectionW(
    IN HINF   InfHandle,
    IN PCWSTR SectionName,
    IN DWORD  Flags
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

BOOL
WINAPI
SetupInstallServicesFromInfSection(
    IN HINF   InfHandle,
    IN PCTSTR SectionName,
    IN DWORD  Flags
    )
/*++

Routine Description:

    This API performs service installation/deletion operations specified in a service
    install section.  Refer to devinstd.c!InstallNtService() for details on the format
    of this section.

Arguments:

    InfHandle - Supplies the handle of the INF containing the service install section

    SectionName - Supplies the name of the service install section to run.

    Flags - Supplies flags controlling the installation.  May be a combination of the
        following values:

        SPSVCINST_TAGTOFRONT - For every kernel or filesystem driver installed (that has
            an associated LoadOrderGroup), always move this service's tag to the front
            of the ordering list.

        SPSVCINST_DELETEEVENTLOGENTRY - For every service specified in a DelService entry,
            delete the associated event log entry (if there is one).

        SPSVCINST_NOCLOBBER_DISPLAYNAME - If this flag is specified, then we will
            not overwrite the service's display name, if it already exists.

        SPSVCINST_NOCLOBBER_STARTTYPE - If this flag is specified, then we will
            not overwrite the service's start type if the service already exists.

        SPSVCINST_NOCLOBBER_ERRORCONTROL - If this flag is specified, then we
            will not overwrite the service's error control value if the service
            already exists.

        SPSVCINST_NOCLOBBER_LOADORDERGROUP - If this flag is specified, then we
            will not overwrite the service's load order group if it already
            exists.

        SPSVCINST_NOCLOBBER_DEPENDENCIES - If this flag is specified, then we
            will not overwrite the service's dependencies list if it already
            exists.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    return SetupInstallServicesFromInfSectionEx(InfHandle,
                                                SectionName,
                                                Flags,
                                                INVALID_HANDLE_VALUE,
                                                NULL,
                                                NULL,
                                                NULL
                                               );
}


#ifdef UNICODE
//
// ANSI version
//
BOOL
WINAPI
SetupInstallServicesFromInfSectionExA(
    IN HINF             InfHandle,
    IN PCSTR            SectionName,
    IN DWORD            Flags,
    IN HDEVINFO         DeviceInfoSet,  OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN PVOID            Reserved1,
    IN PVOID            Reserved2
    )
{
    PCWSTR UnicodeSectionName;
    BOOL b;
    DWORD d;

    if((d = CaptureAndConvertAnsiArg(SectionName, &UnicodeSectionName)) == NO_ERROR) {

        b = SetupInstallServicesFromInfSectionExW(InfHandle,
                                                  UnicodeSectionName,
                                                  Flags,
                                                  DeviceInfoSet,
                                                  DeviceInfoData,
                                                  Reserved1,
                                                  Reserved2
                                                 );

        d = GetLastError();

        MyFree(UnicodeSectionName);

    } else {
        b = FALSE;
    }

    SetLastError(d);
    return b;
}
#else
//
// Unicode stub
//
BOOL
WINAPI
SetupInstallServicesFromInfSectionExW(
    IN HINF             InfHandle,
    IN PCWSTR           SectionName,
    IN DWORD            Flags,
    IN HDEVINFO         DeviceInfoSet,  OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN PVOID            Reserved1,
    IN PVOID            Reserved2
    )
{
    UNREFERENCED_PARAMETER(InfHandle);
    UNREFERENCED_PARAMETER(SectionName);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(DeviceInfoSet);
    UNREFERENCED_PARAMETER(DeviceInfoData);
    UNREFERENCED_PARAMETER(Reserved1);
    UNREFERENCED_PARAMETER(Reserved2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif

BOOL
WINAPI
SetupInstallServicesFromInfSectionEx(
    IN HINF             InfHandle,
    IN PCTSTR           SectionName,
    IN DWORD            Flags,
    IN HDEVINFO         DeviceInfoSet,  OPTIONAL
    IN PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN PVOID            Reserved1,
    IN PVOID            Reserved2
    )
/*++

Routine Description:

    This API performs service installation/deletion operations specified in a service
    install section.  Refer to devinstd.c!InstallNtService() for details on the format
    of this section.

Arguments:

    InfHandle - Supplies the handle of the INF containing the service install section

    SectionName - Supplies the name of the service install section to run.

    Flags - Supplies flags controlling the installation.  May be a combination of the
        following values:

        SPSVCINST_TAGTOFRONT - For every kernel or filesystem driver installed (that has
            an associated LoadOrderGroup), always move this service's tag to the front
            of the ordering list.

        SPSVCINST_ASSOCSERVICE - This flag may only be specified if a device information
            set and a device information element are specified.  If set, this flag
            specifies that the service being installed is the owning service (i.e.,
            function driver) for this device instance.  If the service install section
            contains more than AddService entry, then this flag is ignored (only 1
            service can be the function driver for a device instance).

        SPSVCINST_DELETEEVENTLOGENTRY - For every service specified in a DelService entry,
            delete the associated event log entry (if there is one).

        SPSVCINST_NOCLOBBER_DISPLAYNAME - If this flag is specified, then we will
            not overwrite the service's display name, if it already exists.

        SPSVCINST_NOCLOBBER_STARTTYPE - If this flag is specified, then we will
            not overwrite the service's start type if the service already exists.

        SPSVCINST_NOCLOBBER_ERRORCONTROL - If this flag is specified, then we
            will not overwrite the service's error control value if the service
            already exists.

        SPSVCINST_NOCLOBBER_LOADORDERGROUP - If this flag is specified, then we
            will not overwrite the service's load order group if it already
            exists.

        SPSVCINST_NOCLOBBER_DEPENDENCIES - If this flag is specified, then we
            will not overwrite the service's dependencies list if it already
            exists.

    DeviceInfoSet - Optionally, supplies a handle to the device information set containing
        an element that is to be associated with the service being installed.  If this
        parameter is not specified, then DeviceInfoData is ignored.

    DeviceInfoData - Optionally, specifies the device information element that is to be
        associated with the service being installed.  If DeviceInfoSet is specified, then
        this parameter must be specified.

    Reserved1, Reserved2 - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    INFCONTEXT InfContext;
    DWORD d;
    BOOL DontCare;
    BOOL NeedReboot = FALSE;

    //
    // Validate the flags passed in.
    //
    if(Flags & SPSVCINST_ILLEGAL_FLAGS) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    //
    // Make sure that a device information set and element were specified if the
    // SPSVCINST_ASSOCSERVICE flag is set.
    //
    if(Flags & SPSVCINST_ASSOCSERVICE) {

        if(!DeviceInfoSet ||
           (DeviceInfoSet == INVALID_HANDLE_VALUE) ||
           !DeviceInfoData) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    //
    // Make sure the caller didn't pass us anything in the Reserved parameters.
    //
    if(Reserved1 || Reserved2) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Make sure we were given a section name.
    //
    if(!SectionName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Lock down the device information set for the duration of this call.
    //
    if(Flags & SPSVCINST_ASSOCSERVICE) {

        if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

    } else {
        pDeviceInfoSet = NULL;
    }

    d = NO_ERROR;
    DevInfoElem = NULL;

    try {

        if(pDeviceInfoSet) {

            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                d = ERROR_INVALID_PARAMETER;
                goto clean0;
            }
        }

        //
        // We don't do any validation that the section exists in the worker routine--make
        // sure that it does exist.
        //
        if(SetupFindFirstLine(InfHandle, SectionName, NULL, &InfContext)) {
            //
            // If SPSVCINST_ASSOCSERVICE is specified, then ensure that there is exactly
            // one AddService entry in this service install section.  If not, then clear
            // this flag.
            //
            if((Flags & SPSVCINST_ASSOCSERVICE) &&
               SetupFindFirstLine(InfHandle, SectionName, pszAddService, &InfContext) &&
               SetupFindNextMatchLine(&InfContext, pszAddService, &InfContext))
            {
                Flags &= ~SPSVCINST_ASSOCSERVICE;
            }

            d = InstallNtService(DevInfoElem,
                                 InfHandle,
                                 NULL,
                                 SectionName,
                                 NULL,
                                 Flags | SPSVCINST_NO_DEVINST_CHECK,
                                 &DontCare
                                );

            if ((d == NO_ERROR) && GetLastError() == ERROR_SUCCESS_REBOOT_REQUIRED) {
                NeedReboot = TRUE;
            }

        } else {
            try {
                if (InfHandle != NULL && InfHandle != INVALID_HANDLE_VALUE && LockInf((PLOADED_INF)InfHandle)) {
                    WriteLogEntry(((PLOADED_INF)InfHandle)->LogContext,
                        SETUP_LOG_ERROR,
                        MSG_LOG_NOSECTION_MIN,
                        NULL,
                        SectionName);
                    UnlockInf((PLOADED_INF)InfHandle);
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
            }
            d = ERROR_SECTION_NOT_FOUND;
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        d = ERROR_INVALID_PARAMETER;
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    if (!NeedReboot) {
        SetLastError(d);
    } else {
        MYASSERT( d == NO_ERROR );
    }
    return (d == NO_ERROR);
}


//
// Taken from Win95 sxgen.c. These are flags used when
// we are installing an inf such as when a user right-clicks
// on one and selects the 'install' action.
//
#define HOW_NEVER_REBOOT         0
#define HOW_ALWAYS_SILENT_REBOOT 1
#define HOW_ALWAYS_PROMPT_REBOOT 2
#define HOW_SILENT_REBOOT        3
#define HOW_PROMPT_REBOOT        4


#ifdef UNICODE
//
// ANSI version
//
VOID
WINAPI
InstallHinfSectionA(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCSTR     CommandLine,
    IN INT       ShowCommand
    )
#else
//
// Unicode version
//
VOID
WINAPI
InstallHinfSectionW(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCWSTR    CommandLine,
    IN INT       ShowCommand
    )
#endif
{
    UNREFERENCED_PARAMETER(Window);
    UNREFERENCED_PARAMETER(ModuleHandle);
    UNREFERENCED_PARAMETER(CommandLine);
    UNREFERENCED_PARAMETER(ShowCommand);
}

VOID
WINAPI
InstallHinfSection(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCTSTR    CommandLine,
    IN INT       ShowCommand
    )

/*++

Routine Description:

    This is the entry point that performs the INSTALL action when
    a user right-clicks an inf file. It is called by the shell via rundll32.

    The command line is expected to be of the following form:

        <section name> <flags> <file name>

    The section is expected to be a general format install section, and
    may also have an include= line and a needs= line. Infs listed on the
    include= line are append-loaded to the inf on the command line prior to
    any installation. Sections on the needs= line are installed after the
    section listed on the command line.

    After the specified section has been installed, a section of the form:

        [<section name>.Services]

    is used in a call to SetupInstallServicesFromInfSection.

Arguments:

    Flags - supplies flags for operation.

        1 - reboot the machine in all cases
        2 - ask the user if he wants to reboot
        3 - reboot the machine without asking the user, if we think it is necessary
        4 - if we think reboot is necessary, ask the user if he wants to reboot

        0x80 - set the default file source path for file installation to
               the path where the inf is located.  (NOTE: this is hardly ever
               necessary for the Setup APIs, since we intelligently determine what
               the source path should be.  The only case where this would still be
               useful is if there were a directory that contained INFs that was in
               our INF search path list, but that also contained the files to be
               copied by this INF install action.  In that case, this flag would
               still need to be set, or we would look for the files in the location
               from which the OS was installed.

Return Value:

    None.

--*/

{
    TCHAR SourcePathBuffer[MAX_PATH];
    PTSTR SourcePath;
    TCHAR szCmd[MAX_PATH];
    PTCHAR p;
    PTCHAR szHow;
    PTSTR szInfFile, szSectionName;
    INT   iHow, NeedRebootFlags;
    HINF  InfHandle;
    TCHAR InfSearchPath[MAX_PATH];
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    BOOL b, Error;
    TCHAR ActualSection[MAX_SECT_NAME_LEN];
    DWORD ActualSectionLength;
    DWORD Win32ErrorCode;
    INFCONTEXT InfContext;
    BOOL DontCare;
    DWORD RequiredSize;
    DWORD slot_section = 0;
    BOOL NoProgressUI;

    UNREFERENCED_PARAMETER(ModuleHandle);
    UNREFERENCED_PARAMETER(ShowCommand);

    //
    // Initialize variables that will later contain resource requiring clean-up.
    //
    InfHandle = INVALID_HANDLE_VALUE;
    FileQueue = INVALID_HANDLE_VALUE;
    QueueContext = NULL;

    Error = TRUE;   // assume failure.

    try {
        //
        // Take a copy of the command line then get pointers to the fields.
        //
        lstrcpyn(szCmd, CommandLine, SIZECHARS(szCmd));

        szSectionName = szCmd;
        szHow = _tcschr(szSectionName, TEXT(' '));
        if(!szHow) {
            goto c0;
        }
        *szHow++ = TEXT('\0');
        szInfFile = _tcschr(szHow, TEXT(' '));
        if(!szInfFile) {
            goto c0;
        }
        *szInfFile++ = TEXT('\0');

        iHow = _tcstol(szHow, NULL, 10);

        //
        // Get the full path to the INF, so that the path may be used as a
        // first-pass attempt at locating any associated INFs.
        //
        RequiredSize = GetFullPathName(szInfFile,
                                       SIZECHARS(InfSearchPath),
                                       InfSearchPath,
                                       &p
                                      );

        if(!RequiredSize || (RequiredSize >= SIZECHARS(InfSearchPath))) {
            //
            // If we start failing because MAX_PATH isn't big enough anymore, we
            // wanna know about it!
            //
            MYASSERT(RequiredSize < SIZECHARS(InfSearchPath));
            goto c0;
        }

        //
        // If flag is set (and INF filename includes a path), set up so DIRID_SRCPATH is
        // path where INF is located (i.e., override our default SourcePath determination).
        //
        if((iHow & 0x80) && (MyGetFileTitle(szInfFile) != szInfFile)) {
            SourcePath = lstrcpyn(SourcePathBuffer, InfSearchPath, (int)(p - InfSearchPath) + 1);
        } else {
            SourcePath = NULL;
        }

        iHow &= 0x7f;

        //
        // If we're non-interactive, then we don't want to allow any possibility
        // of a reboot prompt happening.
        //
        if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {

            switch(iHow) {

                case HOW_NEVER_REBOOT:
                case HOW_ALWAYS_SILENT_REBOOT:
                case HOW_SILENT_REBOOT:
                    //
                    // These cases are OK--they're all silent.
                    //
                    break;

                default:
                    //
                    // Anything else is disallowed in non-interactive mode.
                    //
                    goto c0;
            }
        }

        //
        // Load the inf file that was passed on the command line.
        //
        InfHandle = SetupOpenInfFile(szInfFile, NULL, INF_STYLE_WIN4, NULL);
        if(InfHandle == INVALID_HANDLE_VALUE) {
            goto c0;
        }

        //
        // See if there is an nt-specific section
        //
        SetupDiGetActualSectionToInstall(InfHandle,
                                         szSectionName,
                                         ActualSection,
                                         SIZECHARS(ActualSection),
                                         &ActualSectionLength,
                                         NULL
                                        );

        //
        // Check to see if the install section has a "Reboot" line.  If so,
        // then adjust our behavior accordingly.
        //
        if(SetupFindFirstLine(InfHandle, ActualSection, pszReboot, &InfContext)) {

            if(iHow == HOW_SILENT_REBOOT) {
                //
                // We were supposed to only do a silent reboot if necessary.
                // Change this to _always_ do a silent reboot.
                //
                iHow = HOW_ALWAYS_SILENT_REBOOT;

            } else if(iHow != HOW_ALWAYS_SILENT_REBOOT) {

                if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
                    //
                    // In the non-interactive case, we have a problem.  The
                    // caller said to never reboot, but the INF wants us to ask
                    // the user.  We obviously cannot ask the user.
                    //
                    // In this case, we assert (we should never be running INFs
                    // non-interactively that require a reboot unless we've
                    // specified one of the silent reboot flags).  We then
                    // ignore the reboot flag, since the caller obviously
                    // doesn't want it/isn't prepared to deal with it.
                    //
                    MYASSERT(0);

                } else {
                    //
                    // In the interactive case, we want to force the (non-
                    // silent) reboot prompt (i.e., the INF flag overrides the
                    // caller).
                    //
                    iHow = HOW_ALWAYS_PROMPT_REBOOT;
                }
            }
        }

        //
        // Assume there is only one layout file and load it.
        //
        SetupOpenAppendInfFile(NULL, InfHandle, NULL);

        //
        // Append-load any included INFs specified in an "include=" line in our
        // install section.
        //
        AppendLoadIncludedInfs(InfHandle, InfSearchPath, ActualSection, TRUE);

        //
        // Create a setup file queue and initialize the default queue callback.
        //
        FileQueue = SetupOpenFileQueue();
        if(FileQueue == INVALID_HANDLE_VALUE) {
            goto c1;
        }

        //
        // Replace the file queue's log context with the Inf's
        //
        InheritLogContext(((PLOADED_INF) InfHandle)->LogContext,
            &((PSP_FILE_QUEUE) FileQueue)->LogContext);

        NoProgressUI = (GuiSetupInProgress || (GlobalSetupFlags & PSPGF_NONINTERACTIVE));

        QueueContext = SetupInitDefaultQueueCallbackEx(
                           Window,
                           (NoProgressUI ? INVALID_HANDLE_VALUE : NULL),
                           0,
                           0,
                           0
                          );

        if(!QueueContext) {
            goto c2;
        }

        if (slot_section == 0) {
            slot_section = AllocLogInfoSlot(((PSP_FILE_QUEUE) FileQueue)->LogContext,FALSE);
        }
        WriteLogEntry(((PSP_FILE_QUEUE) FileQueue)->LogContext,
            slot_section,
            MSG_LOG_INSTALLING_SECTION_FROM,
            NULL,
            ActualSection,
            szInfFile);

        b = (NO_ERROR == InstallFromInfSectionAndNeededSections(NULL,
                                                                InfHandle,
                                                                ActualSection,
                                                                SPINST_FILES,
                                                                NULL,
                                                                SourcePath,
                                                                SP_COPY_NEWER | SP_COPY_LANGUAGEAWARE,
                                                                NULL,
                                                                NULL,
                                                                INVALID_HANDLE_VALUE,
                                                                NULL,
                                                                FileQueue
                                                               ));

        //
        // Commit file queue.
        //
        if(!SetupCommitFileQueue(Window, FileQueue, SetupDefaultQueueCallback, QueueContext)) {
            goto c3;
        }

        //
        // Perform non-file operations for the section passed on the cmd line.
        //
        b = (NO_ERROR == InstallFromInfSectionAndNeededSections(Window,
                                                                InfHandle,
                                                                ActualSection,
                                                                SPINST_ALL ^ SPINST_FILES,
                                                                NULL,
                                                                NULL,
                                                                0,
                                                                NULL,
                                                                NULL,
                                                                INVALID_HANDLE_VALUE,
                                                                NULL,
                                                                NULL
                                                               ));

        //
        // Now run the corresponding ".Services" section (if there is one), and
        // then finish up the install.
        //
        CopyMemory(&(ActualSection[ActualSectionLength - 1]),
                   pszServicesSectionSuffix,
                   sizeof(pszServicesSectionSuffix)
                  );

        //
        // We don't do any validation that the section exists in the worker
        // routine--make sure that it does exist.
        //
        if(SetupFindFirstLine(InfHandle, ActualSection, NULL, &InfContext)) {

            Win32ErrorCode = InstallNtService(NULL,
                                              InfHandle,
                                              InfSearchPath,
                                              ActualSection,
                                              NULL,
                                              SPSVCINST_NO_DEVINST_CHECK,
                                              &DontCare
                                             );

            if(Win32ErrorCode != NO_ERROR) {
                SetLastError(Win32ErrorCode);
                goto c3;
            }

        }

        if(NO_ERROR != (Win32ErrorCode = InstallStopEx(TRUE, 0, ((PSP_FILE_QUEUE)FileQueue)->LogContext))) {
            SetLastError(Win32ErrorCode);
            goto c3;
        }

        //
        // Refresh the desktop.
        //
        SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_FLUSHNOWAIT,0,0);

        switch(iHow) {

        case HOW_NEVER_REBOOT:
            break;

        case HOW_ALWAYS_PROMPT_REBOOT:
            RestartDialog(Window, NULL, EWX_REBOOT);
            break;

        case HOW_PROMPT_REBOOT:
            SetupPromptReboot(FileQueue, Window, FALSE);
            break;

        case HOW_SILENT_REBOOT:
            if(!(NeedRebootFlags = SetupPromptReboot(FileQueue, Window, TRUE))) {
                break;
            } else if(NeedRebootFlags == -1) {
                //
                // An error occurred--this should never happen.
                //
                goto c3;
            }
            //
            // Let fall through to same code that handles 'always silent reboot'
            // case.
            //

        case HOW_ALWAYS_SILENT_REBOOT:
            //
            // BUGBUG (lonnym): how should we handle the case where the user
            // doesn't have reboot privilege?
            //
            if(EnablePrivilege(SE_SHUTDOWN_NAME, TRUE)) {
                ExitWindowsEx(EWX_REBOOT, 0);
            }
            break;
        }

        //
        // If we get to here, then this routine has been successful.
        //
        Error = FALSE;

c3:
        if(Error && (GetLastError() == ERROR_CANCELLED)) {
            //
            // If the error was because the user cancelled, then we don't want
            // to consider that as an error (i.e., we don't want to give an
            // error popup later).
            //
            Error = FALSE;
        }

        SetupTermDefaultQueueCallback(QueueContext);
        QueueContext = NULL;
c2:
        if (slot_section) {
            ReleaseLogInfoSlot(((PSP_FILE_QUEUE) FileQueue)->LogContext,slot_section);
            slot_section = 0;
        }
        SetupCloseFileQueue(FileQueue);
        FileQueue = INVALID_HANDLE_VALUE;
c1:
        SetupCloseInfFile(InfHandle);
        InfHandle = INVALID_HANDLE_VALUE;

c0:     ; // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if(QueueContext) {
            SetupTermDefaultQueueCallback(QueueContext);
        }
        if(FileQueue != INVALID_HANDLE_VALUE) {
            if (slot_section) {
                ReleaseLogInfoSlot(((PSP_FILE_QUEUE) FileQueue)->LogContext,slot_section);
                slot_section = 0;
            }
            SetupCloseFileQueue(FileQueue);
        }
        if(InfHandle != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile(InfHandle);
        }
    }

    if(Error) {
        //
        // BUGBUG (lonnym): we should be logging an error here.
        //
        if(!(GlobalSetupFlags & PSPGF_NONINTERACTIVE)) {
             //
             // Re-use 'ActualSection' buffer to hold error dialog title.
             //
             if(!LoadString(MyDllModuleHandle,
                            IDS_ERROR,
                            ActualSection,
                            SIZECHARS(ActualSection))) {
                 *ActualSection = TEXT('\0');
             }

             FormatMessageBox(MyDllModuleHandle,
                              Window,
                              MSG_INF_FAILED,
                              ActualSection,
                              MB_OK | MB_ICONSTOP
                             );
        }
    }
}


PTSTR
GetMultiSzFromInf(
    IN  HINF    InfHandle,
    IN  PCTSTR  SectionName,
    IN  PCTSTR  Key,
    OUT PBOOL   OutOfMemory
    )
/*++

Routine Description:

    This routine returns a newly-allocated buffer filled with the multi-sz list contained
    in the specified INF line.  The caller must free this buffer via MyFree().

Arguments:

    InfHandle - Supplies a handle to the INF containing the line

    SectionName - Specifies which section within the INF contains the line

    Key - Specifies the line whose fields are to be retrieved as a multi-sz list

    OutOfMemory - Supplies the address of a boolean variable that is set upon return
        to indicate whether or not a failure occurred because of an out-of-memory condition.
        (Failure for any other reason is assumed to be OK.)

Return Value:

    If successful, the return value is the address of a newly-allocated buffer containing
    the multi-sz list, otherwise, it is NULL.

--*/
{
    INFCONTEXT InfContext;
    PTSTR MultiSz;
    DWORD Size;

    //
    // Initialize out-of-memory indicator to FALSE.
    //
    *OutOfMemory = FALSE;

    if(!SetupFindFirstLine(InfHandle, SectionName, Key, &InfContext) ||
       !SetupGetMultiSzField(&InfContext, 1, NULL, 0, &Size) || (Size < 3)) {

        return NULL;
    }

    if(MultiSz = MyMalloc(Size * sizeof(TCHAR))) {
        if(SetupGetMultiSzField(&InfContext, 1, MultiSz, Size, &Size)) {
            return MultiSz;
        }
        MyFree(MultiSz);
    } else {
        *OutOfMemory = TRUE;
    }

    return NULL;
}


DWORD
InstallStopEx(
    IN BOOL DoRunOnce,
    IN DWORD Flags,
    IN PVOID Reserved OPTIONAL
    )
/*++

Routine Description:

    This routine sets up runonce/grpconv to run after a successful INF installation.

Arguments:

    DoRunOnce - If TRUE, then invoke (via WinExec) the runonce utility to perform the
        runonce actions.  If this flag is FALSE, then this routine simply sets the
        runonce registry values and returns.

        NOTE:  The return code from WinExec is not currently being checked, so the return
        value of InstallStop only reflects whether the registry values were set up
        successfully--_not_ whether 'runonce -r' was successfully run.

    Flags - Supplies flags that modify the behavior of this routine.  May be a
        combination of the following values:

        INSTALLSTOP_NO_UI       - Don't display any UI
        INSTALLSTOP_NO_GRPCONV  - Don't do GrpConv

    Reserved - Reserved for internal use--external callers must pass NULL.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is the Win32 error code
    indicating the error that was encountered.

--*/
{
    HKEY  hKey, hSetupKey;
    DWORD Error = NO_ERROR;
    LONG l;
    DWORD nValues = 0;
    PSETUP_LOG_CONTEXT lc = (PSETUP_LOG_CONTEXT)Reserved;

    //
    // If we're batching up RunOnce operations for server-side processing, then
    // return immediately without doing a thing.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        return NO_ERROR;
    }

    //
    // First, open the key "HKLM\Software\Microsoft\Windows\CurrentVersion\RunOnce"
    //
    if((l = RegOpenKeyEx(HKEY_LOCAL_MACHINE,pszPathRunOnce,0,KEY_WRITE|KEY_READ,&hKey)) != ERROR_SUCCESS) {
        return (DWORD)l;
    }

    if(!(Flags & INSTALLSTOP_NO_GRPCONV)) {
        //
        // If we need to run the runonce exe for the setup key...
        //
        MYASSERT(*pszKeySetup == TEXT('\\'));
        if(RegOpenKeyEx(hKey,
                        pszKeySetup + 1,    // skip the preceding '\'
                        0,
                        KEY_READ,
                        &hSetupKey) == ERROR_SUCCESS) {
            //
            // We don't need the key--we just needed to check its existence.
            //
            RegCloseKey(hSetupKey);

            //
            // Add the runonce value.
            //
            Error = (DWORD)RegSetValueEx(hKey,
                                         REGSTR_VAL_WRAPPER,
                                         0,
                                         REG_SZ,
                                         (PBYTE)pszRunOnceExe,
                                         sizeof(pszRunOnceExe)
                                        );
        } else {
            //
            // We're OK so far.
            //
            Error = NO_ERROR;
        }

        //
        // GroupConv is always run.
        //
        if(Flags & INSTALLSTOP_NO_UI) {
            l = RegSetValueEx(hKey,
                TEXT("GrpConv"),
                0,
                REG_SZ,
                (PBYTE)pszGrpConvNoUi,
                sizeof(pszGrpConvNoUi));
        } else {
            l = RegSetValueEx(hKey,
                TEXT("GrpConv"),
                0,
                REG_SZ,
                (PBYTE)pszGrpConv,
                sizeof(pszGrpConv));
        }
    }

    if( l != ERROR_SUCCESS ) {
        //
        // Since GrpConv is always run, consider it a more serious error than any error
        // encountered when setting 'runonce'.  (This decision is rather arbitrary, but
        // in practice, it should never make any difference.  Once we get the registry key
        // opened, there's no reason either of these calls to RegSetValueEx should fail.)
        //
        Error = (DWORD)l;
    }

    if (DoRunOnce && (GlobalSetupFlags & PSPGF_NO_RUNONCE)==0) {

        STARTUPINFO StartupInfo;
        PROCESS_INFORMATION ProcessInformation;
        BOOL started;
        TCHAR cmdline[MAX_PATH];

        //
        // we want to know how many items we'll be executing in RunOnce to estimate a timeout
        //
        // BugBug!!! (jamiehun) this is black-art, we'll allow 5 mins + 1 min per item we
        // find in RunOnce key, but we don't know if there are any other (new) RunOnce keys
        //
        l = RegQueryInfoKey(hKey,NULL,NULL,NULL,
                                    NULL,NULL,NULL,
                                    &nValues,
                                    NULL, NULL, NULL, NULL);
        if ( l != ERROR_SUCCESS ) {
            nValues = 5;
        } else {
            nValues += 5;
        }

        RegCloseKey(hKey);

        ZeroMemory(&StartupInfo,sizeof(StartupInfo));
        ZeroMemory(&ProcessInformation,sizeof(ProcessInformation));

        StartupInfo.cb = sizeof(StartupInfo);
        StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
        StartupInfo.wShowWindow = SW_SHOWNORMAL; // runonce -r ignores this anyway
        lstrcpy(cmdline,TEXT("runonce -r"));

        if (lc) {
            //
            // log this information only if we have a context, else don't waste space
            //
            WriteLogEntry(lc,
                  SETUP_LOG_INFO,
                  MSG_LOG_RUNONCE_CALL,
                  NULL,
                  nValues
                 );
        }

        started = CreateProcess(NULL,       // use application name below
                      cmdline,              // command to execute
                      NULL,                 // default process security
                      NULL,                 // default thread security
                      FALSE,                // don't inherit handles
                      0,                    // default flags
                      NULL,                 // inherit environment
                      NULL,                 // inherit current directory
                      &StartupInfo,
                      &ProcessInformation);

        if(started) {

            DWORD WaitProcStatus;
            DWORD Timeout;


            if (nValues > RUNONCE_THRESHOLD) {
                Timeout = RUNONCE_TIMEOUT * RUNONCE_THRESHOLD;
            } else if (nValues > 0) {
                Timeout = RUNONCE_TIMEOUT * nValues;
            } else {
                //
                // assume something strange - shouldn't occur
                //
                Timeout = RUNONCE_TIMEOUT * RUNONCE_THRESHOLD;
            }

            //
            // BugBug (jamiehun) 8/31/99
            // Post 5.0 we need to remove non-UNICODE
            //
#ifdef UNICODE
            {
                BOOL KeepWaiting = TRUE;

                while (KeepWaiting) {
                    WaitProcStatus = MyMsgWaitForMultipleObjectsEx(
                        1,
                        &ProcessInformation.hProcess,
                        Timeout,
                        QS_ALLINPUT,
                        MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
                    switch (WaitProcStatus) {
                    case WAIT_OBJECT_0 + 1: { // Process gui messages
                        MSG msg;

                        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }

                        // fall through ...
                    }
                    case WAIT_IO_COMPLETION:
                        break;

                    case WAIT_OBJECT_0:
                    case WAIT_TIMEOUT:
                    default:
                        KeepWaiting = FALSE;
                        break;
                    }
                }
            }
#else
            do {

                WaitProcStatus = WaitForSingleObjectEx(ProcessInformation.hProcess, Timeout , TRUE);

            } while (WaitProcStatus == WAIT_IO_COMPLETION);
#endif

            if (WaitProcStatus == WAIT_TIMEOUT) {
                //
                // We won't consider this a critical failure--the runonce task
                // will continue. We do want to log an error about this.
                //
                WriteLogEntry(lc,
                              SETUP_LOG_ERROR,
                              MSG_LOG_RUNONCE_TIMEOUT,
                              NULL
                             );
            }

            CloseHandle(ProcessInformation.hThread);
            CloseHandle(ProcessInformation.hProcess);

        } else {

            DWORD CreateProcError;

            //
            // We won't consider this a critical failure--the runonce task
            // should get picked up later by someone else (e.g., at next
            // login).  We do want to log an error about this, however.
            //
            CreateProcError = GetLastError();

            WriteLogEntry(lc,
                          SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                          MSG_LOG_RUNONCE_FAILED,
                          NULL
                         );

            WriteLogError(lc,
                          SETUP_LOG_ERROR,
                          CreateProcError
                         );
        }
    } else {

        RegCloseKey(hKey);
    }

    return Error;
}

DWORD
InstallStop(
    IN BOOL DoRunOnce
    )
/*++

Routine Description:

    This routine sets up runonce/grpconv to run after a successful INF installation.

Arguments:

    DoRunOnce - If TRUE, then invoke (via WinExec) the runonce utility to perform the
        runonce actions.  If this flag is FALSE, then this routine simply sets the
        runonce registry values and returns.

        NOTE:  The return code from WinExec is not currently being checked, so the return
        value of InstallStop only reflects whether the registry values were set up
        successfully--_not_ whether 'runonce -r' was successfully run.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is the Win32 error code
    indicating the error that was encountered.

Remarks:

    If we're in non-interactive mode, no registry modifications are made, and
    RunOnce is not invoked.

--*/
{
    return InstallStopEx( DoRunOnce, 0, NULL );
}

DWORD
LoadNtOnlyDll(
    IN  PCTSTR DllName,
    OUT HINSTANCE *Dll_Handle
    )
{
    OSVERSIONINFO OsVer;
    HINSTANCE Temp_Handle;

    if( OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) {

        //Check if the library is already loaded. In a multi-threaded scenario the InterlockedExchange
        //will take care of the race condition. This just speeds it up in other cases.

        if( !SecurityDllHandle ){


            if( Temp_Handle = LoadLibrary( DllName )){
                if( InterlockedExchangePointer( &SecurityDllHandle, Temp_Handle ) ){
                    FreeLibrary( Temp_Handle );
                }

                *Dll_Handle = Temp_Handle;
                return( TRUE );
            }
            else
                return( FALSE );

        }
        else{
            *Dll_Handle = SecurityDllHandle;
            return( TRUE );
        }
    }

    return (LD_WIN95);

}


PRUNONCE_NODE
pSetupAccessRunOnceNodeList(
    VOID
    )
/*++

Routine Description:

    This routine returns a pointer to the head of the global RunOnce node list.
    The caller may traverse the list (via the Next pointer), but does not own
    the list, and may not modify it in any way.

    To cause the list to be freed, use pSetupDestroyRunOnceNodeList.

Arguments:

    None

Return Value:

    Pointer to the first item in the list, or NULL if the list is empty.

Remarks:

    THIS ROUTINE IS NOT THREAD SAFE, AND IS FOR USE SOLELY BY THE SINGLE THREAD
    IN UMPNPMGR THAT DOES DEVICE INSTALLATIONS.

--*/
{
    return RunOnceListHead;
}


VOID
pSetupDestroyRunOnceNodeList(
    VOID
    )
/*++

Routine Description:

    This routine frees the global list of RunOnce nodes, setting it back to an
    empty list.

Arguments:

    None

Return Value:

    None

Remarks:

    THIS ROUTINE IS NOT THREAD SAFE, AND IS FOR USE SOLELY BY THE SINGLE THREAD
    IN UMPNPMGR THAT DOES DEVICE INSTALLATIONS.

--*/
{
    PRUNONCE_NODE NextNode;

    while(RunOnceListHead) {
        NextNode = RunOnceListHead->Next;
        MyFree(RunOnceListHead->DllFullPath);
        MyFree(RunOnceListHead->DllEntryPointName);
        MyFree(RunOnceListHead->DllParams);
        MyFree(RunOnceListHead);
        RunOnceListHead = NextNode;
    }
}


