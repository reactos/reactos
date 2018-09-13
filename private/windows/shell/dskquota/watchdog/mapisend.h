#ifndef __MAPISEND_H
#define __MAPISEND_H
///////////////////////////////////////////////////////////////////////////////
/*  File: mapisend.h

    Description: Implements the most basic MAPI email client to send a message
        to one or more recipients.  All operations are done without UI.

        classes:    CMapiSession
                    CMapiMessage
                    CMapiMessageBody
                    CMapiRecipients
                    MAPI

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/97    Initial creation.                                    BrianAu
    06/22/97    Added class MAPI so clients can dynamically link     BrianAu
                to MAPI32.DLL.
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef MAPIX_H
#   include <mapix.h>
#endif

#ifndef _MAPIUTIL_H_
#   include <mapiutil.h>
#endif

#ifndef MAPITAGS_H
#   include <mapitags.h>
#endif


//-----------------------------------------------------------------------------
// MAPI
//
// This class allows a MAPI client to dynamically link to MAPI32.DLL instead
// of statically linking to it.  This can help performance in that loading
// MAPI is explicitly controlled.  The following example shows how to use
// the class.
//
// MAPI mapi;
// HRESULT hr;
//
// hr = mapi.Load(); // This calls LoadLibrary on MAPI32.DLL.
// if SUCCEEDED(hr))
// {
//     hr = mapi.Initialize(NULL);
//     if (SUCCEEDED(hr))
//     {
//         LPMAPISESSION pSession;
//         hr = mapi.LogonEx(0,             // Hwnd for any UI.
//                           NULL,          // Profile name.
//                           NULL,          // Password.
//                           dwLogonFlags,  // Flags
//                           &pSession);    // Session obj ptr (out).
//         if (SUCCEEDED(hr))
//         {
//             //
//             // Use MAPI interfaces.
//             //
//         }
//         mapi.Uninitialize();
//     }
//     mapi.Unload();  // This calls FreeLibrary for MAPI32.DLL
// }
//
//
//-----------------------------------------------------------------------------

//
// Function pointer typedefs for some MAPI functions.
// Used for defining function pointers in class MAPI.
//
//
// HrQueryAllRows
//
typedef HRESULT (STDMETHODCALLTYPE MAPIHRQUERYALLROWS)(
            LPMAPITABLE lpTable,
            LPSPropTagArray lpPropTags,
            LPSRestriction lpRestriction,
            LPSSortOrderSet lpSortOrderSet,
            LONG crowsMax,
            LPSRowSet FAR *lppRows);

typedef MAPIHRQUERYALLROWS FAR *LPMAPIHRQUERYALLROWS;
//
// FreePadrlist
//
typedef VOID (STDAPICALLTYPE MAPIFREEPADRLIST)(LPADRLIST lpAdrList);
typedef MAPIFREEPADRLIST FAR *LPMAPIFREEPADRLIST;
//
// FreeProws
//
typedef VOID (STDAPICALLTYPE MAPIFREEPROWS)(LPSRowSet lpRows);
typedef MAPIFREEPROWS FAR *LPMAPIFREEPROWS;


class MAPI
{
    public:
        MAPI(VOID);
        ~MAPI(VOID);

        HRESULT Load(VOID);
        VOID Unload(VOID);

        HRESULT LogonEx(
            ULONG ulUIParam,   
            LPTSTR lpszProfileName,   
            LPTSTR lpszPassword,   
            FLAGS flFlags,   
            LPMAPISESSION FAR * lppSession);

        HRESULT Initialize(
            LPVOID lpMapiInit);

        VOID Uninitialize(VOID);

        SCODE AllocateBuffer(
            ULONG cbSize,   
            LPVOID FAR * lppBuffer);

        SCODE AllocateMore(
            ULONG cbSize,   
            LPVOID lpObject,   
            LPVOID FAR * lppBuffer);

        ULONG FreeBuffer(
            LPVOID lpBuffer);

        HRESULT HrQueryAllRows(
            LPMAPITABLE lpTable,
            LPSPropTagArray lpPropTags,
            LPSRestriction lpRestriction,
            LPSSortOrderSet lpSortOrderSet,
            LONG crowsMax,
            LPSRowSet FAR *lppRows);

        VOID FreePadrlist(LPADRLIST lpAdrlist);

        VOID FreeProws(LPSRowSet lpRows);

    private:
        static LONG                 m_cLoadCount;
        static HINSTANCE            m_hmodMAPI;
        static LPMAPIINITIALIZE     m_pfnInitialize;
        static LPMAPILOGONEX        m_pfnLogonEx;
        static LPMAPIUNINITIALIZE   m_pfnUninitialize;
        static LPMAPIALLOCATEBUFFER m_pfnAllocateBuffer;
        static LPMAPIALLOCATEMORE   m_pfnAllocateMore;
        static LPMAPIFREEBUFFER     m_pfnFreeBuffer;
        static LPMAPIHRQUERYALLROWS m_pfnHrQueryAllRows;
        static LPMAPIFREEPADRLIST   m_pfnFreePadrlist;             
        static LPMAPIFREEPROWS      m_pfnFreeProws;

        //
        // Prevent copy.
        //
        MAPI(const MAPI& rhs);
        MAPI& operator = (const MAPI& rhs);
};



//-----------------------------------------------------------------------------
// CMapiRecipients
//      Hides the grossness of adding entries and properties to a MAPI
//      address list.
//-----------------------------------------------------------------------------
class CMapiRecipients
{
    public:
        CMapiRecipients(BOOL bUnicode = TRUE);
        ~CMapiRecipients(VOID);

        CMapiRecipients(const CMapiRecipients& rhs);
        CMapiRecipients& operator = (const CMapiRecipients& rhs);

        //
        // Does the address list contain unicode names?
        //
        BOOL IsUnicode(VOID)
            { return m_bUnicode; }
        //
        // Add a recipient to the address list.
        //
        HRESULT AddRecipient(LPCTSTR pszEmailName, DWORD dwType);
        //
        // Return a count of entries in the address list.
        //
        INT Count(VOID) const;

        //
        // Make it easy to use a CMapiRecipients object in calls to MAPI
        // functions taking an LPADRLIST.  Do note however that by 
        // giving a MAPI function direct access to the address list,
        // you circumvent the automatic growth function built into this
        // class.  This conversion capability is generally intended for
        // resolving names (which modifies the list but doesn't add entries) 
        // or for read access to the list.
        //
        operator LPADRLIST() const
            { return m_pal; }
            
    private:
        LPADRLIST   m_pal;          // Ptr to actual address list.
        UINT        m_cMaxEntries;  // Size of list (max entries)
        BOOL        m_bUnicode;     // Contains Ansi or Unicode names?
        static UINT m_cGrowIncr;    // How many entries we grow by each time.

        HRESULT Grow(
            UINT cEntries);

        HRESULT CopyAdrListEntry(
            ADRENTRY& Dest,
            ADRENTRY& Src,
            BOOL bConvertStrings);

        HRESULT CopySPropVal(
            LPVOID pvBaseAlloc,
            SPropValue& Dest,
            SPropValue& Src,
            BOOL bConvertStrings);

        HRESULT CopyVarLengthSPropVal(
            LPVOID pvBaseAlloc,
            LPVOID *ppvDest,
            LPVOID pvSrc,
            INT cb);

        HRESULT CopySPropValString(
            LPVOID pvBaseAlloc,
            LPSTR *ppszDestA,
            LPCWSTR pszSrcW);

        HRESULT CopySPropValString(
            LPVOID pvBaseAlloc,
            LPWSTR *ppszDestW,
            LPCSTR pszSrcA);
};


//-----------------------------------------------------------------------------
// CMapiMessageBody
//      Encapsulates the addition of text into a MAPI message body.
//      Maintains body text in an OLE stream so that we can easily append
//      text as needed.   Provides formatted text input through Append().
//-----------------------------------------------------------------------------
class CMapiMessageBody
{
    public:
        CMapiMessageBody(VOID);
        ~CMapiMessageBody(VOID);

        CMapiMessageBody(const CMapiMessageBody& rhs);
        CMapiMessageBody& operator = (const CMapiMessageBody& rhs);

        //
        // Append text to the stream.
        //
        HRESULT Append(LPCTSTR pszFmt, ...);
        HRESULT Append(HINSTANCE hInst, UINT idFmt, ...);
        HRESULT Append(LPCTSTR pszFmt, va_list *pargs);
        HRESULT Append(HINSTANCE hInst, UINT idFmt, va_list *pargs);

        BOOL IsValid(VOID)
            { return NULL != m_pStg && NULL != m_pStm; }

        operator LPSTREAM()
            { return m_pStm; }

        operator LPSTORAGE()
            { return m_pStg; }

    private:
        LPSTORAGE m_pStg;
        LPSTREAM  m_pStm;

        HRESULT CommonConstruct(VOID);
};


//-----------------------------------------------------------------------------
// CMapiMessage
//-----------------------------------------------------------------------------
class CMapiMessage
{
    public:
        CMapiMessage(LPMAPIFOLDER pFolder);
        CMapiMessage(LPMAPIFOLDER pFolder, CMapiMessageBody& body, LPCTSTR pszSubject = NULL);
        ~CMapiMessage(VOID);

        //
        // Set the message's subject line.
        //
        HRESULT SetSubject(LPCTSTR pszSubject);
        //
        // Append a line of text to the message body.
        //
        HRESULT Append(LPCTSTR pszFmt, ...);
        HRESULT Append(HINSTANCE hInst, UINT idFmt, ...);
        //
        // Set the message's recipient list.
        //
        HRESULT SetRecipients(LPADRLIST pAdrList);
        //
        // Set props on a message.
        //
        HRESULT SetProps(ULONG cValues, 
                         LPSPropValue lpPropArray, 
                         LPSPropProblemArray *lppProblems);
        //
        // Save property changes.
        //
        HRESULT SaveChanges(ULONG ulFlags);
        //
        // Send the message.
        // Assumes address list is completely resolved.
        //
        HRESULT Send(LPADRLIST pAdrList = NULL);
        
    private:
        LPMESSAGE        m_pMsg;
        CMapiMessageBody m_body;

        HRESULT CommonConstruct(LPMAPIFOLDER pFolder);

        //
        // Prevent copy.
        //
        CMapiMessage(const CMapiMessage& rhs);
        CMapiMessage& operator = (const CMapiMessage& rhs);
};


//-----------------------------------------------------------------------------
// CMapiSession
//      Encapsulates a basic MAPI session for sending very simple text 
//      messages.
//-----------------------------------------------------------------------------
class CMapiSession
{
    public:
        CMapiSession(VOID);
        ~CMapiSession(VOID);

        //
        // Log on to MAPI, open the default store and open the outbox.
        // If this succeeds, you're ready to send messages.  The dtor
        // will close everything down.
        //
        HRESULT Initialize(VOID);
        //
        // Send a message to a list of recipients.
        //
        HRESULT Send(LPADRLIST pAdrList, LPCTSTR pszSubject, LPCTSTR pszMsg);
        HRESULT Send(LPADRLIST pAdrList, LPCTSTR pszSubject, CMapiMessageBody& body);
        HRESULT Send(LPADRLIST pAdrList, CMapiMessage& msg);
        //
        // Get pointer to the address book.
        //
        HRESULT GetAddressBook(LPADRBOOK *ppAdrBook);
        //
        // Get pointer to outbox folder.
        //
        HRESULT GetOutBoxFolder(LPMAPIFOLDER *ppOutBoxFolder);
        //
        // Resolve a list of adresses.
        //
        HRESULT ResolveAddresses(LPADRLIST pAdrList);
        //
        // Get name of current session user.
        //
        HRESULT GetSessionUser(LPSPropValue *ppProps, ULONG *pcbProps);
        //
        // Report MAPI errors returned by some MAPI API calls.
        // The error information is returned in a MAPIERROR
        // structure.
        //
        VOID ReportErrorsReturned(HRESULT hr, BOOL bLogEvent = FALSE);


    private:
        LPMAPISESSION m_pSession;           // Ptr to MAPI session object.
        LPMDB         m_pDefMsgStore;       // Ptr to MAPI default msg store obj.
        LPMAPIFOLDER  m_pOutBoxFolder;      // Ptr to MAPI outbox folder object.
        BOOL          m_bMapiInitialized;   // Has MAPI been initialized?

        HRESULT OpenDefMsgStore(VOID);
        HRESULT OpenOutBoxFolder(VOID);

        //
        // Prevent copy.
        //
        CMapiSession(const CMapiSession& rhs);
        CMapiSession& operator = (const CMapiSession& rhs);
};



#endif //__MAPISEND_H
