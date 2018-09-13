/*
 * intshcut.h - Internet Shortcut interface definitions.
 *
 * Copyright (c) 1995-1996, Microsoft Corporation.  All rights reserved.
 */

#ifndef __INTSHCUT_H__
#define __INTSHCUT_H__

/* Headers
 **********/

#include <isguids.h>

#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */

/* Constants
 ************/

/* Define API decoration for direct import of DLL functions. */

#ifdef _INTSHCUT_
#define INTSHCUTAPI
#else
#define INTSHCUTAPI                 DECLSPEC_IMPORT
#endif

/* HRESULTs */

//
// MessageId: E_FLAGS
//
// MessageText:
//
//  The flag combination is invalid.
//
#define E_FLAGS                     MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x1000)

//
// MessageId: IS_E_EXEC_FAILED
//
// MessageText:
//
//  The URL's protocol handler failed to run.
//
#define IS_E_EXEC_FAILED            MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x2002)

//
// MessageId: URL_E_INVALID_SYNTAX
//
// MessageText:
//
//  The URL's syntax is invalid.
//
#define URL_E_INVALID_SYNTAX        MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x1001)

//
// MessageId: URL_E_UNREGISTERED_PROTOCOL
//
// MessageText:
//
//  The URL's protocol does not have a registered protocol handler.
//
#define URL_E_UNREGISTERED_PROTOCOL MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x1002)

/* Interfaces
 *************/

//
// Input flags for IUniformResourceLocator::SetURL().
//
typedef enum iurl_seturl_flags
{
   IURL_SETURL_FL_GUESS_PROTOCOL        = 0x0001,     // Guess protocol if missing
   IURL_SETURL_FL_USE_DEFAULT_PROTOCOL  = 0x0002,     // Use default protocol if missing
}
IURL_SETURL_FLAGS;

//
// Input flags for IUniformResourceLocator()::InvokeCommand().
//
typedef enum iurl_invokecommand_flags
{
   IURL_INVOKECOMMAND_FL_ALLOW_UI                  = 0x0001,
   IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB          = 0x0002,    // Ignore pcszVerb
}
IURL_INVOKECOMMAND_FLAGS;

//
// Command info for IUniformResourceLocator::InvokeCommand().
//

typedef struct urlinvokecommandinfoA
{
   DWORD  dwcbSize;          // Size of structure
   DWORD  dwFlags;           // Bit field of IURL_INVOKECOMMAND_FLAGS
   HWND   hwndParent;        // Parent window.  Valid only if IURL_INVOKECOMMAND_FL_ALLOW_UI is set.
   LPCSTR pcszVerb;          // Verb to invoke.  Ignored if IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB is set.
}
URLINVOKECOMMANDINFOA;
typedef URLINVOKECOMMANDINFOA *PURLINVOKECOMMANDINFOA;
typedef const URLINVOKECOMMANDINFOA CURLINVOKECOMMANDINFOA;
typedef const URLINVOKECOMMANDINFOA *PCURLINVOKECOMMANDINFOA;

typedef struct urlinvokecommandinfoW
{
   DWORD   dwcbSize;          // Size of structure
   DWORD   dwFlags;           // Bit field of IURL_INVOKECOMMAND_FLAGS
   HWND    hwndParent;        // Parent window.  Valid only if IURL_INVOKECOMMAND_FL_ALLOW_UI is set.
   LPCWSTR pcszVerb;          // Verb to invoke.  Ignored if IURL_INVOKECOMMAND_FL_USE_DEFAULT_VERB is set.
}
URLINVOKECOMMANDINFOW;
typedef URLINVOKECOMMANDINFOW *PURLINVOKECOMMANDINFOW;
typedef const URLINVOKECOMMANDINFOW CURLINVOKECOMMANDINFOW;
typedef const URLINVOKECOMMANDINFOW *PCURLINVOKECOMMANDINFOW;

#ifdef UNICODE
#define URLINVOKECOMMANDINFO            URLINVOKECOMMANDINFOW
#define PURLINVOKECOMMANDINFO           PURLINVOKECOMMANDINFOW
#define CURLINVOKECOMMANDINFO           CURLINVOKECOMMANDINFOW
#define PCURLINVOKECOMMANDINFO          PCURLINVOKECOMMANDINFOW
#else
#define URLINVOKECOMMANDINFO            URLINVOKECOMMANDINFOA
#define PURLINVOKECOMMANDINFO           PURLINVOKECOMMANDINFOA
#define CURLINVOKECOMMANDINFO           CURLINVOKECOMMANDINFOA
#define PCURLINVOKECOMMANDINFO          PCURLINVOKECOMMANDINFOA
#endif


#undef  INTERFACE
#define INTERFACE IUniformResourceLocatorA

DECLARE_INTERFACE_(IUniformResourceLocatorA, IUnknown)
{
   /* IUnknown methods */

   STDMETHOD(QueryInterface)(THIS_
                             REFIID riid,
                             PVOID *ppvObject) PURE;
   STDMETHOD_(ULONG, AddRef)(THIS) PURE;
   STDMETHOD_(ULONG, Release)(THIS) PURE;

   /* IUniformResourceLocator methods */

   STDMETHOD(SetURL)(THIS_
                     LPCSTR pcszURL,
                     DWORD dwInFlags) PURE;

   STDMETHOD(GetURL)(THIS_
                     LPSTR *ppszURL) PURE;

   STDMETHOD(InvokeCommand)(THIS_
                            PURLINVOKECOMMANDINFOA purlici) PURE;
};

#undef  INTERFACE
#define INTERFACE IUniformResourceLocatorW

DECLARE_INTERFACE_(IUniformResourceLocatorW, IUnknown)
{
   /* IUnknown methods */

   STDMETHOD(QueryInterface)(THIS_
                             REFIID riid,
                             PVOID *ppvObject) PURE;
   STDMETHOD_(ULONG, AddRef)(THIS) PURE;
   STDMETHOD_(ULONG, Release)(THIS) PURE;

   /* IUniformResourceLocator methods */

   STDMETHOD(SetURL)(THIS_
                     LPCWSTR pcszURL,
                     DWORD dwInFlags) PURE;

   STDMETHOD(GetURL)(THIS_
                     LPWSTR *ppszURL) PURE;

   STDMETHOD(InvokeCommand)(THIS_
                            PURLINVOKECOMMANDINFOW purlici) PURE;
};

#ifdef UNICODE
#define IUniformResourceLocator         IUniformResourceLocatorW
#define IUniformResourceLocatorVtbl     IUniformResourceLocatorWVtbl
#else
#define IUniformResourceLocator         IUniformResourceLocatorA
#define IUniformResourceLocatorVtbl     IUniformResourceLocatorAVtbl
#endif

typedef IUniformResourceLocator *PIUniformResourceLocator;
typedef const IUniformResourceLocator CIUniformResourceLocator;
typedef const IUniformResourceLocator *PCIUniformResourceLocator;

/* Prototypes
 *************/

//
// Input flags for TranslateURL().
//
typedef enum translateurl_in_flags
{
   TRANSLATEURL_FL_GUESS_PROTOCOL         = 0x0001,     // Guess protocol if missing
   TRANSLATEURL_FL_USE_DEFAULT_PROTOCOL   = 0x0002,     // Use default protocol if missing
}
TRANSLATEURL_IN_FLAGS;

//
//   TranslateURL().  This function applies common translations to a URL
//  string, creating a new URL string.
//
//   This function does not perform any validation on the syntax of the input
//  URL string.  A successful return value does not indicate that the input
//  or output URL strings are valid URLS.
//
//   The function returns S_OK if the URL string is translated successfully
//  and *ppszTranslatedURL points to the translated URL string.  S_FALSE
//  is returned if the URL string did not require translation.  An error
//  code is returned if an error occurs.
//
//  Parameters:
//   pcszURL -- A pointer to the URL string to be translated.
//   dwInFlags -- A bit field of TRANSLATEURL_IN_FLAGS.
//   ppszTranslatedURL -- A pointer to the newly created, translated URL
//     string, if any.  *ppszTranslatedURL is only valid if S_OK is returned.
//     If valid, *ppszTranslatedURL should be freed by calling LocalFree().
//     *ppszTranslatedURL is NULL on error.
//

INTSHCUTAPI HRESULT WINAPI TranslateURLA(PCSTR pcszURL,
                                         DWORD dwInFlags,
                                         PSTR *ppszTranslatedURL);
INTSHCUTAPI HRESULT WINAPI TranslateURLW(PCWSTR pcszURL,
                                         DWORD dwInFlags,
                                         PWSTR *ppszTranslatedURL);
#ifdef UNICODE
#define TranslateURL             TranslateURLW
#else
#define TranslateURL             TranslateURLA
#endif   /* UNICODE */

//
// Input flags for URLAssociationDialog().
//
typedef enum urlassociationdialog_in_flags
{
   URLASSOCDLG_FL_USE_DEFAULT_NAME        = 0x0001,
   URLASSOCDLG_FL_REGISTER_ASSOC          = 0x0002,
}
URLASSOCIATIONDIALOG_IN_FLAGS;

//
//   URLAssocationDialog().  This function invokes the unregistered URL
//  protocol dialog box, providing a standard ui for choosing the handler for
//  an unregistered URL protocol.
//
//  The functions returns S_OK if the application is registered with the
//  URL protocol.  S_FALSE is returned if nothing is registered (a one-time
//  execution via the selected application is requested).
//
//  Parameters:
//   hwndParent -- A handle to the window to be used as the parent
//   dwInFlags -- A bit field of URLASSOCIATIONDIALOG_IN_FLAGS.  The
//                flags are:
//
//                  URLASSOCDLG_FL_USE_DEFAULT_NAME: Use the default Internet
//                   Shortcut file name.  Ignore pcszFile.
//
//                  URLASSOCDLG_FL_REGISTER_ASSOC: The application
//                   selected is to be registered as the handler for URLs
//                   of pcszURL's protocol.  An application is only
//                   registered if this flag is set, and the user indicates
//                   that a persistent association is to be made.
//
//   pcszFile -- The name of the Internet Shortcut file whose URL's protocol
//               requires a protocol handler.  Before a verb, like "open", can
//               be invoked on an Internet Shortcut, a protocol handler must be
//               registered for its URL protocol.  If
//               URLASSOCDLG_FL_USE_DEFAULT_NAME is set in dwInFlags, pcszFile
//               is ignored, and a default Internet Shortcut file name is used.
//               pcszFile is only used for ui.
//   pcszURL -- The URL whose unregistered protocol requires a handler.
//   pszAppBuf -- A buffer to be filled in on success with the path
//                of the application selected by the user.  pszAppBuf's
//                buffer is filled in with the empty string on failure.
//   ucAppBufLen -- The length of pszAppBuf's buffer in characters.
//

INTSHCUTAPI HRESULT WINAPI URLAssociationDialogA(HWND hwndParent,
                                                 DWORD dwInFlags,
                                                 PCSTR pcszFile,
                                                 PCSTR pcszURL,
                                                 PSTR pszAppBuf,
                                                 UINT ucAppBufLen);
INTSHCUTAPI HRESULT WINAPI URLAssociationDialogW(HWND hwndParent,
                                                 DWORD dwInFlags,
                                                 PCWSTR pcszFile,
                                                 PCWSTR pcszURL,
                                                 PWSTR pszAppBuf,
                                                 UINT ucAppBufLen);
#ifdef UNICODE
#define URLAssociationDialog     URLAssociationDialogW
#else
#define URLAssociationDialog     URLAssociationDialogA
#endif  /* UNICODE */

//
// Input flags for MIMEAssocationDialog().
//
typedef enum mimeassociationdialog_in_flags
{
   MIMEASSOCDLG_FL_REGISTER_ASSOC         = 0x0001,
}
MIMEASSOCIATIONDIALOG_IN_FLAGS;

//
//   MIMEAssociationDialog().  Invokes the unregistered MIME content
//  type dialog box.
//
//   This function does not perform any validation on the syntax of the
//  input content type string.  A successful return value does not indicate
//  that the input MIME content type string is a valid content type.
//
//   The function returns S_OK if the MIME content type is associated
//  with the extension.  The extension is associated as the default
//  extension for the content type.  S_FALSE is returned if nothing is
//  registered.  Otherwise, the function returns one of the following
//  errors:
//
//  E_ABORT -- The user cancelled the operation.
//  E_FLAGS -- The flag combination passed in dwFlags is invalid.
//  E_OUTOFMEMORY -- Not enough memory to complete the operation.
//  E_POINTER -- One of the input pointers is invalid.
//
//  Parameters:
//   hwndParent -- A handle to the window to be used as the parent
//                 window of any posted child windows.
//   dwInFlags -- A bit field of MIMEASSOCIATIONDIALOG_IN_FLAGS.  The
//                flags are:
//
//              MIMEASSOCDLG_FL_REGISTER_ASSOC: If set, the application
//               selected is to be registered as the handler for files of
//               the given MIME type.  If clear, no association is to be
//               registered.  An application is only registered if this
//               flag is set, and the user indicates that a persistent
//               association is to be made.  Registration is only possible
//               if pcszFile contains an extension.
//
//   pcszFile -- A pointer to a string indicating the name of the file
//               containing data of pcszMIMEContentType's content type.
//   pcszMIMEContentType -- A pointer to a string indicating the content
//                          type for which an application is sought.
//   pszAppBuf -- A buffer to be filled in on success with the path of
//                the application selected by the user.  pszAppBuf's buffer
//                is filled in with the empty string on failure.
//   ucAppBufLen -- The length of pszAppBuf's buffer in characters.
//

INTSHCUTAPI HRESULT WINAPI MIMEAssociationDialogA(HWND hwndParent,
                                                  DWORD dwInFlags,
                                                  PCSTR pcszFile,
                                                  PCSTR pcszMIMEContentType,
                                                  PSTR pszAppBuf,
                                                  UINT ucAppBufLen);
INTSHCUTAPI HRESULT WINAPI MIMEAssociationDialogW(HWND hwndParent,
                                                  DWORD dwInFlags,
                                                  PCWSTR pcszFile,
                                                  PCWSTR pcszMIMEContentType,
                                                  PWSTR pszAppBuf,
                                                  UINT ucAppBufLen);
#ifdef UNICODE
#define MIMEAssociationDialog    MIMEAssociationDialogW
#else
#define MIMEAssociationDialog    MIMEAssociationDialogA
#endif  /* UNICODE */

//
//   InetIsOffline().  This function determines if the user wants to be
//  "offline" (get all information from the cache).  The dwFlags must be
//  0.
//
//   The function returns TRUE to indicate that the local system is not
//  currently connected to the Internet.  The function returns FALSE to
//  indicate that either the local system is connected to the Internet,
//  or no attempt has yet been made to connect the local system to the
//  Internet.  Applications that wish to support an off-line mode should
//  do so if InetIsOffline() returns TRUE.
//
//   Off-line mode begins when the user has been prompted to dial-in to
//  an Internet providor, but canceled the attempt.
//
INTSHCUTAPI
BOOL
WINAPI
InetIsOffline(
    DWORD dwFlags);

#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */

#endif   /* ! __INTSHCUT_H__ */

