/*
 *  M A P I V A L . H
 *  
 *  Macros used to validate parameters on standard MAPI object methods.
 *  Used in conjunction with routines found in MAPIU.DLL.
 *  
 *  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
 */

#ifndef _INC_VALIDATE
#define _INC_VALIDATE

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAPIUTIL_H
#include    <mapiutil.h>
#endif
#include    <stddef.h>
#include    <stdarg.h>


#define MAKE_ENUM(Method, Interface)    Interface##_##Method

typedef enum _tagMethods
{
/* IUnknown */
    MAKE_ENUM(QueryInterface, IUnknown) = 0,
    MAKE_ENUM(AddRef, IUnknown),            /* For completness */
    MAKE_ENUM(Release, IUnknown),           /* For completness */
    
/* IMAPIProps */
    MAKE_ENUM(GetLastError, IMAPIProp),
    MAKE_ENUM(SaveChanges, IMAPIProp),
    MAKE_ENUM(GetProps, IMAPIProp),
    MAKE_ENUM(GetPropList, IMAPIProp),
    MAKE_ENUM(OpenProperty, IMAPIProp),
    MAKE_ENUM(SetProps, IMAPIProp),
    MAKE_ENUM(DeleteProps, IMAPIProp),
    MAKE_ENUM(CopyTo, IMAPIProp),
    MAKE_ENUM(CopyProps, IMAPIProp),
    MAKE_ENUM(GetNamesFromIDs, IMAPIProp),
    MAKE_ENUM(GetIDsFromNames, IMAPIProp),

/* IMAPITable */
    MAKE_ENUM(GetLastError, IMAPITable),
    MAKE_ENUM(Advise, IMAPITable),
    MAKE_ENUM(Unadvise, IMAPITable),
    MAKE_ENUM(GetStatus, IMAPITable),
    MAKE_ENUM(SetColumns, IMAPITable),
    MAKE_ENUM(QueryColumns, IMAPITable),
    MAKE_ENUM(GetRowCount, IMAPITable),
    MAKE_ENUM(SeekRow, IMAPITable),
    MAKE_ENUM(SeekRowApprox, IMAPITable),
    MAKE_ENUM(QueryPosition, IMAPITable),
    MAKE_ENUM(FindRow, IMAPITable),
    MAKE_ENUM(Restrict, IMAPITable),
    MAKE_ENUM(CreateBookmark, IMAPITable),
    MAKE_ENUM(FreeBookmark, IMAPITable),
    MAKE_ENUM(SortTable, IMAPITable),
    MAKE_ENUM(QuerySortOrder, IMAPITable),
    MAKE_ENUM(QueryRows, IMAPITable),
    MAKE_ENUM(Abort, IMAPITable),
    MAKE_ENUM(ExpandRow, IMAPITable),
    MAKE_ENUM(CollapseRow, IMAPITable),
    MAKE_ENUM(WaitForCompletion, IMAPITable),
    MAKE_ENUM(GetCollapseState, IMAPITable),
    MAKE_ENUM(SetCollapseState, IMAPITable),

/* IMAPIContainer */
    MAKE_ENUM(GetContentsTable, IMAPIContainer),
    MAKE_ENUM(GetHierarchyTable, IMAPIContainer),
    MAKE_ENUM(OpenEntry, IMAPIContainer),
    MAKE_ENUM(SetSearchCriteria, IMAPIContainer),
    MAKE_ENUM(GetSearchCriteria, IMAPIContainer),

/* IABContainer */
    MAKE_ENUM(CreateEntry, IABContainer),
    MAKE_ENUM(CopyEntries, IABContainer),
    MAKE_ENUM(DeleteEntries, IABContainer),
    MAKE_ENUM(ResolveNames, IABContainer),

/* IDistList */
    MAKE_ENUM(CreateEntry, IDistList),
    MAKE_ENUM(CopyEntries, IDistList),
    MAKE_ENUM(DeleteEntries, IDistList),
    MAKE_ENUM(ResolveNames, IDistList),

/* IMAPIFolder */
    MAKE_ENUM(CreateMessage, IMAPIFolder),
    MAKE_ENUM(CopyMessages, IMAPIFolder),
    MAKE_ENUM(DeleteMessages, IMAPIFolder),
    MAKE_ENUM(CreateFolder, IMAPIFolder),
    MAKE_ENUM(CopyFolder, IMAPIFolder),
    MAKE_ENUM(DeleteFolder, IMAPIFolder),
    MAKE_ENUM(SetReadFlags, IMAPIFolder),
    MAKE_ENUM(GetMessageStatus, IMAPIFolder),
    MAKE_ENUM(SetMessageStatus, IMAPIFolder),
    MAKE_ENUM(SaveContentsSort, IMAPIFolder),
    MAKE_ENUM(EmptyFolder, IMAPIFolder),

/* IMsgStore */
    MAKE_ENUM(Advise, IMsgStore),
    MAKE_ENUM(Unadvise, IMsgStore),
    MAKE_ENUM(CompareEntryIDs, IMsgStore),
    MAKE_ENUM(OpenEntry, IMsgStore),
    MAKE_ENUM(SetReceiveFolder, IMsgStore),
    MAKE_ENUM(GetReceiveFolder, IMsgStore),
    MAKE_ENUM(GetReceiveFolderTable, IMsgStore),
    MAKE_ENUM(StoreLogoff, IMsgStore),
    MAKE_ENUM(AbortSubmit, IMsgStore),
    MAKE_ENUM(GetOutgoingQueue, IMsgStore),
    MAKE_ENUM(SetLockState, IMsgStore),
    MAKE_ENUM(FinishedMsg, IMsgStore),
    MAKE_ENUM(NotifyNewMail, IMsgStore),

/* IMessage */
    MAKE_ENUM(GetAttachmentTable, IMessage),
    MAKE_ENUM(OpenAttach, IMessage),
    MAKE_ENUM(CreateAttach, IMessage),
    MAKE_ENUM(DeleteAttach, IMessage),
    MAKE_ENUM(GetRecipientTable, IMessage),
    MAKE_ENUM(ModifyRecipients, IMessage),
    MAKE_ENUM(SubmitMessage, IMessage),
    MAKE_ENUM(SetReadFlag, IMessage),


/* IABProvider */
    MAKE_ENUM(Shutdown, IABProvider),
    MAKE_ENUM(Logon, IABProvider),

/* IABLogon */
    MAKE_ENUM(GetLastError, IABLogon),
    MAKE_ENUM(Logoff, IABLogon),
    MAKE_ENUM(OpenEntry, IABLogon),
    MAKE_ENUM(CompareEntryIDs, IABLogon),
    MAKE_ENUM(Advise, IABLogon),
    MAKE_ENUM(Unadvise, IABLogon),
    MAKE_ENUM(OpenStatusEntry, IABLogon),
    MAKE_ENUM(OpenTemplateID, IABLogon),
    MAKE_ENUM(GetOneOffTable, IABLogon),
    MAKE_ENUM(PrepareRecips, IABLogon),

/* IXPProvider */
    MAKE_ENUM(Shutdown, IXPProvider),
    MAKE_ENUM(TransportLogon, IXPProvider),

/* IXPLogon */
    MAKE_ENUM(AddressTypes, IXPLogon),
    MAKE_ENUM(RegisterOptions, IXPLogon),
    MAKE_ENUM(TransportNotify, IXPLogon),
    MAKE_ENUM(Idle, IXPLogon),
    MAKE_ENUM(TransportLogoff, IXPLogon),
    MAKE_ENUM(SubmitMessage, IXPLogon),
    MAKE_ENUM(EndMessage, IXPLogon),
    MAKE_ENUM(Poll, IXPLogon),
    MAKE_ENUM(StartMessage, IXPLogon),
    MAKE_ENUM(OpenStatusEntry, IXPLogon),
    MAKE_ENUM(ValidateState, IXPLogon),
    MAKE_ENUM(FlushQueues, IXPLogon),

/* IMSProvider */
    MAKE_ENUM(Shutdown, IMSProvider),
    MAKE_ENUM(Logon, IMSProvider),
    MAKE_ENUM(SpoolerLogon, IMSProvider),
    MAKE_ENUM(CompareStoreIDs, IMSProvider),

/* IMSLogon */
    MAKE_ENUM(GetLastError, IMSLogon),
    MAKE_ENUM(Logoff, IMSLogon),
    MAKE_ENUM(OpenEntry, IMSLogon),
    MAKE_ENUM(CompareEntryIDs, IMSLogon),
    MAKE_ENUM(Advise, IMSLogon),
    MAKE_ENUM(Unadvise, IMSLogon),
    MAKE_ENUM(OpenStatusEntry, IMSLogon),
    
/* IMAPIControl */
    MAKE_ENUM(GetLastError, IMAPIControl),
    MAKE_ENUM(Activate, IMAPIControl),
    MAKE_ENUM(GetState, IMAPIControl),
    
/* IMAPIStatus */
    MAKE_ENUM(ValidateState, IMAPIStatus),
    MAKE_ENUM(SettingsDialog, IMAPIStatus),
    MAKE_ENUM(ChangePassword, IMAPIStatus),
    MAKE_ENUM(FlushQueues, IMAPIStatus),

/* IStream */
    MAKE_ENUM(Read, IStream),
    MAKE_ENUM(Write, IStream),
    MAKE_ENUM(Seek, IStream),
    MAKE_ENUM(SetSize, IStream),
    MAKE_ENUM(CopyTo, IStream),
    MAKE_ENUM(Commit, IStream),
    MAKE_ENUM(Revert, IStream),
    MAKE_ENUM(LockRegion, IStream),
    MAKE_ENUM(UnlockRegion, IStream),
    MAKE_ENUM(Stat, IStream),
    MAKE_ENUM(Clone, IStream),

/* IMAPIAdviseSink */
    MAKE_ENUM(OnNotify, IMAPIAdviseSink),

} METHODS;


/* Macro wrappers to hide the Validate function return handling */
#if defined(_X86_) || defined( WIN16 )
#ifdef __cplusplus

/* C++ methods can't take the address of the This pointer, so we must
   use the first parameter instead */

#define ValidateParameters(eMethod, First)              \
        {   HRESULT   _hr_;                             \
            _hr_ = __CPPValidateParameters(eMethod, (LPVOID) &First);   \
            if (HR_FAILED(_hr_)) return (_hr_); }

#define UlValidateParameters(eMethod, First)                \
        {   HRESULT   _hr_;                             \
            _hr_ = __CPPValidateParameters(eMethod, &First);    \
            if (HR_FAILED(_hr_)) return (ULONG) (_hr_); }

/* Methods called by MAPI should have correct parameters
   - just assert in Debug to check */
#define CheckParameters(eMethod, First)             \
        AssertSz(HR_SUCCEEDED(__CPPValidateParameters(eMethod, &First)), "Parameter validation failed for method called by MAPI!")


#else /* __cplusplus */

/* For methods that will be called by clients 
   - validate always */
   
#define ValidateParameters(eMethod, ppThis)             \
        {   HRESULT   _hr_;                             \
            _hr_ = __ValidateParameters(eMethod, ppThis);   \
            if (HR_FAILED(_hr_)) return (_hr_); }

#define UlValidateParameters(eMethod, ppThis)               \
        {   HRESULT   _hr_;                             \
            _hr_ = __ValidateParameters(eMethod, ppThis);   \
            if (HR_FAILED(_hr_)) return (ULONG) (_hr_); }

/* Methods called by MAPI should have correct parameters
   - just assert in Debug to check */
#define CheckParameters(eMethod, ppThis)                \
        AssertSz(HR_SUCCEEDED(__ValidateParameters(eMethod, ppThis)), "Parameter validation failed for method called by MAPI!")

#endif /* __cplusplus */
#endif /* _X86_ || WIN16 */

/* Prototypes for functions used to validate complex parameters.
 */
#define FBadPropVal( lpPropVal) (FAILED(ScCountProps( 1, lpPropVal, NULL)))

#define FBadRgPropVal( lpPropVal, cValues) \
        (FAILED(ScCountProps( cValues, lpPropVal, NULL)))

#define FBadAdrList( lpAdrList) \
        (   AssertSz(   (   offsetof( ADRLIST, cEntries) \
                         == offsetof( SRowSet, cRows)) \
                     && (   offsetof( ADRLIST, aEntries) \
                         == offsetof( SRowSet, aRow)) \
                     && (   offsetof( ADRENTRY, cValues) \
                         == offsetof( SRow, cValues)) \
                     && (   offsetof( ADRENTRY, rgPropVals) \
                         == offsetof( SRow, lpProps)) \
                    , "ADRLIST doesn't match SRowSet") \
         || FBadRowSet( (LPSRowSet) lpAdrList))

STDAPI_(BOOL)
FBadRglpszW( LPWSTR FAR *lppszW,
             ULONG      cStrings);

STDAPI_(BOOL)
FBadRowSet( LPSRowSet   lpRowSet);

STDAPI_(BOOL)
FBadRglpNameID( LPMAPINAMEID FAR *  lppNameId,
                ULONG               cNames);

STDAPI_(BOOL)
FBadEntryList( LPENTRYLIST  lpEntryList);


/* BAD_STANDARD_OBJ
 *
 * This macro insures that the object is a writable object of the correct size
 * and that this method belongs to the object.
 *
 * NOTES ON USE!
 *  This depends upon using the standard method of declaring the object
 *  interface.
 *
 *  prefix is the method prefix you chose when declaring the object interface.
 *  method is the standard method name of the calling method.
 *  lpVtbl is the name of the lpVtbl element of your object.
 */
#define BAD_STANDARD_OBJ( lpObj, prefix, method, lpVtbl) \
    (   IsBadWritePtr( (lpObj), sizeof(*lpObj)) \
     || IsBadReadPtr( (void *) &(lpObj->lpVtbl->method), sizeof(LPVOID)) \
     ||( ( LPVOID) (lpObj->lpVtbl->method) != (LPVOID) (prefix##method)))


#define FBadUnknown( lpObj ) \
    (   IsBadReadPtr( (lpObj), sizeof(LPVOID) ) \
     || IsBadReadPtr( (lpObj)->lpVtbl, 3 * sizeof(LPUNKNOWN) ) \
     || IsBadCodePtr( (FARPROC)(lpObj)->lpVtbl->QueryInterface ))

/*
 * IUnknown
 */


/*
 * QueryInterface
 */
#define FBadQueryInterface( lpObj, riid, ppvObj)    \
    (   IsBadReadPtr( riid, sizeof(IID)) \
     || IsBadWritePtr( ppvObj, sizeof(LPVOID)))


/*
 * AddRef
 *  No parameter validation required.
 */
#define FBadAddRef( lpObj)  FALSE


/*
 * Release
 *  No parameter validation required.
 */
#define FBadRelease( lpObj) FALSE


/*
 * GetLastError
 */
#define FBadGetLastError( lpObj, hResult, ulFlags, lppMAPIError )\
    (IsBadWritePtr( lppMAPIError, sizeof(LPMAPIERROR)))

/*
 * IMAPIProp
 */


/*
 * SaveChanges
 *  No parameter validation required.
 */
#define FBadSaveChanges( lpObj, ulFlags)    FALSE


/*
 * GetProps
 */
#define FBadGetProps( lpObj, lpPTagA, lpcValues, lppPropArray) \
    (   (   lpPTagA \
         && (   IsBadReadPtr( lpPTagA, sizeof(ULONG)) \
             || IsBadReadPtr( lpPTagA, (UINT)(  (lpPTagA->cValues + 1) \
                                              * sizeof(ULONG))))) \
     || IsBadWritePtr( lpcValues, sizeof(ULONG)) \
     || IsBadWritePtr( lppPropArray, sizeof(LPSPropValue)))


/*
 * GetPropList
 */
#define FBadGetPropList( lpObj, lppPTagA) \
    (IsBadWritePtr( lppPTagA, sizeof(LPSPropTagArray FAR *)))


/*
 * OpenProperty
 */
#define FBadOpenProperty( lpObj, ulPropTag, lpiid, ulInterfaceOptions, ulFlags \
                        , lppUnk) \
    (   IsBadReadPtr( lpiid, sizeof(IID)) \
     || IsBadWritePtr( lppUnk, sizeof (LPUNKNOWN FAR *)))


/*
 * SetProps
 */
#define FBadSetProps( lpObj, cValues, lpPropArray, lppProblems) \
    (   FBadRgPropVal( lpPropArray, (UINT) cValues) \
     || (   lppProblems \
         && IsBadWritePtr( lppProblems, sizeof(LPSPropProblemArray))))


/*
 * DeleteProps
 */
#define FBadDeleteProps( lpObj, lpPTagA, lppProblems) \
    (   (   !lpPTagA \
         || (   IsBadReadPtr( lpPTagA, sizeof(ULONG)) \
             || IsBadReadPtr( lpPTagA, (UINT)(  (lpPTagA->cValues + 1) \
                                              * sizeof(ULONG))))) \
     || (   lppProblems \
         && IsBadWritePtr( lppProblems, sizeof(LPSPropProblemArray))))


/*
 * CopyTo
 */
#define FBadCopyTo( lpIPDAT, ciidExclude, rgiidExclude, lpExcludeProps \
                  , ulUIParam, lpProgress, lpInterface, lpDestObj \
                  , ulFlags, lppProblems) \
    (   (   ciidExclude \
         && (  IsBadReadPtr( rgiidExclude, (UINT)(ciidExclude * sizeof(IID))))) \
     || (   lpExcludeProps \
         && (   IsBadReadPtr( lpExcludeProps, sizeof(ULONG)) \
             || IsBadReadPtr( lpExcludeProps \
                            , (UINT)(  (lpExcludeProps->cValues + 1) \
                                     * sizeof(ULONG))))) \
     || (lpProgress && FBadUnknown( lpProgress )) \
     || (lpInterface && IsBadReadPtr( lpInterface, sizeof(IID))) \
     || IsBadReadPtr( lpDestObj, sizeof(LPVOID)) \
     || (   lppProblems \
         && IsBadWritePtr( lppProblems, sizeof(LPSPropProblemArray))))


/*
 * CopyProps
 */
#define FBadCopyProps( lpIPDAT, lpPropTagArray \
                     , ulUIParam, lpProgress, lpInterface, lpDestObj \
                     , ulFlags, lppProblems) \
    (   (   lpPropTagArray \
         && (   IsBadReadPtr( lpPropTagArray, sizeof(ULONG)) \
             || IsBadReadPtr( lpPropTagArray \
                            , (UINT)(  (lpPropTagArray->cValues + 1) \
                                     * sizeof(ULONG))))) \
     || (lpProgress && FBadUnknown( lpProgress )) \
     || (lpInterface && IsBadReadPtr( lpInterface, sizeof(IID))) \
     || IsBadReadPtr( lpDestObj, sizeof(LPVOID)) \
     || (   lppProblems \
         && IsBadWritePtr( lppProblems, sizeof(LPSPropProblemArray))))



/*
 * GetNamesFromIDs
 */
#define FBadGetNamesFromIDs( lpIPDAT, lppPropTags, lpPropSetGuid, ulFlags, \
                             lpcPropNames, lpppPropNames) \
    (   IsBadReadPtr( lppPropTags, sizeof(LPSPropTagArray)) \
     || ( lpPropSetGuid && IsBadReadPtr( lpPropSetGuid, sizeof(GUID))) \
     || (   *lppPropTags \
         && (   IsBadReadPtr( *lppPropTags, sizeof(ULONG)) \
             || IsBadReadPtr( *lppPropTags \
                            , (UINT)( ( ( *lppPropTags)->cValues + 1) \
                                     * sizeof(ULONG))))) \
     || IsBadWritePtr( lpcPropNames, sizeof (ULONG)) \
     || IsBadWritePtr( lpppPropNames, sizeof (LPVOID FAR *)))



/*
 * GetNamesFromIDs
 */
#define FBadGetIDsFromNames( lpIPDAT, cPropNames, lppPropNames, ulFlags \
                           , lppPropTags) \
    (   (cPropNames && FBadRglpNameID( lppPropNames, cPropNames)) \
     || IsBadWritePtr( lppPropTags, sizeof(LPULONG FAR *)))


STDAPI_(ULONG)
FBadRestriction( LPSRestriction lpres );

STDAPI_(ULONG)
FBadPropTag( ULONG ulPropTag );

STDAPI_(ULONG)
FBadRow( LPSRow lprow );

STDAPI_(ULONG)
FBadProp( LPSPropValue lpprop );

STDAPI_(ULONG)
FBadSortOrderSet( LPSSortOrderSet lpsos );

STDAPI_(ULONG)
FBadColumnSet( LPSPropTagArray lpptaCols );

/* Validation function

    The eMethod parameter tells us which internal validation to perform.
    
    The ppThis parameter tells us where the stack is, so we can access the other 
    parameters.  
    
    Becuase of this *magic* we MUST obtain the pointer to the This pointer in 
    the method function.
    
*/

#ifdef WIN16
#define BASED_STACK         __based(__segname("_STACK"))
#else
#define BASED_STACK
#endif


#ifdef WIN16
HRESULT  PASCAL
#else
HRESULT  STDAPICALLTYPE     
#endif
__CPPValidateParameters(METHODS eMethod, const LPVOID ppFirst);

#ifdef WIN16
HRESULT  PASCAL
#else
HRESULT  STDAPICALLTYPE     
#endif
__ValidateParameters(METHODS eMethod, LPVOID ppThis);

#ifdef _MAC
#define STDAPIVCALLTYPE         __cdecl
#define STDAPIV                 EXTERN_C HRESULT STDAPIVCALLTYPE
#define STDAPIV_(type)          EXTERN_C type STDAPIVCALLTYPE
#endif /* _MAC */

/* Macro wrappers for platform independent validation */

#if defined(_X86_) || defined( WIN16 )
#define ValidateParms(x)    { HRESULT _hr_ = HrValidateParameters x; if (HR_FAILED(_hr_)) return (_hr_); }
#define UlValidateParms(x)  { HRESULT _hr_ = HrValidateParameters x; if (HR_FAILED(_hr_)) return (ULONG)(_hr_); }
#define CheckParms(x)       AssertSz(HR_SUCCEEDED( HrValidateParameters x), "Parameter validation failed for method called by MAPI!")
#else
#define ValidateParms(x)    { HRESULT _hr_ = HrValidateParametersV x; if (HR_FAILED(_hr_)) return (_hr_); }
#define UlValidateParms(x)  { HRESULT _hr_ = HrValidateParametersV x; if (HR_FAILED(_hr_)) return (ULONG)(_hr_); }
#define CheckParms(x)       AssertSz(HR_SUCCEEDED( HrValidateParametersV x ), "Parameter validation failed for method called by MAPI!")
#endif

#if defined(_X86_) || defined( WIN16 )

#define ValidateParameters1( m, a1 ) 
#define ValidateParameters2( m, a1, a2 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters3( m, a1, a2, a3 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters4( m, a1, a2, a3, a4 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters5( m, a1, a2, a3, a4, a5 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters6( m, a1, a2, a3, a4, a5, a6 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters7( m, a1, a2, a3, a4, a5, a6, a7 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters8( m, a1, a2, a3, a4, a5, a6, a7, a8 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters9( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters10( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters11( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters12( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters13( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters14( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters15( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define ValidateParameters16( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ) \
            ValidateParms( ( m, (LPVOID FAR *) &a2 ) )

#define UlValidateParameters1( m, a1 ) 
#define UlValidateParameters2( m, a1, a2 )  \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters3( m, a1, a2, a3 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters4( m, a1, a2, a3, a4 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters5( m, a1, a2, a3, a4, a5 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters6( m, a1, a2, a3, a4, a5, a6 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters7( m, a1, a2, a3, a4, a5, a6, a7 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters8( m, a1, a2, a3, a4, a5, a6, a7, a8 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters9( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters10( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters11( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters12( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters13( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters14( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters15( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )
#define UlValidateParameters16( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ) \
            UlValidateParms( ( m, (LPVOID FAR *) &a2 ) )

#define CheckParameters1( m, a1 ) 
#define CheckParameters2( m, a1, a2 )   \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters3( m, a1, a2, a3)    \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters4( m, a1, a2, a3, a4 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters5( m, a1, a2, a3, a4, a5 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters6( m, a1, a2, a3, a4, a5, a6 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters7( m, a1, a2, a3, a4, a5, a6, a7 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters8( m, a1, a2, a3, a4, a5, a6, a7, a8 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters9( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters10( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters11( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters12( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters13( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters14( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters15( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )
#define CheckParameters16( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ) \
            CheckParms( ( m, (LPVOID FAR *) &a2 ) )

#else /* !_X86_  && !WIN16 */

#define ValidateParameters1( m, a1 ) \
            ValidateParms( ( m, a1 ) )
#define ValidateParameters2( m, a1, a2 )    \
            ValidateParms( ( m, a1, a2 ))
#define ValidateParameters3( m, a1, a2, a3 )    \
            ValidateParms( ( m, a1, a2, a3 ))
#define ValidateParameters4( m, a1, a2, a3, a4 ) \
            ValidateParms( ( m, a1, a2, a3, a4 ))
#define ValidateParameters5( m, a1, a2, a3, a4, a5 ) \
            ValidateParms( ( m, a1, a2, a3, a4, a5 ))
#define ValidateParameters6( m, a1, a2, a3, a4, a5, a6 ) \
            ValidateParms( ( m, a1, a2, a3, a4, a5, a6 ))
#define ValidateParameters7( m, a1, a2, a3, a4, a5, a6, a7 ) \
            ValidateParms( ( m, a1, a2, a3, a4, a5, a6, a7 ))
#define ValidateParameters8( m, a1, a2, a3, a4, a5, a6, a7, a8 ) \
            ValidateParms( ( m, a1, a2, a3, a4, a5, a6, a7, a8 ))
#define ValidateParameters9( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
            ValidateParms( ( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ))
#define ValidateParameters10( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
            ValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ))
#define ValidateParameters11( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ) \
            ValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ))
#define ValidateParameters12( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ) \
            ValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ))
#define ValidateParameters13( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
            ValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ))
#define ValidateParameters14( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ) \
            ValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ))
#define ValidateParameters15( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ) \
            ValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ))
#define ValidateParameters16( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ) \
            ValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ))

#define UlValidateParameters1( m, a1 ) \
            UlValidateParms( ( m, a1 ) )
#define UlValidateParameters2( m, a1, a2 )  \
            UlValidateParms( ( m, a1, a2 ))
#define UlValidateParameters3( m, a1, a2, a3 )  \
            UlValidateParms( ( m, a1, a2, a3 ))
#define UlValidateParameters4( m, a1, a2, a3, a4 ) \
            UlValidateParms( ( m, a1, a2, a3, a4 ))
#define UlValidateParameters5( m, a1, a2, a3, a4, a5 ) \
            UlValidateParms( ( m, a1, a2, a3, a4, a5 ))
#define UlValidateParameters6( m, a1, a2, a3, a4, a5, a6 ) \
            UlValidateParms( ( m, a1, a2, a3, a4, a5, a6 ))
#define UlValidateParameters7( m, a1, a2, a3, a4, a5, a6, a7 ) \
            UlValidateParms( ( m, a1, a2, a3, a4, a5, a6, a7 ))
#define UlValidateParameters8( m, a1, a2, a3, a4, a5, a6, a7, a8 ) \
            UlValidateParms( ( m, a1, a2, a3, a4, a5, a6, a7, a8 ))
#define UlValidateParameters9( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
            UlValidateParms( ( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ))
#define UlValidateParameters10( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
            UlValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ))
#define UlValidateParameters11( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ) \
            UlValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ))
#define UlValidateParameters12( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ) \
            UlValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ))
#define UlValidateParameters13( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
            UlValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ))
#define UlValidateParameters14( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ) \
            UlValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ))
#define UlValidateParameters15( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ) \
            UlValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ))
#define UlValidateParameters16( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ) \
            UlValidateParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ))

#define CheckParameters1( m, a1 ) \
            CheckParms( ( m, a1 ) )
#define CheckParameters2( m, a1, a2 )   \
            CheckParms( ( m, a1, a2 ))
#define CheckParameters3( m, a1, a2, a3 )   \
            CheckParms( ( m, a1, a2, a3 ))
#define CheckParameters4( m, a1, a2, a3, a4 ) \
            CheckParms( ( m, a1, a2, a3, a4 ))
#define CheckParameters5( m, a1, a2, a3, a4, a5 ) \
            CheckParms( ( m, a1, a2, a3, a4, a5 ))
#define CheckParameters6( m, a1, a2, a3, a4, a5, a6 ) \
            CheckParms( ( m, a1, a2, a3, a4, a5, a6 ))
#define CheckParameters7( m, a1, a2, a3, a4, a5, a6, a7 ) \
            CheckParms( ( m, a1, a2, a3, a4, a5, a6, a7 ))
#define CheckParameters8( m, a1, a2, a3, a4, a5, a6, a7, a8 ) \
            CheckParms( ( m, a1, a2, a3, a4, a5, a6, a7, a8 ))
#define CheckParameters9( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
            CheckParms( ( m, a1, a2, a3, a4, a5, a6, a7, a8, a9 ))
#define CheckParameters10( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
            CheckParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ))
#define CheckParameters11( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ) \
            CheckParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11 ))
#define CheckParameters12( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ) \
            CheckParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 ))
#define CheckParameters13( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
            CheckParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ))
#define CheckParameters14( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ) \
            CheckParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14 ))
#define CheckParameters15( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ) \
            CheckParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15 ))
#define CheckParameters16( m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ) \
            CheckParms( (  m, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16 ))

#endif /* _X86_ || WIN16 */


/*
 *      M A P I    P A R A M E T E R   V A L I D A T I O N    M A C R O S
 */


/* IUnknown */

#define Validate_IUnknown_QueryInterface( a1, a2, a3 ) \
             ValidateParameters3( IUnknown_QueryInterface, a1, a2, a3 )
#define UlValidate_IUnknown_QueryInterface( a1, a2, a3 ) \
             UlValidateParameters3( IUnknown_QueryInterface, a1, a2, a3 )
#define CheckParameters_IUnknown_QueryInterface( a1, a2, a3 ) \
             CheckParameters3( IUnknown_QueryInterface, a1, a2, a3 )

#define Validate_IUnknown_AddRef( a1 ) \
             ValidateParameters1( IUnknown_AddRef, a1 )
#define UlValidate_IUnknown_AddRef( a1 ) \
             UlValidateParameters1( IUnknown_AddRef, a1 )
#define CheckParameters_IUnknown_AddRef( a1 ) \
             CheckParameters1( IUnknown_AddRef, a1 )

#define Validate_IUnknown_Release( a1 ) \
             ValidateParameters1( IUnknown_Release, a1 )
#define UlValidate_IUnknown_Release( a1 ) \
             UlValidateParameters1( IUnknown_Release, a1 )
#define CheckParameters_IUnknown_Release( a1 ) \
             CheckParameters1( IUnknown_Release, a1 )


/* IMAPIProp */

#define Validate_IMAPIProp_GetLastError( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPIProp_GetLastError, a1, a2, a3, a4 )
#define UlValidate_IMAPIProp_GetLastError( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPIProp_GetLastError, a1, a2, a3, a4 )
#define CheckParameters_IMAPIProp_GetLastError( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPIProp_GetLastError, a1, a2, a3, a4 )

#define Validate_IMAPIProp_SaveChanges( a1, a2 ) \
             ValidateParameters2( IMAPIProp_SaveChanges, a1, a2 )
#define UlValidate_IMAPIProp_SaveChanges( a1, a2 ) \
             UlValidateParameters2( IMAPIProp_SaveChanges, a1, a2 )
#define CheckParameters_IMAPIProp_SaveChanges( a1, a2 ) \
             CheckParameters2( IMAPIProp_SaveChanges, a1, a2 )

#define Validate_IMAPIProp_GetProps( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPIProp_GetProps, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPIProp_GetProps( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPIProp_GetProps, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPIProp_GetProps( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPIProp_GetProps, a1, a2, a3, a4, a5 )

#define Validate_IMAPIProp_GetPropList( a1, a2, a3 ) \
             ValidateParameters3( IMAPIProp_GetPropList, a1, a2, a3 )
#define UlValidate_IMAPIProp_GetPropList( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIProp_GetPropList, a1, a2, a3 )
#define CheckParameters_IMAPIProp_GetPropList( a1, a2, a3 ) \
             CheckParameters3( IMAPIProp_GetPropList, a1, a2, a3 )

#define Validate_IMAPIProp_OpenProperty( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IMAPIProp_OpenProperty, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IMAPIProp_OpenProperty( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IMAPIProp_OpenProperty, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IMAPIProp_OpenProperty( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IMAPIProp_OpenProperty, a1, a2, a3, a4, a5, a6 )

#define Validate_IMAPIProp_SetProps( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPIProp_SetProps, a1, a2, a3, a4 )
#define UlValidate_IMAPIProp_SetProps( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPIProp_SetProps, a1, a2, a3, a4 )
#define CheckParameters_IMAPIProp_SetProps( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPIProp_SetProps, a1, a2, a3, a4 )

#define Validate_IMAPIProp_DeleteProps( a1, a2, a3 ) \
             ValidateParameters3( IMAPIProp_DeleteProps, a1, a2, a3 )
#define UlValidate_IMAPIProp_DeleteProps( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIProp_DeleteProps, a1, a2, a3 )
#define CheckParameters_IMAPIProp_DeleteProps( a1, a2, a3 ) \
             CheckParameters3( IMAPIProp_DeleteProps, a1, a2, a3 )

#define Validate_IMAPIProp_CopyTo( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
             ValidateParameters10( IMAPIProp_CopyTo, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 )
#define UlValidate_IMAPIProp_CopyTo( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
             UlValidateParameters10( IMAPIProp_CopyTo, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 )
#define CheckParameters_IMAPIProp_CopyTo( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) \
             CheckParameters10( IMAPIProp_CopyTo, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 )

#define Validate_IMAPIProp_CopyProps( a1, a2, a3, a4, a5, a6, a7, a8 ) \
             ValidateParameters8( IMAPIProp_CopyProps, a1, a2, a3, a4, a5, a6, a7, a8 )
#define UlValidate_IMAPIProp_CopyProps( a1, a2, a3, a4, a5, a6, a7, a8 ) \
             UlValidateParameters8( IMAPIProp_CopyProps, a1, a2, a3, a4, a5, a6, a7, a8 )
#define CheckParameters_IMAPIProp_CopyProps( a1, a2, a3, a4, a5, a6, a7, a8 ) \
             CheckParameters8( IMAPIProp_CopyProps, a1, a2, a3, a4, a5, a6, a7, a8 )

#define Validate_IMAPIProp_GetNamesFromIDs( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IMAPIProp_GetNamesFromIDs, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IMAPIProp_GetNamesFromIDs( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IMAPIProp_GetNamesFromIDs, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IMAPIProp_GetNamesFromIDs( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IMAPIProp_GetNamesFromIDs, a1, a2, a3, a4, a5, a6 )

#define Validate_IMAPIProp_GetIDsFromNames( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPIProp_GetIDsFromNames, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPIProp_GetIDsFromNames( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPIProp_GetIDsFromNames, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPIProp_GetIDsFromNames( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPIProp_GetIDsFromNames, a1, a2, a3, a4, a5 )


/* IMAPITable */

#define Validate_IMAPITable_GetLastError( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPITable_GetLastError, a1, a2, a3, a4 )
#define UlValidate_IMAPITable_GetLastError( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPITable_GetLastError, a1, a2, a3, a4 )
#define CheckParameters_IMAPITable_GetLastError( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPITable_GetLastError, a1, a2, a3, a4 )

#define Validate_IMAPITable_Advise( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPITable_Advise, a1, a2, a3, a4 )
#define UlValidate_IMAPITable_Advise( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPITable_Advise, a1, a2, a3, a4 )
#define CheckParameters_IMAPITable_Advise( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPITable_Advise, a1, a2, a3, a4 )

#define Validate_IMAPITable_Unadvise( a1, a2 ) \
             ValidateParameters2( IMAPITable_Unadvise, a1, a2 )
#define UlValidate_IMAPITable_Unadvise( a1, a2 ) \
             UlValidateParameters2( IMAPITable_Unadvise, a1, a2 )
#define CheckParameters_IMAPITable_Unadvise( a1, a2 ) \
             CheckParameters2( IMAPITable_Unadvise, a1, a2 )

#define Validate_IMAPITable_GetStatus( a1, a2, a3 ) \
             ValidateParameters3( IMAPITable_GetStatus, a1, a2, a3 )
#define UlValidate_IMAPITable_GetStatus( a1, a2, a3 ) \
             UlValidateParameters3( IMAPITable_GetStatus, a1, a2, a3 )
#define CheckParameters_IMAPITable_GetStatus( a1, a2, a3 ) \
             CheckParameters3( IMAPITable_GetStatus, a1, a2, a3 )

#define Validate_IMAPITable_SetColumns( a1, a2, a3 ) \
             ValidateParameters3( IMAPITable_SetColumns, a1, a2, a3 )
#define UlValidate_IMAPITable_SetColumns( a1, a2, a3 ) \
             UlValidateParameters3( IMAPITable_SetColumns, a1, a2, a3 )
#define CheckParameters_IMAPITable_SetColumns( a1, a2, a3 ) \
             CheckParameters3( IMAPITable_SetColumns, a1, a2, a3 )

#define Validate_IMAPITable_QueryColumns( a1, a2, a3 ) \
             ValidateParameters3( IMAPITable_QueryColumns, a1, a2, a3 )
#define UlValidate_IMAPITable_QueryColumns( a1, a2, a3 ) \
             UlValidateParameters3( IMAPITable_QueryColumns, a1, a2, a3 )
#define CheckParameters_IMAPITable_QueryColumns( a1, a2, a3 ) \
             CheckParameters3( IMAPITable_QueryColumns, a1, a2, a3 )

#define Validate_IMAPITable_GetRowCount( a1, a2, a3 ) \
             ValidateParameters3( IMAPITable_GetRowCount, a1, a2, a3 )
#define UlValidate_IMAPITable_GetRowCount( a1, a2, a3 ) \
             UlValidateParameters3( IMAPITable_GetRowCount, a1, a2, a3 )
#define CheckParameters_IMAPITable_GetRowCount( a1, a2, a3 ) \
             CheckParameters3( IMAPITable_GetRowCount, a1, a2, a3 )

#define Validate_IMAPITable_SeekRow( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPITable_SeekRow, a1, a2, a3, a4 )
#define UlValidate_IMAPITable_SeekRow( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPITable_SeekRow, a1, a2, a3, a4 )
#define CheckParameters_IMAPITable_SeekRow( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPITable_SeekRow, a1, a2, a3, a4 )

#define Validate_IMAPITable_SeekRowApprox( a1, a2, a3 ) \
             ValidateParameters3( IMAPITable_SeekRowApprox, a1, a2, a3 )
#define UlValidate_IMAPITable_SeekRowApprox( a1, a2, a3 ) \
             UlValidateParameters3( IMAPITable_SeekRowApprox, a1, a2, a3 )
#define CheckParameters_IMAPITable_SeekRowApprox( a1, a2, a3 ) \
             CheckParameters3( IMAPITable_SeekRowApprox, a1, a2, a3 )

#define Validate_IMAPITable_QueryPosition( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPITable_QueryPosition, a1, a2, a3, a4 )
#define UlValidate_IMAPITable_QueryPosition( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPITable_QueryPosition, a1, a2, a3, a4 )
#define CheckParameters_IMAPITable_QueryPosition( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPITable_QueryPosition, a1, a2, a3, a4 )

#define Validate_IMAPITable_FindRow( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPITable_FindRow, a1, a2, a3, a4 )
#define UlValidate_IMAPITable_FindRow( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPITable_FindRow, a1, a2, a3, a4 )
#define CheckParameters_IMAPITable_FindRow( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPITable_FindRow, a1, a2, a3, a4 )

#define Validate_IMAPITable_Restrict( a1, a2, a3 ) \
             ValidateParameters3( IMAPITable_Restrict, a1, a2, a3 )
#define UlValidate_IMAPITable_Restrict( a1, a2, a3 ) \
             UlValidateParameters3( IMAPITable_Restrict, a1, a2, a3 )
#define CheckParameters_IMAPITable_Restrict( a1, a2, a3 ) \
             CheckParameters3( IMAPITable_Restrict, a1, a2, a3 )

#define Validate_IMAPITable_CreateBookmark( a1, a2 ) \
             ValidateParameters2( IMAPITable_CreateBookmark, a1, a2 )
#define UlValidate_IMAPITable_CreateBookmark( a1, a2 ) \
             UlValidateParameters2( IMAPITable_CreateBookmark, a1, a2 )
#define CheckParameters_IMAPITable_CreateBookmark( a1, a2 ) \
             CheckParameters2( IMAPITable_CreateBookmark, a1, a2 )

#define Validate_IMAPITable_FreeBookmark( a1, a2 ) \
             ValidateParameters2( IMAPITable_FreeBookmark, a1, a2 )
#define UlValidate_IMAPITable_FreeBookmark( a1, a2 ) \
             UlValidateParameters2( IMAPITable_FreeBookmark, a1, a2 )
#define CheckParameters_IMAPITable_FreeBookmark( a1, a2 ) \
             CheckParameters2( IMAPITable_FreeBookmark, a1, a2 )

#define Validate_IMAPITable_SortTable( a1, a2, a3 ) \
             ValidateParameters3( IMAPITable_SortTable, a1, a2, a3 )
#define UlValidate_IMAPITable_SortTable( a1, a2, a3 ) \
             UlValidateParameters3( IMAPITable_SortTable, a1, a2, a3 )
#define CheckParameters_IMAPITable_SortTable( a1, a2, a3 ) \
             CheckParameters3( IMAPITable_SortTable, a1, a2, a3 )

#define Validate_IMAPITable_QuerySortOrder( a1, a2 ) \
             ValidateParameters2( IMAPITable_QuerySortOrder, a1, a2 )
#define UlValidate_IMAPITable_QuerySortOrder( a1, a2 ) \
             UlValidateParameters2( IMAPITable_QuerySortOrder, a1, a2 )
#define CheckParameters_IMAPITable_QuerySortOrder( a1, a2 ) \
             CheckParameters2( IMAPITable_QuerySortOrder, a1, a2 )

#define Validate_IMAPITable_QueryRows( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPITable_QueryRows, a1, a2, a3, a4 )
#define UlValidate_IMAPITable_QueryRows( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPITable_QueryRows, a1, a2, a3, a4 )
#define CheckParameters_IMAPITable_QueryRows( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPITable_QueryRows, a1, a2, a3, a4 )

#define Validate_IMAPITable_Abort( a1 ) \
             ValidateParameters1( IMAPITable_Abort, a1 )
#define UlValidate_IMAPITable_Abort( a1 ) \
             UlValidateParameters1( IMAPITable_Abort, a1 )
#define CheckParameters_IMAPITable_Abort( a1 ) \
             CheckParameters1( IMAPITable_Abort, a1 )

#define Validate_IMAPITable_ExpandRow( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMAPITable_ExpandRow, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMAPITable_ExpandRow( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMAPITable_ExpandRow, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMAPITable_ExpandRow( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMAPITable_ExpandRow, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IMAPITable_CollapseRow( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPITable_CollapseRow, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPITable_CollapseRow( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPITable_CollapseRow, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPITable_CollapseRow( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPITable_CollapseRow, a1, a2, a3, a4, a5 )

#define Validate_IMAPITable_WaitForCompletion( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPITable_WaitForCompletion, a1, a2, a3, a4 )
#define UlValidate_IMAPITable_WaitForCompletion( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPITable_WaitForCompletion, a1, a2, a3, a4 )
#define CheckParameters_IMAPITable_WaitForCompletion( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPITable_WaitForCompletion, a1, a2, a3, a4 )

#define Validate_IMAPITable_GetCollapseState( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IMAPITable_GetCollapseState, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IMAPITable_GetCollapseState( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IMAPITable_GetCollapseState, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IMAPITable_GetCollapseState( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IMAPITable_GetCollapseState, a1, a2, a3, a4, a5, a6 )

#define Validate_IMAPITable_SetCollapseState( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPITable_SetCollapseState, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPITable_SetCollapseState( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPITable_SetCollapseState, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPITable_SetCollapseState( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPITable_SetCollapseState, a1, a2, a3, a4, a5 )


/* IMAPIContainer */

#define Validate_IMAPIContainer_GetContentsTable( a1, a2, a3 ) \
             ValidateParameters3( IMAPIContainer_GetContentsTable, a1, a2, a3 )
#define UlValidate_IMAPIContainer_GetContentsTable( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIContainer_GetContentsTable, a1, a2, a3 )
#define CheckParameters_IMAPIContainer_GetContentsTable( a1, a2, a3 ) \
             CheckParameters3( IMAPIContainer_GetContentsTable, a1, a2, a3 )

#define Validate_IMAPIContainer_GetHierarchyTable( a1, a2, a3 ) \
             ValidateParameters3( IMAPIContainer_GetHierarchyTable, a1, a2, a3 )
#define UlValidate_IMAPIContainer_GetHierarchyTable( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIContainer_GetHierarchyTable, a1, a2, a3 )
#define CheckParameters_IMAPIContainer_GetHierarchyTable( a1, a2, a3 ) \
             CheckParameters3( IMAPIContainer_GetHierarchyTable, a1, a2, a3 )

#define Validate_IMAPIContainer_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMAPIContainer_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMAPIContainer_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMAPIContainer_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMAPIContainer_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMAPIContainer_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IMAPIContainer_SetSearchCriteria( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPIContainer_SetSearchCriteria, a1, a2, a3, a4 )
#define UlValidate_IMAPIContainer_SetSearchCriteria( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPIContainer_SetSearchCriteria, a1, a2, a3, a4 )
#define CheckParameters_IMAPIContainer_SetSearchCriteria( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPIContainer_SetSearchCriteria, a1, a2, a3, a4 )

#define Validate_IMAPIContainer_GetSearchCriteria( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPIContainer_GetSearchCriteria, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPIContainer_GetSearchCriteria( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPIContainer_GetSearchCriteria, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPIContainer_GetSearchCriteria( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPIContainer_GetSearchCriteria, a1, a2, a3, a4, a5 )


/* IABContainer */

#define Validate_IABContainer_CreateEntry( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IABContainer_CreateEntry, a1, a2, a3, a4, a5 )
#define UlValidate_IABContainer_CreateEntry( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IABContainer_CreateEntry, a1, a2, a3, a4, a5 )
#define CheckParameters_IABContainer_CreateEntry( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IABContainer_CreateEntry, a1, a2, a3, a4, a5 )

#define Validate_IABContainer_CopyEntries( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IABContainer_CopyEntries, a1, a2, a3, a4, a5 )
#define UlValidate_IABContainer_CopyEntries( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IABContainer_CopyEntries, a1, a2, a3, a4, a5 )
#define CheckParameters_IABContainer_CopyEntries( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IABContainer_CopyEntries, a1, a2, a3, a4, a5 )

#define Validate_IABContainer_DeleteEntries( a1, a2, a3 ) \
             ValidateParameters3( IABContainer_DeleteEntries, a1, a2, a3 )
#define UlValidate_IABContainer_DeleteEntries( a1, a2, a3 ) \
             UlValidateParameters3( IABContainer_DeleteEntries, a1, a2, a3 )
#define CheckParameters_IABContainer_DeleteEntries( a1, a2, a3 ) \
             CheckParameters3( IABContainer_DeleteEntries, a1, a2, a3 )

#define Validate_IABContainer_ResolveNames( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IABContainer_ResolveNames, a1, a2, a3, a4, a5 )
#define UlValidate_IABContainer_ResolveNames( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IABContainer_ResolveNames, a1, a2, a3, a4, a5 )
#define CheckParameters_IABContainer_ResolveNames( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IABContainer_ResolveNames, a1, a2, a3, a4, a5 )


/* IDistList */

#define Validate_IDistList_CreateEntry( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IDistList_CreateEntry, a1, a2, a3, a4, a5 )
#define UlValidate_IDistList_CreateEntry( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IDistList_CreateEntry, a1, a2, a3, a4, a5 )
#define CheckParameters_IDistList_CreateEntry( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IDistList_CreateEntry, a1, a2, a3, a4, a5 )

#define Validate_IDistList_CopyEntries( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IDistList_CopyEntries, a1, a2, a3, a4, a5 )
#define UlValidate_IDistList_CopyEntries( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IDistList_CopyEntries, a1, a2, a3, a4, a5 )
#define CheckParameters_IDistList_CopyEntries( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IDistList_CopyEntries, a1, a2, a3, a4, a5 )

#define Validate_IDistList_DeleteEntries( a1, a2, a3 ) \
             ValidateParameters3( IDistList_DeleteEntries, a1, a2, a3 )
#define UlValidate_IDistList_DeleteEntries( a1, a2, a3 ) \
             UlValidateParameters3( IDistList_DeleteEntries, a1, a2, a3 )
#define CheckParameters_IDistList_DeleteEntries( a1, a2, a3 ) \
             CheckParameters3( IDistList_DeleteEntries, a1, a2, a3 )

#define Validate_IDistList_ResolveNames( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IDistList_ResolveNames, a1, a2, a3, a4, a5 )
#define UlValidate_IDistList_ResolveNames( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IDistList_ResolveNames, a1, a2, a3, a4, a5 )
#define CheckParameters_IDistList_ResolveNames( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IDistList_ResolveNames, a1, a2, a3, a4, a5 )


/* IMAPIFolder */

#define Validate_IMAPIFolder_CreateMessage( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPIFolder_CreateMessage, a1, a2, a3, a4 )
#define UlValidate_IMAPIFolder_CreateMessage( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPIFolder_CreateMessage, a1, a2, a3, a4 )
#define CheckParameters_IMAPIFolder_CreateMessage( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPIFolder_CreateMessage, a1, a2, a3, a4 )

#define Validate_IMAPIFolder_CopyMessages( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMAPIFolder_CopyMessages, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMAPIFolder_CopyMessages( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMAPIFolder_CopyMessages, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMAPIFolder_CopyMessages( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMAPIFolder_CopyMessages, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IMAPIFolder_DeleteMessages( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPIFolder_DeleteMessages, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPIFolder_DeleteMessages( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPIFolder_DeleteMessages, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPIFolder_DeleteMessages( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPIFolder_DeleteMessages, a1, a2, a3, a4, a5 )

#define Validate_IMAPIFolder_CreateFolder( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMAPIFolder_CreateFolder, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMAPIFolder_CreateFolder( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMAPIFolder_CreateFolder, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMAPIFolder_CreateFolder( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMAPIFolder_CreateFolder, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IMAPIFolder_CopyFolder( a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
             ValidateParameters9( IMAPIFolder_CopyFolder, a1, a2, a3, a4, a5, a6, a7, a8, a9 )
#define UlValidate_IMAPIFolder_CopyFolder( a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
             UlValidateParameters9( IMAPIFolder_CopyFolder, a1, a2, a3, a4, a5, a6, a7, a8, a9 )
#define CheckParameters_IMAPIFolder_CopyFolder( a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
             CheckParameters9( IMAPIFolder_CopyFolder, a1, a2, a3, a4, a5, a6, a7, a8, a9 )

#define Validate_IMAPIFolder_DeleteFolder( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IMAPIFolder_DeleteFolder, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IMAPIFolder_DeleteFolder( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IMAPIFolder_DeleteFolder, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IMAPIFolder_DeleteFolder( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IMAPIFolder_DeleteFolder, a1, a2, a3, a4, a5, a6 )

#define Validate_IMAPIFolder_SetReadFlags( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPIFolder_SetReadFlags, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPIFolder_SetReadFlags( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPIFolder_SetReadFlags, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPIFolder_SetReadFlags( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPIFolder_SetReadFlags, a1, a2, a3, a4, a5 )

#define Validate_IMAPIFolder_GetMessageStatus( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPIFolder_GetMessageStatus, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPIFolder_GetMessageStatus( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPIFolder_GetMessageStatus, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPIFolder_GetMessageStatus( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPIFolder_GetMessageStatus, a1, a2, a3, a4, a5 )

#define Validate_IMAPIFolder_SetMessageStatus( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IMAPIFolder_SetMessageStatus, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IMAPIFolder_SetMessageStatus( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IMAPIFolder_SetMessageStatus, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IMAPIFolder_SetMessageStatus( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IMAPIFolder_SetMessageStatus, a1, a2, a3, a4, a5, a6 )

#define Validate_IMAPIFolder_SaveContentsSort( a1, a2, a3 ) \
             ValidateParameters3( IMAPIFolder_SaveContentsSort, a1, a2, a3 )
#define UlValidate_IMAPIFolder_SaveContentsSort( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIFolder_SaveContentsSort, a1, a2, a3 )
#define CheckParameters_IMAPIFolder_SaveContentsSort( a1, a2, a3 ) \
             CheckParameters3( IMAPIFolder_SaveContentsSort, a1, a2, a3 )

#define Validate_IMAPIFolder_EmptyFolder( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPIFolder_EmptyFolder, a1, a2, a3, a4 )
#define UlValidate_IMAPIFolder_EmptyFolder( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPIFolder_EmptyFolder, a1, a2, a3, a4 )
#define CheckParameters_IMAPIFolder_EmptyFolder( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPIFolder_EmptyFolder, a1, a2, a3, a4 )


/* IMsgStore */

#define Validate_IMsgStore_Advise( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IMsgStore_Advise, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IMsgStore_Advise( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IMsgStore_Advise, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IMsgStore_Advise( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IMsgStore_Advise, a1, a2, a3, a4, a5, a6 )

#define Validate_IMsgStore_Unadvise( a1, a2 ) \
             ValidateParameters2( IMsgStore_Unadvise, a1, a2 )
#define UlValidate_IMsgStore_Unadvise( a1, a2 ) \
             UlValidateParameters2( IMsgStore_Unadvise, a1, a2 )
#define CheckParameters_IMsgStore_Unadvise( a1, a2 ) \
             CheckParameters2( IMsgStore_Unadvise, a1, a2 )

#define Validate_IMsgStore_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMsgStore_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMsgStore_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMsgStore_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMsgStore_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMsgStore_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IMsgStore_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMsgStore_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMsgStore_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMsgStore_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMsgStore_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMsgStore_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IMsgStore_SetReceiveFolder( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMsgStore_SetReceiveFolder, a1, a2, a3, a4, a5 )
#define UlValidate_IMsgStore_SetReceiveFolder( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMsgStore_SetReceiveFolder, a1, a2, a3, a4, a5 )
#define CheckParameters_IMsgStore_SetReceiveFolder( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMsgStore_SetReceiveFolder, a1, a2, a3, a4, a5 )

#define Validate_IMsgStore_GetReceiveFolder( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IMsgStore_GetReceiveFolder, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IMsgStore_GetReceiveFolder( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IMsgStore_GetReceiveFolder, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IMsgStore_GetReceiveFolder( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IMsgStore_GetReceiveFolder, a1, a2, a3, a4, a5, a6 )

#define Validate_IMsgStore_GetReceiveFolderTable( a1, a2, a3 ) \
             ValidateParameters3( IMsgStore_GetReceiveFolderTable, a1, a2, a3 )
#define UlValidate_IMsgStore_GetReceiveFolderTable( a1, a2, a3 ) \
             UlValidateParameters3( IMsgStore_GetReceiveFolderTable, a1, a2, a3 )
#define CheckParameters_IMsgStore_GetReceiveFolderTable( a1, a2, a3 ) \
             CheckParameters3( IMsgStore_GetReceiveFolderTable, a1, a2, a3 )

#define Validate_IMsgStore_StoreLogoff( a1, a2 ) \
             ValidateParameters2( IMsgStore_StoreLogoff, a1, a2 )
#define UlValidate_IMsgStore_StoreLogoff( a1, a2 ) \
             UlValidateParameters2( IMsgStore_StoreLogoff, a1, a2 )
#define CheckParameters_IMsgStore_StoreLogoff( a1, a2 ) \
             CheckParameters2( IMsgStore_StoreLogoff, a1, a2 )

#define Validate_IMsgStore_AbortSubmit( a1, a2, a3, a4 ) \
             ValidateParameters4( IMsgStore_AbortSubmit, a1, a2, a3, a4 )
#define UlValidate_IMsgStore_AbortSubmit( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMsgStore_AbortSubmit, a1, a2, a3, a4 )
#define CheckParameters_IMsgStore_AbortSubmit( a1, a2, a3, a4 ) \
             CheckParameters4( IMsgStore_AbortSubmit, a1, a2, a3, a4 )

#define Validate_IMsgStore_GetOutgoingQueue( a1, a2, a3 ) \
             ValidateParameters3( IMsgStore_GetOutgoingQueue, a1, a2, a3 )
#define UlValidate_IMsgStore_GetOutgoingQueue( a1, a2, a3 ) \
             UlValidateParameters3( IMsgStore_GetOutgoingQueue, a1, a2, a3 )
#define CheckParameters_IMsgStore_GetOutgoingQueue( a1, a2, a3 ) \
             CheckParameters3( IMsgStore_GetOutgoingQueue, a1, a2, a3 )

#define Validate_IMsgStore_SetLockState( a1, a2, a3 ) \
             ValidateParameters3( IMsgStore_SetLockState, a1, a2, a3 )
#define UlValidate_IMsgStore_SetLockState( a1, a2, a3 ) \
             UlValidateParameters3( IMsgStore_SetLockState, a1, a2, a3 )
#define CheckParameters_IMsgStore_SetLockState( a1, a2, a3 ) \
             CheckParameters3( IMsgStore_SetLockState, a1, a2, a3 )

#define Validate_IMsgStore_FinishedMsg( a1, a2, a3, a4 ) \
             ValidateParameters4( IMsgStore_FinishedMsg, a1, a2, a3, a4 )
#define UlValidate_IMsgStore_FinishedMsg( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMsgStore_FinishedMsg, a1, a2, a3, a4 )
#define CheckParameters_IMsgStore_FinishedMsg( a1, a2, a3, a4 ) \
             CheckParameters4( IMsgStore_FinishedMsg, a1, a2, a3, a4 )

#define Validate_IMsgStore_NotifyNewMail( a1, a2 ) \
             ValidateParameters2( IMsgStore_NotifyNewMail, a1, a2 )
#define UlValidate_IMsgStore_NotifyNewMail( a1, a2 ) \
             UlValidateParameters2( IMsgStore_NotifyNewMail, a1, a2 )
#define CheckParameters_IMsgStore_NotifyNewMail( a1, a2 ) \
             CheckParameters2( IMsgStore_NotifyNewMail, a1, a2 )


/* IMessage */

#define Validate_IMessage_GetAttachmentTable( a1, a2, a3 ) \
             ValidateParameters3( IMessage_GetAttachmentTable, a1, a2, a3 )
#define UlValidate_IMessage_GetAttachmentTable( a1, a2, a3 ) \
             UlValidateParameters3( IMessage_GetAttachmentTable, a1, a2, a3 )
#define CheckParameters_IMessage_GetAttachmentTable( a1, a2, a3 ) \
             CheckParameters3( IMessage_GetAttachmentTable, a1, a2, a3 )

#define Validate_IMessage_OpenAttach( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMessage_OpenAttach, a1, a2, a3, a4, a5 )
#define UlValidate_IMessage_OpenAttach( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMessage_OpenAttach, a1, a2, a3, a4, a5 )
#define CheckParameters_IMessage_OpenAttach( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMessage_OpenAttach, a1, a2, a3, a4, a5 )

#define Validate_IMessage_CreateAttach( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMessage_CreateAttach, a1, a2, a3, a4, a5 )
#define UlValidate_IMessage_CreateAttach( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMessage_CreateAttach, a1, a2, a3, a4, a5 )
#define CheckParameters_IMessage_CreateAttach( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMessage_CreateAttach, a1, a2, a3, a4, a5 )

#define Validate_IMessage_DeleteAttach( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMessage_DeleteAttach, a1, a2, a3, a4, a5 )
#define UlValidate_IMessage_DeleteAttach( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMessage_DeleteAttach, a1, a2, a3, a4, a5 )
#define CheckParameters_IMessage_DeleteAttach( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMessage_DeleteAttach, a1, a2, a3, a4, a5 )

#define Validate_IMessage_GetRecipientTable( a1, a2, a3 ) \
             ValidateParameters3( IMessage_GetRecipientTable, a1, a2, a3 )
#define UlValidate_IMessage_GetRecipientTable( a1, a2, a3 ) \
             UlValidateParameters3( IMessage_GetRecipientTable, a1, a2, a3 )
#define CheckParameters_IMessage_GetRecipientTable( a1, a2, a3 ) \
             CheckParameters3( IMessage_GetRecipientTable, a1, a2, a3 )

#define Validate_IMessage_ModifyRecipients( a1, a2, a3 ) \
             ValidateParameters3( IMessage_ModifyRecipients, a1, a2, a3 )
#define UlValidate_IMessage_ModifyRecipients( a1, a2, a3 ) \
             UlValidateParameters3( IMessage_ModifyRecipients, a1, a2, a3 )
#define CheckParameters_IMessage_ModifyRecipients( a1, a2, a3 ) \
             CheckParameters3( IMessage_ModifyRecipients, a1, a2, a3 )

#define Validate_IMessage_SubmitMessage( a1, a2 ) \
             ValidateParameters2( IMessage_SubmitMessage, a1, a2 )
#define UlValidate_IMessage_SubmitMessage( a1, a2 ) \
             UlValidateParameters2( IMessage_SubmitMessage, a1, a2 )
#define CheckParameters_IMessage_SubmitMessage( a1, a2 ) \
             CheckParameters2( IMessage_SubmitMessage, a1, a2 )

#define Validate_IMessage_SetReadFlag( a1, a2 ) \
             ValidateParameters2( IMessage_SetReadFlag, a1, a2 )
#define UlValidate_IMessage_SetReadFlag( a1, a2 ) \
             UlValidateParameters2( IMessage_SetReadFlag, a1, a2 )
#define CheckParameters_IMessage_SetReadFlag( a1, a2 ) \
             CheckParameters2( IMessage_SetReadFlag, a1, a2 )


/* IABProvider */

#define Validate_IABProvider_Shutdown( a1, a2 ) \
             ValidateParameters2( IABProvider_Shutdown, a1, a2 )
#define UlValidate_IABProvider_Shutdown( a1, a2 ) \
             UlValidateParameters2( IABProvider_Shutdown, a1, a2 )
#define CheckParameters_IABProvider_Shutdown( a1, a2 ) \
             CheckParameters2( IABProvider_Shutdown, a1, a2 )

#define Validate_IABProvider_Logon( a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
             ValidateParameters9( IABProvider_Logon, a1, a2, a3, a4, a5, a6, a7, a8, a9 )
#define UlValidate_IABProvider_Logon( a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
             UlValidateParameters9( IABProvider_Logon, a1, a2, a3, a4, a5, a6, a7, a8, a9 )
#define CheckParameters_IABProvider_Logon( a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
             CheckParameters9( IABProvider_Logon, a1, a2, a3, a4, a5, a6, a7, a8, a9 )


/* IABLogon */

#define Validate_IABLogon_GetLastError( a1, a2, a3, a4 ) \
             ValidateParameters4( IABLogon_GetLastError, a1, a2, a3, a4 )
#define UlValidate_IABLogon_GetLastError( a1, a2, a3, a4 ) \
             UlValidateParameters4( IABLogon_GetLastError, a1, a2, a3, a4 )
#define CheckParameters_IABLogon_GetLastError( a1, a2, a3, a4 ) \
             CheckParameters4( IABLogon_GetLastError, a1, a2, a3, a4 )

#define Validate_IABLogon_Logoff( a1, a2 ) \
             ValidateParameters2( IABLogon_Logoff, a1, a2 )
#define UlValidate_IABLogon_Logoff( a1, a2 ) \
             UlValidateParameters2( IABLogon_Logoff, a1, a2 )
#define CheckParameters_IABLogon_Logoff( a1, a2 ) \
             CheckParameters2( IABLogon_Logoff, a1, a2 )

#define Validate_IABLogon_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IABLogon_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IABLogon_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IABLogon_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IABLogon_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IABLogon_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IABLogon_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IABLogon_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IABLogon_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IABLogon_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IABLogon_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IABLogon_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IABLogon_Advise( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IABLogon_Advise, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IABLogon_Advise( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IABLogon_Advise, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IABLogon_Advise( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IABLogon_Advise, a1, a2, a3, a4, a5, a6 )

#define Validate_IABLogon_Unadvise( a1, a2 ) \
             ValidateParameters2( IABLogon_Unadvise, a1, a2 )
#define UlValidate_IABLogon_Unadvise( a1, a2 ) \
             UlValidateParameters2( IABLogon_Unadvise, a1, a2 )
#define CheckParameters_IABLogon_Unadvise( a1, a2 ) \
             CheckParameters2( IABLogon_Unadvise, a1, a2 )

#define Validate_IABLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IABLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )
#define UlValidate_IABLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IABLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )
#define CheckParameters_IABLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IABLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )

#define Validate_IABLogon_OpenTemplateID( a1, a2, a3, a4, a5, a6, a7, a8 ) \
             ValidateParameters8( IABLogon_OpenTemplateID, a1, a2, a3, a4, a5, a6, a7, a8 )
#define UlValidate_IABLogon_OpenTemplateID( a1, a2, a3, a4, a5, a6, a7, a8 ) \
             UlValidateParameters8( IABLogon_OpenTemplateID, a1, a2, a3, a4, a5, a6, a7, a8 )
#define CheckParameters_IABLogon_OpenTemplateID( a1, a2, a3, a4, a5, a6, a7, a8 ) \
             CheckParameters8( IABLogon_OpenTemplateID, a1, a2, a3, a4, a5, a6, a7, a8 )

#define Validate_IABLogon_GetOneOffTable( a1, a2, a3 ) \
             ValidateParameters3( IABLogon_GetOneOffTable, a1, a2, a3 )
#define UlValidate_IABLogon_GetOneOffTable( a1, a2, a3 ) \
             UlValidateParameters3( IABLogon_GetOneOffTable, a1, a2, a3 )
#define CheckParameters_IABLogon_GetOneOffTable( a1, a2, a3 ) \
             CheckParameters3( IABLogon_GetOneOffTable, a1, a2, a3 )

#define Validate_IABLogon_PrepareRecips( a1, a2, a3, a4 ) \
             ValidateParameters4( IABLogon_PrepareRecips, a1, a2, a3, a4 )
#define UlValidate_IABLogon_PrepareRecips( a1, a2, a3, a4 ) \
             UlValidateParameters4( IABLogon_PrepareRecips, a1, a2, a3, a4 )
#define CheckParameters_IABLogon_PrepareRecips( a1, a2, a3, a4 ) \
             CheckParameters4( IABLogon_PrepareRecips, a1, a2, a3, a4 )


/* IXPProvider */

#define Validate_IXPProvider_Shutdown( a1, a2 ) \
             ValidateParameters2( IXPProvider_Shutdown, a1, a2 )
#define UlValidate_IXPProvider_Shutdown( a1, a2 ) \
             UlValidateParameters2( IXPProvider_Shutdown, a1, a2 )
#define CheckParameters_IXPProvider_Shutdown( a1, a2 ) \
             CheckParameters2( IXPProvider_Shutdown, a1, a2 )

#define Validate_IXPProvider_TransportLogon( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IXPProvider_TransportLogon, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IXPProvider_TransportLogon( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IXPProvider_TransportLogon, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IXPProvider_TransportLogon( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IXPProvider_TransportLogon, a1, a2, a3, a4, a5, a6, a7 )


/* IXPLogon */

#define Validate_IXPLogon_AddressTypes( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IXPLogon_AddressTypes, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IXPLogon_AddressTypes( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IXPLogon_AddressTypes, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IXPLogon_AddressTypes( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IXPLogon_AddressTypes, a1, a2, a3, a4, a5, a6 )

#define Validate_IXPLogon_RegisterOptions( a1, a2, a3, a4 ) \
             ValidateParameters4( IXPLogon_RegisterOptions, a1, a2, a3, a4 )
#define UlValidate_IXPLogon_RegisterOptions( a1, a2, a3, a4 ) \
             UlValidateParameters4( IXPLogon_RegisterOptions, a1, a2, a3, a4 )
#define CheckParameters_IXPLogon_RegisterOptions( a1, a2, a3, a4 ) \
             CheckParameters4( IXPLogon_RegisterOptions, a1, a2, a3, a4 )

#define Validate_IXPLogon_TransportNotify( a1, a2, a3 ) \
             ValidateParameters3( IXPLogon_TransportNotify, a1, a2, a3 )
#define UlValidate_IXPLogon_TransportNotify( a1, a2, a3 ) \
             UlValidateParameters3( IXPLogon_TransportNotify, a1, a2, a3 )
#define CheckParameters_IXPLogon_TransportNotify( a1, a2, a3 ) \
             CheckParameters3( IXPLogon_TransportNotify, a1, a2, a3 )

#define Validate_IXPLogon_Idle( a1, a2 ) \
             ValidateParameters2( IXPLogon_Idle, a1, a2 )
#define UlValidate_IXPLogon_Idle( a1, a2 ) \
             UlValidateParameters2( IXPLogon_Idle, a1, a2 )
#define CheckParameters_IXPLogon_Idle( a1, a2 ) \
             CheckParameters2( IXPLogon_Idle, a1, a2 )

#define Validate_IXPLogon_TransportLogoff( a1, a2 ) \
             ValidateParameters2( IXPLogon_TransportLogoff, a1, a2 )
#define UlValidate_IXPLogon_TransportLogoff( a1, a2 ) \
             UlValidateParameters2( IXPLogon_TransportLogoff, a1, a2 )
#define CheckParameters_IXPLogon_TransportLogoff( a1, a2 ) \
             CheckParameters2( IXPLogon_TransportLogoff, a1, a2 )

#define Validate_IXPLogon_SubmitMessage( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IXPLogon_SubmitMessage, a1, a2, a3, a4, a5 )
#define UlValidate_IXPLogon_SubmitMessage( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IXPLogon_SubmitMessage, a1, a2, a3, a4, a5 )
#define CheckParameters_IXPLogon_SubmitMessage( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IXPLogon_SubmitMessage, a1, a2, a3, a4, a5 )

#define Validate_IXPLogon_EndMessage( a1, a2, a3 ) \
             ValidateParameters3( IXPLogon_EndMessage, a1, a2, a3 )
#define UlValidate_IXPLogon_EndMessage( a1, a2, a3 ) \
             UlValidateParameters3( IXPLogon_EndMessage, a1, a2, a3 )
#define CheckParameters_IXPLogon_EndMessage( a1, a2, a3 ) \
             CheckParameters3( IXPLogon_EndMessage, a1, a2, a3 )

#define Validate_IXPLogon_Poll( a1, a2 ) \
             ValidateParameters2( IXPLogon_Poll, a1, a2 )
#define UlValidate_IXPLogon_Poll( a1, a2 ) \
             UlValidateParameters2( IXPLogon_Poll, a1, a2 )
#define CheckParameters_IXPLogon_Poll( a1, a2 ) \
             CheckParameters2( IXPLogon_Poll, a1, a2 )

#define Validate_IXPLogon_StartMessage( a1, a2, a3, a4 ) \
             ValidateParameters4( IXPLogon_StartMessage, a1, a2, a3, a4 )
#define UlValidate_IXPLogon_StartMessage( a1, a2, a3, a4 ) \
             UlValidateParameters4( IXPLogon_StartMessage, a1, a2, a3, a4 )
#define CheckParameters_IXPLogon_StartMessage( a1, a2, a3, a4 ) \
             CheckParameters4( IXPLogon_StartMessage, a1, a2, a3, a4 )

#define Validate_IXPLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IXPLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )
#define UlValidate_IXPLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IXPLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )
#define CheckParameters_IXPLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IXPLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )

#define Validate_IXPLogon_ValidateState( a1, a2, a3 ) \
             ValidateParameters3( IXPLogon_ValidateState, a1, a2, a3 )
#define UlValidate_IXPLogon_ValidateState( a1, a2, a3 ) \
             UlValidateParameters3( IXPLogon_ValidateState, a1, a2, a3 )
#define CheckParameters_IXPLogon_ValidateState( a1, a2, a3 ) \
             CheckParameters3( IXPLogon_ValidateState, a1, a2, a3 )

#define Validate_IXPLogon_FlushQueues( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IXPLogon_FlushQueues, a1, a2, a3, a4, a5 )
#define UlValidate_IXPLogon_FlushQueues( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IXPLogon_FlushQueues, a1, a2, a3, a4, a5 )
#define CheckParameters_IXPLogon_FlushQueues( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IXPLogon_FlushQueues, a1, a2, a3, a4, a5 )


/* IMSProvider */

#define Validate_IMSProvider_Shutdown( a1, a2 ) \
             ValidateParameters2( IMSProvider_Shutdown, a1, a2 )
#define UlValidate_IMSProvider_Shutdown( a1, a2 ) \
             UlValidateParameters2( IMSProvider_Shutdown, a1, a2 )
#define CheckParameters_IMSProvider_Shutdown( a1, a2 ) \
             CheckParameters2( IMSProvider_Shutdown, a1, a2 )

#define Validate_IMSProvider_Logon( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
             ValidateParameters13( IMSProvider_Logon, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 )
#define UlValidate_IMSProvider_Logon( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
             UlValidateParameters13( IMSProvider_Logon, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 )
#define CheckParameters_IMSProvider_Logon( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
             CheckParameters13( IMSProvider_Logon, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 )

#define Validate_IMSProvider_SpoolerLogon( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
             ValidateParameters13( IMSProvider_SpoolerLogon, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 )
#define UlValidate_IMSProvider_SpoolerLogon( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
             UlValidateParameters13( IMSProvider_SpoolerLogon, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 )
#define CheckParameters_IMSProvider_SpoolerLogon( a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 ) \
             CheckParameters13( IMSProvider_SpoolerLogon, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13 )

#define Validate_IMSProvider_CompareStoreIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMSProvider_CompareStoreIDs, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMSProvider_CompareStoreIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMSProvider_CompareStoreIDs, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMSProvider_CompareStoreIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMSProvider_CompareStoreIDs, a1, a2, a3, a4, a5, a6, a7 )


/* IMSLogon */

#define Validate_IMSLogon_GetLastError( a1, a2, a3, a4 ) \
             ValidateParameters4( IMSLogon_GetLastError, a1, a2, a3, a4 )
#define UlValidate_IMSLogon_GetLastError( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMSLogon_GetLastError, a1, a2, a3, a4 )
#define CheckParameters_IMSLogon_GetLastError( a1, a2, a3, a4 ) \
             CheckParameters4( IMSLogon_GetLastError, a1, a2, a3, a4 )

#define Validate_IMSLogon_Logoff( a1, a2 ) \
             ValidateParameters2( IMSLogon_Logoff, a1, a2 )
#define UlValidate_IMSLogon_Logoff( a1, a2 ) \
             UlValidateParameters2( IMSLogon_Logoff, a1, a2 )
#define CheckParameters_IMSLogon_Logoff( a1, a2 ) \
             CheckParameters2( IMSLogon_Logoff, a1, a2 )

#define Validate_IMSLogon_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMSLogon_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMSLogon_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMSLogon_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMSLogon_OpenEntry( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMSLogon_OpenEntry, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IMSLogon_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             ValidateParameters7( IMSLogon_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )
#define UlValidate_IMSLogon_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             UlValidateParameters7( IMSLogon_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )
#define CheckParameters_IMSLogon_CompareEntryIDs( a1, a2, a3, a4, a5, a6, a7 ) \
             CheckParameters7( IMSLogon_CompareEntryIDs, a1, a2, a3, a4, a5, a6, a7 )

#define Validate_IMSLogon_Advise( a1, a2, a3, a4, a5, a6 ) \
             ValidateParameters6( IMSLogon_Advise, a1, a2, a3, a4, a5, a6 )
#define UlValidate_IMSLogon_Advise( a1, a2, a3, a4, a5, a6 ) \
             UlValidateParameters6( IMSLogon_Advise, a1, a2, a3, a4, a5, a6 )
#define CheckParameters_IMSLogon_Advise( a1, a2, a3, a4, a5, a6 ) \
             CheckParameters6( IMSLogon_Advise, a1, a2, a3, a4, a5, a6 )

#define Validate_IMSLogon_Unadvise( a1, a2 ) \
             ValidateParameters2( IMSLogon_Unadvise, a1, a2 )
#define UlValidate_IMSLogon_Unadvise( a1, a2 ) \
             UlValidateParameters2( IMSLogon_Unadvise, a1, a2 )
#define CheckParameters_IMSLogon_Unadvise( a1, a2 ) \
             CheckParameters2( IMSLogon_Unadvise, a1, a2 )

#define Validate_IMSLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMSLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )
#define UlValidate_IMSLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMSLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )
#define CheckParameters_IMSLogon_OpenStatusEntry( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMSLogon_OpenStatusEntry, a1, a2, a3, a4, a5 )


/* IMAPIControl */

#define Validate_IMAPIControl_GetLastError( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPIControl_GetLastError, a1, a2, a3, a4 )
#define UlValidate_IMAPIControl_GetLastError( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPIControl_GetLastError, a1, a2, a3, a4 )
#define CheckParameters_IMAPIControl_GetLastError( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPIControl_GetLastError, a1, a2, a3, a4 )

#define Validate_IMAPIControl_Activate( a1, a2, a3 ) \
             ValidateParameters3( IMAPIControl_Activate, a1, a2, a3 )
#define UlValidate_IMAPIControl_Activate( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIControl_Activate, a1, a2, a3 )
#define CheckParameters_IMAPIControl_Activate( a1, a2, a3 ) \
             CheckParameters3( IMAPIControl_Activate, a1, a2, a3 )

#define Validate_IMAPIControl_GetState( a1, a2, a3 ) \
             ValidateParameters3( IMAPIControl_GetState, a1, a2, a3 )
#define UlValidate_IMAPIControl_GetState( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIControl_GetState, a1, a2, a3 )
#define CheckParameters_IMAPIControl_GetState( a1, a2, a3 ) \
             CheckParameters3( IMAPIControl_GetState, a1, a2, a3 )


/* IMAPIStatus */

#define Validate_IMAPIStatus_ValidateState( a1, a2, a3 ) \
             ValidateParameters3( IMAPIStatus_ValidateState, a1, a2, a3 )
#define UlValidate_IMAPIStatus_ValidateState( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIStatus_ValidateState, a1, a2, a3 )
#define CheckParameters_IMAPIStatus_ValidateState( a1, a2, a3 ) \
             CheckParameters3( IMAPIStatus_ValidateState, a1, a2, a3 )

#define Validate_IMAPIStatus_SettingsDialog( a1, a2, a3 ) \
             ValidateParameters3( IMAPIStatus_SettingsDialog, a1, a2, a3 )
#define UlValidate_IMAPIStatus_SettingsDialog( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIStatus_SettingsDialog, a1, a2, a3 )
#define CheckParameters_IMAPIStatus_SettingsDialog( a1, a2, a3 ) \
             CheckParameters3( IMAPIStatus_SettingsDialog, a1, a2, a3 )

#define Validate_IMAPIStatus_ChangePassword( a1, a2, a3, a4 ) \
             ValidateParameters4( IMAPIStatus_ChangePassword, a1, a2, a3, a4 )
#define UlValidate_IMAPIStatus_ChangePassword( a1, a2, a3, a4 ) \
             UlValidateParameters4( IMAPIStatus_ChangePassword, a1, a2, a3, a4 )
#define CheckParameters_IMAPIStatus_ChangePassword( a1, a2, a3, a4 ) \
             CheckParameters4( IMAPIStatus_ChangePassword, a1, a2, a3, a4 )

#define Validate_IMAPIStatus_FlushQueues( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IMAPIStatus_FlushQueues, a1, a2, a3, a4, a5 )
#define UlValidate_IMAPIStatus_FlushQueues( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IMAPIStatus_FlushQueues, a1, a2, a3, a4, a5 )
#define CheckParameters_IMAPIStatus_FlushQueues( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IMAPIStatus_FlushQueues, a1, a2, a3, a4, a5 )


/* IStream */

#define Validate_IStream_Read( a1, a2, a3, a4 ) \
             ValidateParameters4( IStream_Read, a1, a2, a3, a4 )
#define UlValidate_IStream_Read( a1, a2, a3, a4 ) \
             UlValidateParameters4( IStream_Read, a1, a2, a3, a4 )
#define CheckParameters_IStream_Read( a1, a2, a3, a4 ) \
             CheckParameters4( IStream_Read, a1, a2, a3, a4 )

#define Validate_IStream_Write( a1, a2, a3, a4 ) \
             ValidateParameters4( IStream_Write, a1, a2, a3, a4 )
#define UlValidate_IStream_Write( a1, a2, a3, a4 ) \
             UlValidateParameters4( IStream_Write, a1, a2, a3, a4 )
#define CheckParameters_IStream_Write( a1, a2, a3, a4 ) \
             CheckParameters4( IStream_Write, a1, a2, a3, a4 )

#define Validate_IStream_Seek( a1, a2, a3, a4 ) \
             ValidateParameters4( IStream_Seek, a1, a2, a3, a4 )
#define UlValidate_IStream_Seek( a1, a2, a3, a4 ) \
             UlValidateParameters4( IStream_Seek, a1, a2, a3, a4 )
#define CheckParameters_IStream_Seek( a1, a2, a3, a4 ) \
             CheckParameters4( IStream_Seek, a1, a2, a3, a4 )

#define Validate_IStream_SetSize( a1, a2 ) \
             ValidateParameters2( IStream_SetSize, a1, a2 )
#define UlValidate_IStream_SetSize( a1, a2 ) \
             UlValidateParameters2( IStream_SetSize, a1, a2 )
#define CheckParameters_IStream_SetSize( a1, a2 ) \
             CheckParameters2( IStream_SetSize, a1, a2 )

#define Validate_IStream_CopyTo( a1, a2, a3, a4, a5 ) \
             ValidateParameters5( IStream_CopyTo, a1, a2, a3, a4, a5 )
#define UlValidate_IStream_CopyTo( a1, a2, a3, a4, a5 ) \
             UlValidateParameters5( IStream_CopyTo, a1, a2, a3, a4, a5 )
#define CheckParameters_IStream_CopyTo( a1, a2, a3, a4, a5 ) \
             CheckParameters5( IStream_CopyTo, a1, a2, a3, a4, a5 )

#define Validate_IStream_Commit( a1, a2 ) \
             ValidateParameters2( IStream_Commit, a1, a2 )
#define UlValidate_IStream_Commit( a1, a2 ) \
             UlValidateParameters2( IStream_Commit, a1, a2 )
#define CheckParameters_IStream_Commit( a1, a2 ) \
             CheckParameters2( IStream_Commit, a1, a2 )

#define Validate_IStream_Revert( a1 ) \
             ValidateParameters1( IStream_Revert, a1 )
#define UlValidate_IStream_Revert( a1 ) \
             UlValidateParameters1( IStream_Revert, a1 )
#define CheckParameters_IStream_Revert( a1 ) \
             CheckParameters1( IStream_Revert, a1 )

#define Validate_IStream_LockRegion( a1, a2, a3, a4 ) \
             ValidateParameters4( IStream_LockRegion, a1, a2, a3, a4 )
#define UlValidate_IStream_LockRegion( a1, a2, a3, a4 ) \
             UlValidateParameters4( IStream_LockRegion, a1, a2, a3, a4 )
#define CheckParameters_IStream_LockRegion( a1, a2, a3, a4 ) \
             CheckParameters4( IStream_LockRegion, a1, a2, a3, a4 )

#define Validate_IStream_UnlockRegion( a1, a2, a3, a4 ) \
             ValidateParameters4( IStream_UnlockRegion, a1, a2, a3, a4 )
#define UlValidate_IStream_UnlockRegion( a1, a2, a3, a4 ) \
             UlValidateParameters4( IStream_UnlockRegion, a1, a2, a3, a4 )
#define CheckParameters_IStream_UnlockRegion( a1, a2, a3, a4 ) \
             CheckParameters4( IStream_UnlockRegion, a1, a2, a3, a4 )

#define Validate_IStream_Stat( a1, a2, a3 ) \
             ValidateParameters3( IStream_Stat, a1, a2, a3 )
#define UlValidate_IStream_Stat( a1, a2, a3 ) \
             UlValidateParameters3( IStream_Stat, a1, a2, a3 )
#define CheckParameters_IStream_Stat( a1, a2, a3 ) \
             CheckParameters3( IStream_Stat, a1, a2, a3 )

#define Validate_IStream_Clone( a1, a2 ) \
             ValidateParameters2( IStream_Clone, a1, a2 )
#define UlValidate_IStream_Clone( a1, a2 ) \
             UlValidateParameters2( IStream_Clone, a1, a2 )
#define CheckParameters_IStream_Clone( a1, a2 ) \
             CheckParameters2( IStream_Clone, a1, a2 )


/* IMAPIAdviseSink */

#define Validate_IMAPIAdviseSink_OnNotify( a1, a2, a3 ) \
             ValidateParameters3( IMAPIAdviseSink_OnNotify, a1, a2, a3 )
#define UlValidate_IMAPIAdviseSink_OnNotify( a1, a2, a3 ) \
             UlValidateParameters3( IMAPIAdviseSink_OnNotify, a1, a2, a3 )
#define CheckParameters_IMAPIAdviseSink_OnNotify( a1, a2, a3 ) \
             CheckParameters3( IMAPIAdviseSink_OnNotify, a1, a2, a3 )


#ifdef WIN16
HRESULT 
PASCAL  HrValidateParameters( METHODS eMethod, LPVOID FAR *ppFirstArg );
#elif defined(_X86_)
STDAPI  HrValidateParameters( METHODS eMethod, LPVOID FAR *ppFirstArg );
#else
STDAPIV HrValidateParametersV( METHODS eMethod, ... );
STDAPIV HrValidateParametersValist( METHODS eMethod, va_list arglist );
#endif /* WIN16 */


#ifdef __cplusplus
}
#endif

#endif  /* _INC_VALIDATE */

