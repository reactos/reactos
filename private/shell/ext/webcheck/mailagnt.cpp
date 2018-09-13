//
// WebCheck Mail Agent
//
// A user specifies that they are to be notified via email when a web checked
// object (usually a page) changes.
//
// When the subscription is the delivery agent will call the mail agent upon completion
//  with a temporary ISubscriptionItem
// 
// Julian Jiggins (julianj), January 8, 1997
//

#include "private.h"
#include "mapi.h"
#include "smtp.h"
#include "mlang.h"

#include <mluisupp.h>

#undef TF_THISMODULE
#define TF_THISMODULE   TF_MAILAGENT

//
// Global strings
// REVIEW move to a better place
//
#define MAIL_HANDLER    TEXT("Software\\Clients\\Mail")
#define MAIL_ATHENA     TEXT("Internet Mail and News")
#define SUBJECT_LINE TEXT("Subscription delivered")
#define MESSAGE_PREFIX TEXT(" \r\nThe website you requested ")
#define MESSAGE_SUFFIX TEXT(" has been delivered")

#define ATHENA_SMTP_SERVER \
    TEXT("Software\\Microsoft\\Internet Mail and News\\Mail\\SMTP")
#define NETSCAPE_SMTP_SERVER \
    TEXT("Software\\Netscape\\netscape Navigator\\Services\\SMTP_Server")
#define EUDORA_COMMANDLINE TEXT("Software\\Qualcomm\\Eudora\\CommandLine")

#define NOTE_TEXT_LENGTH 4096

#define ENCODING_STRLEN 32

//////////////////////////////////////////////////////////////////////////
//
// Email helper functions
//
//////////////////////////////////////////////////////////////////////////

//
// Returns a MemAlloc'd string with HTMLBreak inserted in place of '\d'.
//
void AddHTMLBreakText(LPSTR szText, LPSTR szHTMLBreak, LPSTR *lpHTMLText)
{
    ASSERT(szText);
    ASSERT(szHTMLBreak);
    ASSERT(!*lpHTMLText);

    LPSTR lpTmp = NULL, lpTmp2 = NULL, lpHTMLAbstract = NULL;
    int cbCRs = 0;
    int cbLFs = 0;
    DWORD dwExtra = 0;
    
    //
    // Count number of carriage returns
    //
    for (lpTmp = szText; *lpTmp; lpTmp++)
    {
        if (*lpTmp == 0x0d)
            cbCRs++;
        if (*lpTmp == 0x0a)
            cbLFs++;
    }
    
    dwExtra = lstrlenA(szText) - cbCRs - cbLFs + cbCRs * lstrlenA(szHTMLBreak) + 1;

    //
    // Allocate appropriate size string
    //
    *lpHTMLText = lpHTMLAbstract = (LPSTR)MemAlloc(LPTR, dwExtra);
    if (!lpHTMLAbstract)
        return;

    // 
    // Create new HTML abstract string.
    //
    for (lpTmp = szText; *lpTmp; lpTmp++)
    {
        if (*lpTmp == 0x0d)
        {
            for (lpTmp2 = szHTMLBreak; *lpTmp2; lpTmp2++, lpHTMLAbstract++)
                *lpHTMLAbstract = *lpTmp2;
        }
        else if (*lpTmp != 0x0a)
        {
            *lpHTMLAbstract = *lpTmp;
            lpHTMLAbstract++;
        }
    }

    *lpHTMLAbstract = '\0';

}

#ifdef DEBUG
void DBG_OUTPUT_MAPI_ERROR(ULONG ul)
{
    switch(ul)
    {
    case MAPI_E_LOGON_FAILURE: 
        DBG("MailAgent: MAPI LOGON FAILURE"); break;
    case MAPI_E_FAILURE:
        DBG("MailAgent: MAPI_E_FAILURE"); break;
    default: 
        DBG("MailAgent: Failed to send mail message"); break;
    }
}
#else
#define DBG_OUTPUT_MAPI_ERROR(ul)
#endif

//
// Build an HTML message containing a frameset that effectively inlines
// the requested URL
//
BOOL BuildHTMLMessage(LPSTR szEmailAddress, LPSTR szName, LPSTR szURL, 
                      CHAR **ppHTMLMessage,  LPSTR szTitle, LPSTR szAbstract,
                      LPSTR szSrcCharset)
{
    *ppHTMLMessage = NULL; // clear out parameter
    
    CHAR * lpBuffer = NULL;

    CHAR szWrapper[NOTE_TEXT_LENGTH];
    CHAR szMessageFormat[NOTE_TEXT_LENGTH];
    CHAR szMessageFormat2[NOTE_TEXT_LENGTH];
    CHAR szMessageText[NOTE_TEXT_LENGTH];
    CHAR szMessageHTML[NOTE_TEXT_LENGTH];
    CHAR szTextBreak[10];
    CHAR szHTMLBreak[10];

    //
    // Load the wrapper for the HTML message. This is the header stuff 
    // and multipart MIME and HTML goop
    //
    int iRet = MLLoadStringA(IDS_AGNT_HTMLMESSAGEWRAPPER, szWrapper, NOTE_TEXT_LENGTH);
    ASSERT(iRet > 0);

    if (szTitle != NULL) {

        // NOTE: Size is probably slightly larger than necessary due to %1's.

        LPSTR lpHTMLAbstract = NULL, lpNewAbstract = NULL;
        DWORD dwTotalSize = 0;
        //
        // load string for single HTML line break as well as tag on for custom email
        //

        MLLoadStringA(IDS_AGNT_EMAILMESSAGE, szMessageText, ARRAYSIZE(szMessageText));

        MLLoadStringA(IDS_AGNT_HTMLBREAKSINGLE, szHTMLBreak, ARRAYSIZE(szHTMLBreak));

        // 
        // Create new abstract string (szAbstract + email tagger)
        //
        dwTotalSize = lstrlenA(szAbstract) + lstrlenA(szMessageText) + 1;

        LPSTR szNewAbstract = (LPSTR)MemAlloc(LPTR, dwTotalSize * sizeof(CHAR));
        if (!szNewAbstract)
            return FALSE;

        lstrcpyA(szNewAbstract, szAbstract);
        StrCatA(szNewAbstract, szMessageText);
        
        AddHTMLBreakText(szNewAbstract, szHTMLBreak, &lpHTMLAbstract);
        if (!lpHTMLAbstract) 
        {
            MemFree(szNewAbstract);
            return FALSE;
        }
            
        dwTotalSize = lstrlenA(szWrapper) + lstrlenA(szEmailAddress) + 
                            2*lstrlenA(szTitle) + lstrlenA(szNewAbstract) + lstrlenA(szSrcCharset) +
                            lstrlenA(lpHTMLAbstract) + lstrlenA(szURL) + 1;

        lpBuffer = (CHAR *)MemAlloc(LPTR, dwTotalSize * sizeof(CHAR));
        if (!lpBuffer)
            return FALSE;

        LPSTR lpArguments[6];
        lpArguments[0] = szEmailAddress;
        lpArguments[1] = szTitle;
        lpArguments[2] = szNewAbstract;
        lpArguments[3] = szSrcCharset;    // the charset of the HTML page
        lpArguments[4] = szURL;
        lpArguments[5] = lpHTMLAbstract;

        //
        // Reason for FormatMessage is that wsprintf is limited up to 1024 bytes
        //

        FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                szWrapper, 0, 0, lpBuffer, dwTotalSize, (va_list *)&lpArguments[0]);

        MemFree(szNewAbstract);
        MemFree(lpHTMLAbstract);

    } else {

        //
        // Load line breaks for the plaintext and html messages
        //
        iRet = MLLoadStringA(IDS_AGNT_TEXTBREAK, szTextBreak, ARRAYSIZE(szTextBreak));
        ASSERT(iRet > 0);
        iRet = MLLoadStringA(IDS_AGNT_HTMLBREAK, szHTMLBreak, ARRAYSIZE(szHTMLBreak));
        ASSERT(iRet > 0);

        //
        // Load the actual text message to put sent
        //
        iRet = MLLoadStringA(IDS_AGNT_HTMLMESSAGETEXT, szMessageFormat, NOTE_TEXT_LENGTH);
        ASSERT(iRet > 0);

        iRet = MLLoadStringA(IDS_AGNT_HTMLMESSAGETEXT2, szMessageFormat2, NOTE_TEXT_LENGTH);
        ASSERT(iRet > 0);

        //
        // Insert the text messages into the wrapper. Note two message get
        // Once in the mime section for text/ascii and once in the 
        // noframes section of the text/html frameset. This is a work around
        // for lame clients (like Outlook) that think they can render HTML
        // but cannot really. 
        // The second message IDS_AGNT_HTMLMESSAGETEXT2 should NOT be localized
        // this is only going to be seen by Exchange users. In the future exchange
        // will handle html mail correct, so it acceptable that for example
        // Japanese Exchange users see english in this message. Most Japanese
        // users will user Outlook Express and so will just see the html message
        //

        // First we format 2 text messages, one for text and one for HTML,
        // since message itself is relatively small we know its < 1024 bytes

        iRet = wnsprintfA(szMessageText, ARRAYSIZE(szMessageText), szMessageFormat, 
                         szName, szTextBreak, szURL, szTextBreak);
        ASSERT(iRet > lstrlenA(szMessageFormat));

        iRet = wnsprintfA(szMessageHTML, ARRAYSIZE(szMessageHTML), szMessageFormat2, 
                         szName, szHTMLBreak, szURL, szHTMLBreak);
        ASSERT(iRet > lstrlenA(szMessageFormat2));

        DWORD dwTotalSize = lstrlenA(szWrapper) + lstrlenA(szEmailAddress) +
                            lstrlenA(szName) + lstrlenA(szMessageText) + lstrlenA(szSrcCharset) +
                            lstrlenA(szMessageHTML) + lstrlenA(szURL) + 1;

        lpBuffer = (CHAR *)MemAlloc(LPTR, dwTotalSize * sizeof(CHAR));
        if (!lpBuffer)
            return FALSE;

        LPSTR lpArguments[6];
        lpArguments[0] = szEmailAddress;  // target email address
        lpArguments[1] = szName;          // the name of the page that goes in the subject line  
        lpArguments[2] = szMessageText;   // the plain text message
        lpArguments[3] = szSrcCharset;    // the charset of the HTML page
        lpArguments[4] = szURL;           // the href of the page that goes in the frame set
        lpArguments[5] = szMessageHTML;   // the plain text message that goes in the 
                                          // noframes part of the frameset

        DWORD dwRet;
        dwRet = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            szWrapper, 0, 0, lpBuffer, dwTotalSize, (va_list *)&lpArguments[0]);
        ASSERT(dwRet);            
    }

    *ppHTMLMessage = lpBuffer;

    return TRUE;
}

//
// Build the actual text of the message to be sent via SMTP,
// load format string from resource and insert URL and URL's friently name.
//
void BuildSMTPMessage(LPSTR szName, LPSTR szURL, LPSTR *szMessage,
                      LPSTR szTitle, LPSTR szAbstract)
{
    CHAR szFormatText[NOTE_TEXT_LENGTH];
    int i;
    ASSERT(szMessage);
    
    if (!szMessage)
        return;


    *szMessage = NULL;

    if (szTitle != NULL) {
        i = MLLoadStringA(IDS_AGNT_SMTPMESSAGE_OTHER, szFormatText, NOTE_TEXT_LENGTH);
        ASSERT(i != 0);
        
        DWORD dwLen = lstrlenA(szFormatText) + lstrlenA(szTitle) + lstrlenA(szAbstract) + 1;

        *szMessage = (LPSTR) MemAlloc(LPTR, dwLen * sizeof(CHAR));
        if (!*szMessage)
            return;

        LPSTR lpArgs[2];
        lpArgs[0] = szTitle;
        lpArgs[1] = szAbstract;

        FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                szFormatText, 0, 0, *szMessage, dwLen, (va_list *)&lpArgs[0]);

    } else {
        i = MLLoadStringA(IDS_AGNT_SMTPMESSAGE, szFormatText, NOTE_TEXT_LENGTH);
        ASSERT(i != 0);
        
        DWORD dwLen = lstrlenA(szFormatText) + 2*lstrlenA(szName) + lstrlenA(szURL) + 1;

        *szMessage = (LPSTR) MemAlloc(LPTR, dwLen * sizeof(CHAR));
        if (!*szMessage)
            return;

        LPSTR lpArgs[3];
        lpArgs[0] = lpArgs[1] = szName;
        lpArgs[2] = szURL;

        FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                szFormatText, 0, 0, *szMessage, dwLen, (va_list *)&lpArgs[0]);
    }

}

#if 0
//
// Build MAPI message structure
//

void BuildMAPIMessage(
    LPTSTR lpszName, 
    LPTSTR lpszURL, 
    LPTSTR lpszEmailAddress,
    MapiMessage   * lpMessage,
    MapiRecipDesc * lpRecipient,
    LPTSTR lpszNoteText,                    // [out]
    LPTSTR lpszSubject,                     // [out]
    LPTSTR lpszTitle,                        // [in] title, NULL if not custom
    LPTSTR lpszAbstract)                     // [in] abstract, NULL if not custom
{
    TCHAR szFmtNoteText[NOTE_TEXT_LENGTH];
    TCHAR szFmtSubject[INTERNET_MAX_URL_LENGTH];
    int i;

    //
    // zero out the passed in mapi structures
    //
    ZeroMemory(lpMessage,   sizeof(MapiMessage));
    ZeroMemory(lpRecipient, sizeof(MapiRecipDesc));
#error The wsprintf's below need to be converted to wnsprintf
    //
    // Load the strings containing the bulk of the email message
    //
    if (lpszTitle != NULL) {
        MLLoadString(IDS_AGNT_MAPIMESSAGE_OTHER, szFmtNoteText, NOTE_TEXT_LENGTH);
        MLLoadString(IDS_AGNT_MAPISUBJECT_OTHER, szFmtSubject, INTERNET_MAX_URL_LENGTH);

        i = wsprintf(lpszNoteText, szFmtNoteText, lpszAbstract);
        ASSERT(i < NOTE_TEXT_LENGTH);
        i = wsprintf(lpszSubject, szFmtSubject, lpszTitle);
        ASSERT(i < INTERNET_MAX_URL_LENGTH);

    } else {
        MLLoadString(IDS_AGNT_MAPIMESSAGE, szFmtNoteText, NOTE_TEXT_LENGTH);
        MLLoadString(IDS_AGNT_MAPISUBJECT, szFmtSubject,  INTERNET_MAX_URL_LENGTH);

        i = wsprintf(lpszNoteText, szFmtNoteText, lpszName, lpszURL);
        ASSERT(i < NOTE_TEXT_LENGTH);
        i = wsprintf(lpszSubject,  szFmtSubject, lpszName);
        ASSERT(i < INTERNET_MAX_URL_LENGTH);
    }

    //
    // Build a mapi mail recipient structure
    //
    lpRecipient->ulRecipClass = MAPI_TO;
    lpRecipient->lpszName = lpszEmailAddress;

    //
    // Fill in the message subject line, recipient and note text
    //
    lpMessage->nRecipCount = 1;
    lpMessage->lpRecips = lpRecipient;
    lpMessage->lpszNoteText = lpszNoteText;
    lpMessage->lpszSubject  = lpszSubject;
}
#endif

//
// Use the MLANG apis to translate the string
//
// Returns success if translation occurred, fails otherwise
//
// Note if lpszSrcCharSet is NULL then use CP_ACP as the codepage
//

HRESULT TranslateCharset(
    LPSTR lpszSrcString, LPSTR lpszDstString, UINT uiDstSize,
    LPSTR lpszSrcCharset, LPSTR lpszDstCharset
    )
{
    HRESULT hr = E_FAIL;

    WCHAR wszSrcCharset[ENCODING_STRLEN];
    WCHAR wszDstCharset[ENCODING_STRLEN];

    if (lpszSrcString  == NULL || lpszDstString  == NULL || 
        lpszDstCharset == NULL)
    {
        return E_INVALIDARG;
    }

    SHAnsiToUnicode(lpszDstCharset, wszDstCharset, ARRAYSIZE(wszDstCharset));
    if (lpszSrcCharset)
        SHAnsiToUnicode(lpszSrcCharset, wszSrcCharset, ARRAYSIZE(wszSrcCharset));

    LPMULTILANGUAGE2 pIML2 = NULL;

    //
    // Create the MLANG object
    //
    if (SUCCEEDED(CoCreateInstance (CLSID_CMultiLanguage, NULL,
        CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (void**)&pIML2)))
    {
        UINT srcCodePage = (UINT)-1, dstCodePage;
        MIMECSETINFO mcsi = {0};

        //
        // First get the source code page either from the passed in string
        // name of source Charset or from the default one if null if passed in
        //
        if (lpszSrcCharset == NULL)
        {
            srcCodePage = GetACP();
            hr = S_OK;
        }
        else
        {
            //
            // Use the mlang object to get the codepages
            //
            hr = pIML2->GetCharsetInfo(wszSrcCharset, &mcsi);
            if (SUCCEEDED(hr))
            {
                srcCodePage = mcsi.uiInternetEncoding;
            }
        }
        
        if (SUCCEEDED(hr))
        {
            hr = pIML2->GetCharsetInfo(wszDstCharset, &mcsi);

            if (SUCCEEDED(hr))
            {
                dstCodePage = mcsi.uiInternetEncoding;

                if (srcCodePage != dstCodePage)
                {
                    //
                    // To work around a bug in the Mlang::ConvertString api
                    // have to pass in a ptr to length of the src string
                    //
                    UINT uiSrcSize = lstrlenA(lpszSrcString) + 1;

                    DWORD dwMode = 0;
                    hr = pIML2->ConvertString(
                        &dwMode, 
                        srcCodePage, 
                        dstCodePage,
                        (LPBYTE)lpszSrcString,
                        &uiSrcSize,
                        (LPBYTE)lpszDstString,
                        &uiDstSize);
                }
                else
                {
                    lstrcpynA(lpszDstString, lpszSrcString, uiDstSize);
                }
            }
        }
        pIML2->Release();
    }
    return hr;
}



//////////////////////////////////////////////////////////////////////////
//
// Mail notification implementation
//
//////////////////////////////////////////////////////////////////////////

//
// Notify via email that the pszURL has changed
//
// There are 3 ways to send via email -
//
// Use straight MAPI (IE Exchange or Outlook)
//      Most people don't have Exchange in the real world.
//
// Use Athena's MAPI implementation
//      It's broken and doesn't handle UI'less mode
//
// Use straight SMTP,
//      Need to get the name of an SMTP server
//
HRESULT
NotifyViaEMail(
    LPSTR lpszURL,             // url that was downloaded
    LPSTR lpszEmailAddress,    // email address to send notification to
    LPSTR lpszSMTPServer,      // SMTP server to use to deliver email
    LPSTR &lpszName,           // friendly name of url (probably page title)
    LPSTR lpszTitle,           // optional: NULL if not custom message
    LPSTR lpszAbstract,        // optional: NULL if not custom message
    LPSTR lpszCharSet,         // optional: charset of html page
    BOOL  fSendHTMLEmail )     // TRUE if registry allows it and check mode
                               // supports it.
{
    BOOL b;
    
    LPSTR lpszSMTPMessage;

    //
    // lpszName comes from the title of the web page. If the charset of the page
    // is not the same as the one that this version of IE has been localized to
    // then we need to use the MLANG api's to coerce the string into the correct
    // charset
    //
    CHAR szTargetEncoding[ENCODING_STRLEN];
    MLLoadStringA(IDS_TARGET_CHARSET_EMAIL, szTargetEncoding, ARRAYSIZE(szTargetEncoding));

    //
    // Allocate buffer for new name. This is a conversion from one dbcs charset 
    // to another so size shouldn't but to be safe use *2 multiplier.
    //
    UINT uiSize = lstrlenA(lpszName) * 2;
    LPSTR lpszNewName = (LPSTR) MemAlloc(LMEM_FIXED, uiSize * sizeof(CHAR));

    if (lpszNewName)
    {
        //
        // Note check for S_OK as will return S_FALSE if there is no appropriate
        // translation installed on this machine
        //
        if (S_OK == TranslateCharset(lpszName, lpszNewName, uiSize, lpszCharSet,
                                     szTargetEncoding))
        {
            //
            // if translation occurred alias new name to old name
            //
            SAFELOCALFREE(lpszName);
            lpszName = lpszNewName;
        }
        else
        {
            SAFELOCALFREE(lpszNewName); // don't need newname after all
        }
    }
    
    //
    // If we are requested to HTML mail and we successfully built the html
    //
    if (!(fSendHTMLEmail &&
         BuildHTMLMessage(lpszEmailAddress, lpszName, lpszURL, &lpszSMTPMessage,
                          lpszTitle, lpszAbstract, lpszCharSet)))
    {
        //
        // If sending a simple notification or BuildHTMLMessage failed
        // force fSendHTMLEmail to false and build simple smtp message
        //
        fSendHTMLEmail = FALSE;
        BuildSMTPMessage(lpszName, lpszURL, &lpszSMTPMessage, lpszTitle, lpszAbstract);
    }

    //
    // Disable MAPI for now
    //
    //-----------------------------------------
    //BUG BUG: If this is enabled then wsprintf on message text should be changed to FormatMessage
    //         due to 1KB message limit.
    #if 0
    
    //
    // First try and load a full mapi implementation (exchange or outlook)
    //
    HMODULE hmodMail = LoadNormalMapi();
    if (hmodMail != NULL)
    {
        //
        // Get mapi function entry points
        //
        LPMAPISENDMAIL pfnSendMail;
        LPMAPILOGON pfnLogon;
        MapiMessage   message;
        MapiRecipDesc recipient;

        pfnSendMail = (LPMAPISENDMAIL)GetProcAddress(hmodMail, "MAPISendMail");
        pfnLogon    = (LPMAPILOGON)   GetProcAddress(hmodMail, "MAPILogon");
        if (pfnSendMail != NULL && pfnLogon != NULL)
        {
            //
            // Logon to mapi provider
            //
            LHANDLE hSession = 0;
            LPSTR lpszProfileName = NULL;  // for now logon on with NULL
            LPSTR lpszPassword = NULL;     // credentials
            ULONG ul = pfnLogon(0, lpszProfileName, lpszPassword, 0, 0, &hSession);
            if (ul != SUCCESS_SUCCESS)
            {
                DBG_OUTPUT_MAPI_ERROR(ul);
                //
                // Fall through to try other mail delivery methods
                //
            }
            else
            {
                TCHAR szSubject[INTERNET_MAX_URL_LENGTH];

                //
                // Fill in the text, subject line and recipient in mapi message
                //
                BuildMAPIMessage(lpszName, lpszURL, lpszEmailAddress, 
                                 &message, &recipient,
                                 szSubject, lpszSMTPMessage, szTitle, szAbstract);

                //
                // Actually send the message via MAPI with no UI
                //
                ul = pfnSendMail(0, 0, &message, 0, 0);
                if (ul != SUCCESS_SUCCESS)
                {
                    DBG_OUTPUT_MAPI_ERROR(ul);
                }
                else
                {
                    FreeLibrary(hmodMail);
                    return S_OK;
                }
            }
        }
        FreeLibrary(hmodMail);
    }
    #endif

    //
    // Send message to given address and from given address
    //
    if (lpszSMTPMessage)
    {
        b = SMTPSendMessage(lpszSMTPServer,  lpszEmailAddress, 
                            lpszEmailAddress, lpszSMTPMessage);

        MemFree(lpszSMTPMessage);

    }
    else
    {
        b = FALSE;
    }

    if (b)
        return S_OK;

    return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////
//
// Helper function to send email
//
//////////////////////////////////////////////////////////////////////////
HRESULT SendEmailFromItem(ISubscriptionItem *pItem)
{
    HRESULT hr = E_FAIL;
    LPSTR pszURL = NULL;
    LPSTR pszName = NULL;
    LPSTR pszTitle = NULL;
    LPSTR pszAbstract = NULL;
    LPSTR pszCharSet = NULL;
    
    // Get the Email URL to send.  Fall back to the download URL.
    ReadAnsiSTR(pItem, c_szPropEmailURL, &pszURL);
    if (!pszURL)
        ReadAnsiSTR(pItem, c_szPropURL, &pszURL);
    ASSERT(pszURL);

    // Get the friendly name.  Fall back to the download URL.
    ReadAnsiSTR(pItem, c_szPropName, &pszName);
    ASSERT(pszName);
    if (!pszName)
        ReadAnsiSTR(pItem, c_szPropURL, &pszName);

    // Get Email Title and Abstract if flag is set.
    DWORD dwEmailFlags = 0;
    ReadDWORD(pItem, c_szPropEmailFlags, &dwEmailFlags);
    if (dwEmailFlags & MAILAGENT_FLAG_CUSTOM_MSG)
    {
        ReadAnsiSTR(pItem, c_szPropEmailTitle, &pszTitle);
        ASSERT(pszTitle);
        ReadAnsiSTR(pItem, c_szPropEmailAbstract, &pszAbstract);
        ASSERT(pszAbstract);
    }

    //
    // Get the charset in the notification
    //
    ReadAnsiSTR(pItem, c_szPropCharSet, &pszCharSet);

    // Get Email address and SMTP server
    TCHAR tszBuf[MAX_PATH];
    CHAR szEmailAddress[MAX_PATH];
    CHAR szSMTPServer[MAX_PATH];
    
    ReadDefaultEmail(tszBuf, ARRAYSIZE(tszBuf));
    SHTCharToAnsi(tszBuf, szEmailAddress, ARRAYSIZE(szEmailAddress));
    ReadDefaultSMTPServer(tszBuf, ARRAYSIZE(tszBuf));
    SHTCharToAnsi(tszBuf, szSMTPServer, ARRAYSIZE(szSMTPServer));

    // Send the email
    if (pszURL && pszName)
    {
        //
        // Check if HTML Mail notification is enabled or disabled thru the registry
        //
        BOOL fSendHTMLEmail = FALSE;

        if (!ReadRegValue(HKEY_CURRENT_USER, c_szRegKey,
            TEXT("EnableHTMLMailNotification"),
            &fSendHTMLEmail, sizeof(DWORD)))
        {
            fSendHTMLEmail = TRUE; // default to on if not read from registry
        }

        // Now make sure our crawling mode supports HTML mail. We don't
        // want to send HTML if we're in check-for-change only.
        DWORD dwTemp = 0;
        ReadDWORD(pItem, c_szPropCrawlChangesOnly, &dwTemp);
        if (dwTemp != 0)
        {
            fSendHTMLEmail = FALSE;
        }
        // else, leave fSendHTMLEmail in its reg-based setting.

        hr = NotifyViaEMail(pszURL, szEmailAddress, szSMTPServer, 
                            pszName, pszTitle, pszAbstract, pszCharSet,
                            fSendHTMLEmail );
    }

    // Clean up.
    SAFELOCALFREE(pszURL);
    SAFELOCALFREE(pszName);
    SAFELOCALFREE(pszTitle);
    SAFELOCALFREE(pszAbstract);
    return hr;
}


