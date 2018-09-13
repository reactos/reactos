/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    readhtml.cxx

Abstract:

    Contains functions for returning files and directory listings (gopher and
    FTP) as HTML documents

    Contents:
        ReadHtmlUrlData
        QueryHtmlDataAvailable
        (MakeHtmlDirEntry)

Author:

    Richard L Firth (rfirth) 23-Jun-1995

Environment:

    Win32 user-mode

Revision History:

    23-Jun-1995 rfirth
        Created

--*/

#include <wininetp.h>

//
// private manifests
//

//
// HTML encapsulation strings - we generate HTML documents using the following
// strings. Although we don't need carriage-return, line-feed at the end of each
// line, we add them anyway since it allows View Source and Save As commands in
// the viewer to create human-sensible documents
//

//
// HTML_DOCUMENT_HEADER - every HTML document should have a header. It must have
// a title. This string defines the header, title and start of the document body
//

#define HTML_DOCUMENT_HEADER        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\r\n" \
                                    "<HTML>\r\n" \
                                    "<HEAD>\r\n" \
                                    "<TITLE>%s</TITLE>\r\n" \
                                    "</HEAD>\r\n" \
                                    "<BODY>\r\n" \
                                    "<H2>%s</H2>\r\n"

#ifdef EXTENDED_ERROR_HTML

//
// HTML_ERROR_DOCUMENT_HEADER - error variant of standard document header
//

#define HTML_ERROR_DOCUMENT_HEADER  "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\r\n" \
                                    "<HTML>\r\n" \
                                    "<HEAD>\r\n" \
                                    "<TITLE>%s</TITLE>\r\n" \
                                    "</HEAD>\r\n" \
                                    "<BODY>\r\n" \
                                    "<H2>%s</H2>\r\n" \
                                    "<HR>\r\n"

#endif

//
// HTML_DOCUMENT_PREFORMAT - directory entries are preformatted to stop the HTML
// viewer putting its own interpretation on the listing
//

#define HTML_DOCUMENT_PREFORMAT     "<PRE>\r\n"

//
// HTML_DOCUMENT_END_PREFORMAT - this is required after the last directory entry
// to allow the viewer to resume HTML formatting
//

#define HTML_DOCUMENT_END_PREFORMAT "</PRE>\r\n"

//
// HTML_HORIZONTAL_RULE - we separate each part of the document with a horizontal
// rule line
//

#define HTML_HORIZONTAL_RULE        "<HR>\r\n"

//
// HTML_FTP_WELCOME_START - for FTP directories, we display the FTP welcome
// message
//

#define HTML_FTP_WELCOME_START      HTML_HORIZONTAL_RULE \
                                    "<H4><PRE>\r\n"

//
// HTML_FTP_WELCOME_END - finish off at end of FTP welcome message
//

#define HTML_FTP_WELCOME_END        "</PRE></H4>\r\n"

//
// HTML_DOCUMENT_DIR_START - when creating a directory listing document, we
// follow the header with a Horizontal Rule (<HR>)
//

#define HTML_DOCUMENT_DIR_START     HTML_HORIZONTAL_RULE \
                                    HTML_DOCUMENT_PREFORMAT

//
// HTML_DOCUMENT_FTP_DIR_START - same as HTML_DOCUMENT_DIR_START, but used to
// add an additional "back up one level" if FTP and not the root
//

#define HTML_DOCUMENT_FTP_DIR_START HTML_HORIZONTAL_RULE \
                                    "%s" \
                                    HTML_DOCUMENT_PREFORMAT

//
// HTML_DOCUMENT_DIR_END - string that appears at the end of the preformatted
// directory list, but before the end of the document
//

#define HTML_DOCUMENT_DIR_END       HTML_DOCUMENT_END_PREFORMAT \
                                    HTML_HORIZONTAL_RULE

//
// HTML_ISINDEX - string that causes browser to display search form
//
#define HTML_ISINDEX                "<ISINDEX>"

//
// HTML_DOCUMENT_FOOTER - this goes at the end of every HTML document we create
//

#define HTML_DOCUMENT_FOOTER        "</BODY>\r\n" \
                                    "</HTML>\r\n"

//
// HTML_DOCUMENT_DIR_ENTRY - each directory entry is formatted using the
// following string. Directories are bold
//

#define HTML_DOCUMENT_DIR_ENTRY     "%s %s <A HREF=\"%s\"><B>%s</B></A>\r\n"

//
// HTML_DOCUMENT_FILE_ENTRY - same as HTML_DOCUMENT_DIR_ENTRY, except link is
// not bold
//

#define HTML_DOCUMENT_FILE_ENTRY    "%s %s <A HREF=\"%s\">%s</A>\r\n"

#define SCRATCH_PAD_SIZE            1024

#define FTP_WELCOME_INTRO           "230-"
#define FTP_WELCOME_INTRO_LENGTH    (sizeof(FTP_WELCOME_INTRO) - 1)

#define MAX_FMT_BUF 200 // size of buffer for loading title format string resource
char szDir[32];
char szSearch[32];

//
// private prototypes
//

PRIVATE
DWORD
MakeHtmlDirEntry(
    IN HINTERNET_HANDLE_TYPE HandleType,
    IN LPSTR PathPrefix,
    IN LPVOID DirBuffer,
    IN LPBYTE EntryBuffer,
    IN OUT LPDWORD BufferLength,
    OUT LPSTR* DirEntry
    );

//
// functions
//


BOOL
ReadHtmlUrlData(
    IN HINTERNET hInternet,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwBytesReturned
    )

/*++

Routine Description:

    Converts an FTP or gopher file or directory listing to a HTML document. This
    function is a wrapper for InternetReadFile() and InternetFindNext(), and so
    returns BOOL: the error code proper is set by the appropriate API

Arguments:

    hInternet           - handle of file or find object

    lpBuffer            - pointer to caller's buffer

    dwBufferLength      - size of lpBuffer on input

    lpdwBytesReturned   - pointer to returned number of bytes read into lpBuffer

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER((DBG_INET,
                Bool,
                "ReadHtmlUrlData",
                "%#x, %#x, %d, %#x",
                hInternet,
                lpBuffer,
                dwBufferLength,
                lpdwBytesReturned
                ));

    DWORD error;
    HINTERNET_HANDLE_TYPE handleType;
    BOOL success;
    LPSTR url;
    DWORD charsCopied;
    LPBYTE buffer;
    BYTE dirBuffer[max(sizeof(WIN32_FIND_DATA), sizeof(GOPHER_FIND_DATA))];
    BOOL done;
    HTML_STATE htmlState;
    HTML_STATE previousHtmlState;
    BYTE scratchPad[SCRATCH_PAD_SIZE];
    BOOL dataCopied;
    LPSTR lastDirEntry;
    LPSTR urlCopy;

    //
    // initialize variables in case we quit early
    //

    urlCopy = NULL;

    //
    // retrieve some information from the file/find handle - the handle type,
    // URL string, current HTML document state and previous failed dir entry
    //

    error = RGetHandleType(hInternet, &handleType);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    error = RGetUrl(hInternet, &url);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    error = RGetHtmlState(hInternet, &htmlState);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    error = RGetDirEntry(hInternet, &lastDirEntry);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // initialize variables for loop
    //

    *lpdwBytesReturned = dwBufferLength;
    charsCopied = 0;
    buffer = (LPBYTE)lpBuffer;
    done = FALSE;
    success = TRUE;
    previousHtmlState = htmlState;
    dataCopied = FALSE;

    //
    // first, maybe we already have data available from a previous
    // QueryDataAvailable operation
    //

#ifdef EXTENDED_ERROR_HTML
    INET_ASSERT((handleType == TypeFtpFindHandleHtml)
                || (handleType == TypeFtpFileHandleHtml)
                || (handleType == TypeGopherFindHandleHtml));
#else
    INET_ASSERT((handleType == TypeFtpFindHandleHtml)
                || (handleType == TypeGopherFindHandleHtml));
#endif

    if (handleType == TypeFtpFindHandleHtml) {

        FTP_FIND_HANDLE_OBJECT * pObject = (FTP_FIND_HANDLE_OBJECT *)hInternet;

        if (pObject->HaveQueryData()) {
            dwBufferLength -= pObject->CopyQueriedData(lpBuffer, dwBufferLength);
            goto quit;
        }
    }
#ifdef EXTENDED_ERROR_HTML
    else if (handleType == TypeFtpFileHandleHtml) {

        FTP_ERROR_HANDLE_OBJECT * pObject = (FTP_ERROR_HANDLE_OBJECT *)hInternet;

        if (pObject->HaveQueryData()) {
            dwBufferLength -= pObject->CopyQueriedData(lpBuffer, dwBufferLength);
            goto quit;
        }
    }
#endif
    else
    {
        GOPHER_FIND_HANDLE_OBJECT * pObject = (GOPHER_FIND_HANDLE_OBJECT *)hInternet;

        if (pObject->HaveQueryData()) {
            dwBufferLength -= pObject->CopyQueriedData(lpBuffer, dwBufferLength);
            goto quit;
        }
    }

    //
    // loop round, writing as much data as we are able to the caller's buffer
    //

    do {
        switch (htmlState) {
        case HTML_STATE_START:

            //
            // if we are dealing with files then we go straight to copying the
            // file contents - we don't encapsulate files in HTML
            //

#ifdef EXTENDED_ERROR_HTML
            if (handleType == TypeFtpFileHandleHtml) {

                //
                // FTP file handle is used for FTP errors
                //

                charsCopied = (DWORD)wsprintf((LPSTR)scratchPad,
                                              HTML_ERROR_DOCUMENT_HEADER,
                                              "FTP Error",
                                              "FTP Error"
                                              );
                if (charsCopied > dwBufferLength) {

                    //
                    // caller's buffer not large enough to fit header - clean up
                    // and allow the caller to retry
                    //

                    success = FALSE;
                    error = ERROR_INSUFFICIENT_BUFFER;
                    done = TRUE;
                } else {

                    //
                    // copy the header to the caller's buffer
                    //

                    memcpy(buffer, scratchPad, charsCopied);
                    dataCopied = TRUE;
                }
                htmlState = HTML_STATE_ERROR_BODY;
            } else if (handleType == TypeGopherFileHandleHtml)
#else
            if ((handleType == TypeFtpFileHandleHtml)
            || (handleType == TypeGopherFileHandleHtml))
#endif
            {
                htmlState = HTML_STATE_BODY;
            }
            else
            {
                htmlState = (HTML_STATE)((int)htmlState + 1);
            }
            break;

        case HTML_STATE_HEADER: {

            //
            // crack the URL for the info we need (i) for the header (ii) for
            // FTP file paths
            //

            DWORD urlLength;
            LPSTR host;
            DWORD hostLength;
            char title[MAX_FMT_BUF + INTERNET_MAX_PATH_LENGTH + 1];
            char hostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];

            //
            // first, make a copy of the URL, reserving 2 extra characters in
            // case this is an FTP request and we need to add a root directory
            // path
            //

            urlLength = strlen(url);
            urlCopy = NEW_MEMORY(urlLength + 2, char);
            if (urlCopy == NULL) {
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
            memcpy(urlCopy, url, urlLength + 1);

            //
            // get the start of the URL path and the host name. CrackUrl() will
            // destroy url but leave urlCopy intact
            //

            error = CrackUrl(urlCopy,
                             0,     // dwUrlLength
                             FALSE, // decode the url-path
                             NULL,  // we don't care about the scheme
                             NULL,  // or scheme name
                             NULL,
                             &host, // we want the host name (for title)
                             &hostLength,
                             NULL,  // or the port
                             NULL,  // or the user name
                             NULL,
                             NULL,  // or the password
                             NULL,
                             &url,  // we want the url-path
                             &urlLength,
                             NULL,
                             NULL,
                             NULL
                             );
            if (error != ERROR_SUCCESS) {

                //
                // handle still has previous URL pointer, but it will be deleted
                // when the handle is closed
                //

                goto quit;
            }

            //
            // ensure the host name is zero-terminated
            //

            hostLength = min(hostLength, sizeof(hostName) - 1);
            memcpy(hostName, host, hostLength);
            hostName[hostLength] = '\0';

            //
            // same for the URL-path
            //

            url[urlLength] = '\0';

            //
            // create the header/title
            //

            char szFmt[MAX_FMT_BUF];
            szFmt[0] = 0; // guard against LoadString failing (why would it?)

            if (handleType == TypeGopherFindHandleHtml) {

                GOPHER_FIND_HANDLE_OBJECT* pGopherFind =
                    (GOPHER_FIND_HANDLE_OBJECT *) hInternet;

                switch (pGopherFind->GetFixedType()) {

                    case GOPHER_TYPE_CSO:
                        LoadString (GlobalDllHandle, IDS_GOPHER_CSO,
                            szFmt, sizeof(szFmt));
                        wsprintf (title, szFmt, hostName);
                        break;

                    case GOPHER_TYPE_INDEX_SERVER:
                        LoadString (GlobalDllHandle, IDS_GOPHER_INDEX,
                            szFmt, sizeof(szFmt));
                        wsprintf (title, szFmt, hostName);
                        break;

                    default:
                        if ((*url == '\0') || (memcmp(url, "/", 2) == 0)) {
                            LoadString (GlobalDllHandle, IDS_GOPHER_ROOT,
                                szFmt, sizeof(szFmt));
                            wsprintf(title, szFmt, hostName);
                        } else {
                            LoadString (GlobalDllHandle, IDS_GOPHER_DIR,
                                szFmt, sizeof(szFmt));
                            wsprintf(title, szFmt, hostName);
                        }
                }

            } else if (handleType == TypeFtpFindHandleHtml) {
                if ((*url == '\0') || (memcmp(url, "/", 2) == 0)) {
                    LoadString (GlobalDllHandle, IDS_FTP_ROOT,
                        szFmt, sizeof(szFmt));
                    wsprintf(title, szFmt, hostName);
                } else {
                    LoadString (GlobalDllHandle, IDS_FTP_DIR,
                        szFmt, sizeof(szFmt));
                    wsprintf(title, szFmt, url, hostName);
                }
            } else {
                title[0] = '\0';
            }
            charsCopied = (DWORD)wsprintf((LPSTR)scratchPad,
                                          HTML_DOCUMENT_HEADER,
                                          title,
                                          title
                                          );
            if (charsCopied > dwBufferLength) {

                //
                // caller's buffer not large enough to fit header - clean up
                // and allow the caller to retry
                //

                success = FALSE;
                error = ERROR_INSUFFICIENT_BUFFER;
                done = TRUE;
            } else {

                //
                // copy the header to the caller's buffer
                //

                memcpy(buffer, scratchPad, charsCopied);
                dataCopied = TRUE;

                //
                // if the URL contains a NULL path then point it at the FTP
                // root. Likewise, end any and all directory paths with '/'
                //

                if (handleType == TypeFtpFindHandleHtml) {
                    if (urlLength == 0) {
                        *url = '/';
                        ++urlLength;
                    } else if ((url[urlLength - 1] != '/')
                    && (url[urlLength - 1] != '\\')) {
                        url[urlLength++] = '/';
                    }
                    url[urlLength] = '\0';
                } else {
                    url = NULL;
                }

                //
                // free the URL in the handle object if gopher, else (FTP) set
                // it to be the path part of the URL. The previous string will
                // be deleted, and a new one allocated
                //

                RSetUrl(hInternet, url);

                //
                // we can now start on the dir header
                //

                htmlState = (HTML_STATE)((int)htmlState + 1);
            }
            break;
        }

        case HTML_STATE_WELCOME: {

            LPSTR lastInfo;
            DWORD lastInfoLength;
            INTERNET_CONNECT_HANDLE_OBJECT * pConnect;
            BOOL freeLastResponseInfo;

            if (  handleType == TypeGopherFindHandleHtml
               && ((GOPHER_FIND_HANDLE_OBJECT *) hInternet)->GetFixedType()) {

                //
                // BUGBUG - wimp out on CSO searches for now.
                //

                if (((GOPHER_FIND_HANDLE_OBJECT *) hInternet)->GetFixedType()
                         == GOPHER_TYPE_CSO) {
                    success = TRUE;
                    dataCopied = FALSE;
                    htmlState = HTML_STATE_FOOTER;
                    break;
                }


                charsCopied = sizeof(HTML_ISINDEX);
                if (dwBufferLength < charsCopied) {

                    success = FALSE;

                } else {

                    memcpy (buffer, HTML_ISINDEX, charsCopied);
                    dataCopied = TRUE;
                    success = TRUE;
                    htmlState = HTML_STATE_FOOTER;
                }
                break;
            }

            pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)
                            ((HANDLE_OBJECT *)hInternet)->GetParent();
            lastInfo = pConnect->GetLastResponseInfo(&lastInfoLength);

            //
            // if not an FTP find operation then we don't check for any welcome
            // message, or other text from the server. Just go to the next state
            //

            if ((handleType == TypeFtpFindHandleHtml) && (lastInfo != NULL)) {

                //
                // find the welcome message. This starts with "230-" and can
                // continue over multiple lines, each of which may start with
                // "230-", or each may start with a line beginning only with a
                // space
                //

                LPSTR p = strstr(lastInfo, FTP_WELCOME_INTRO);

                if (p != NULL) {

                    //
                    // first, copy the separator
                    //

                    LPBYTE pBuffer = buffer;
                    DWORD bufferLeft = dwBufferLength;

                    if (bufferLeft >= (sizeof(HTML_FTP_WELCOME_START) - 1)) {
                        memcpy(pBuffer,
                               HTML_FTP_WELCOME_START,
                               sizeof(HTML_FTP_WELCOME_START) - 1
                               );
                        pBuffer += sizeof(HTML_FTP_WELCOME_START) - 1;
                        bufferLeft -= sizeof(HTML_FTP_WELCOME_START) - 1;
                        charsCopied = sizeof(HTML_FTP_WELCOME_START) - 1;
                    } else {

                        //
                        // caller's buffer not large enough to fit header -
                        // clean up and allow the caller to retry
                        //

                        success = FALSE;
                        error = ERROR_INSUFFICIENT_BUFFER;
                        done = TRUE;
                        freeLastResponseInfo = FALSE;
                        goto quit_welcome;
                    }

                    //
                    // then copy each welcome message line, dropping the "230-"
                    // from each
                    //

                    do {
                        if (!strncmp(p, FTP_WELCOME_INTRO, FTP_WELCOME_INTRO_LENGTH)) {
                            p += FTP_WELCOME_INTRO_LENGTH;
                        } else if (*p != ' ') {

                            //
                            // line doesn't start with "230-" or a space. We're
                            // done
                            //

                            break;
                        }

                        LPSTR lastChar;
                        DWORD len;

                        lastChar = strchr(p, '\n');
                        if (lastChar != NULL) {
                            len = (DWORD)(lastChar - p) + 1;
                        } else {
                            len = strlen(p);
                        }

                        if (bufferLeft >= len) {
                            memcpy(pBuffer, p, len);
                            pBuffer += len;
                            p += len;
                            bufferLeft -= len;
                            charsCopied += len;

                            //
                            // find start of next line, or end of buffer, in
                            // case there are multiple line terminators
                            //

                            while ((*p == '\r') || (*p == '\n')) {
                                ++p;
                            }
                        } else {

                            //
                            // caller's buffer not large enough to fit header -
                            // clean up and allow the caller to retry
                            //

                            success = FALSE;
                            error = ERROR_INSUFFICIENT_BUFFER;
                            done = TRUE;
                            freeLastResponseInfo = FALSE;
                            break;
                        }
                    } while (TRUE);

                    //
                    // finish off
                    //

                    if (bufferLeft >= (sizeof(HTML_FTP_WELCOME_END) - 1)) {
                        memcpy(pBuffer,
                               HTML_FTP_WELCOME_END,
                               sizeof(HTML_FTP_WELCOME_END) - 1
                               );
                        pBuffer += sizeof(HTML_FTP_WELCOME_END) - 1;
                        bufferLeft -= sizeof(HTML_FTP_WELCOME_END) - 1;
                        charsCopied += sizeof(HTML_FTP_WELCOME_END) - 1;

                        //
                        // successfully completed welcome message part
                        //

                        dataCopied = TRUE;
                        freeLastResponseInfo = TRUE;
                        htmlState = (HTML_STATE)((int)htmlState + 1);
                    } else {

                        //
                        // caller's buffer not large enough to fit header -
                        // clean up and allow the caller to retry
                        //

                        success = FALSE;
                        error = ERROR_INSUFFICIENT_BUFFER;
                        done = TRUE;
                        freeLastResponseInfo = FALSE;
                    }
                } else {

                    //
                    // last response info, but no welcome text. Continue
                    //

                    htmlState = (HTML_STATE)((int)htmlState + 1);
                    freeLastResponseInfo = TRUE;
                    charsCopied = 0;
                }
            } else {

                //
                // no info from server. Continue
                //

                htmlState = (HTML_STATE)((int)htmlState + 1);
                freeLastResponseInfo = FALSE;
                charsCopied = 0;
            }

quit_welcome:

            if (freeLastResponseInfo) {
                pConnect->FreeLastResponseInfo();
            }
            break;
        }

        case HTML_STATE_DIR_HEADER:

            //
            // copy the directory listing introduction. N.B. this is mainly here
            // from the time when we had directory listings AND files being HTML
            // encapsulated. This is no longer true, so I could change things
            // somewhat
            //

            //
            // if FTP directory AND not root directory, display link to higher
            // level
            //

            DWORD cbTotal;
            char szUp[64];
            LPSTR lpszHtml;

            szUp[0] = 0;

            if (handleType == TypeFtpFindHandleHtml) {
                lpszHtml = HTML_DOCUMENT_FTP_DIR_START;
                cbTotal = sizeof(HTML_DOCUMENT_FTP_DIR_START);
                if ((*url != '\0') && (memcmp(url, "/", 2) != 0)) {
                    cbTotal += LoadString(GlobalDllHandle,
                                          IDS_FTP_UPLEVEL,
                                          szUp,
                                          sizeof(szUp)
                                          );
                }
            } else {
                lpszHtml = HTML_DOCUMENT_DIR_START;
                cbTotal = sizeof(HTML_DOCUMENT_DIR_START);
            }
            if (dwBufferLength >= cbTotal) {
                charsCopied = wsprintf((char *)buffer, lpszHtml, szUp);
                dataCopied = TRUE;
                htmlState = (HTML_STATE)((int)htmlState + 1);
            } else {
                error = ERROR_INSUFFICIENT_BUFFER;
                success = FALSE;
            }
            break;

        case HTML_STATE_BODY:

            //
            // if we already have a formatted dir entry from last time (it
            // wouldn't fit in the buffer), then copy it now
            //

            if (lastDirEntry != NULL) {
                charsCopied = strlen(lastDirEntry);
                if (dwBufferLength >= charsCopied) {
                    memcpy(buffer, lastDirEntry, charsCopied);

                    //
                    // we no longer require the string
                    //

                    RSetDirEntry(hInternet, NULL);
                    lastDirEntry = NULL;
                    success = TRUE;
                } else {

                    //
                    // still not enough room
                    //

                    error = ERROR_INSUFFICIENT_BUFFER;
                    success = FALSE;
                    done = TRUE;
                }

                //
                // in both cases fall out of the switch()
                //

                break;
            }

            //
            // read the next part of the HTML document
            //

            switch (handleType) {
            case TypeFtpFindHandleHtml:
                success = FtpFindNextFileA(hInternet,
                                           (LPWIN32_FIND_DATA)dirBuffer
                                           );
                goto create_dir_entry;

            case TypeGopherFindHandleHtml:

                //
                // Check if we simply can return fixed text for a search
                //

                success = GopherFindNextA(hInternet,
                                          (LPGOPHER_FIND_DATA)dirBuffer
                                          );

                //
                // directory listing - get the next directory entry, convert
                // to HTML and write to the caller's buffer. If there's not
                // enough space, return an error
                //

create_dir_entry:

                if (success) {
                    charsCopied = dwBufferLength;
                    error = MakeHtmlDirEntry(handleType,
                                             url,
                                             dirBuffer,
                                             buffer,
                                             &charsCopied,
                                             &lastDirEntry
                                             );
                    if (error != ERROR_SUCCESS) {

                        //
                        // if MakeHtmlDirEntry() created a copy of the directory
                        // entry then store it in the handle object for next
                        // time
                        //

                        if (lastDirEntry != NULL) {
                            RSetDirEntry(hInternet, lastDirEntry);

                            //
                            // no longer need our copy of the string -
                            // RSetLastDirEntry() also makes a copy and adds it
                            // to the handle object
                            //

                            DEL_STRING(lastDirEntry);
                        }

                        //
                        // probably out of buffer space - we're done
                        //

                        success = FALSE;
                        done = TRUE;
                    } else {
                        dataCopied = TRUE;
                    }
                } else if (GetLastError() == ERROR_NO_MORE_FILES) {

                    //
                    // change the error to success - ReadFile() doesn't return
                    // ERROR_NO_MORE_FILES
                    //

                    SetLastError(ERROR_SUCCESS);

                    //
                    // reached the end of the directory listing - time to
                    // add the footer
                    //

                    charsCopied = 0;
                    success = TRUE;
                    htmlState = (HTML_STATE)((int)htmlState + 1);
                }
                break;

            case TypeFtpFileHandleHtml:
            case TypeGopherFileHandleHtml:

                //
                // BUGBUG - this path never taken because we do not encapsulate
                //          files?
                //

                //
                // file data - just read the next chunk
                //

                success = InternetReadFile(hInternet,
                                           (LPVOID)buffer,
                                           dwBufferLength,
                                           &charsCopied
                                           );
                if (success) {
                    if (charsCopied == 0) {

                        //
                        // read all of file - we're done (no HTML to add for
                        // files)
                        //

                        htmlState = HTML_STATE_END;
                    } else {
                        buffer += charsCopied;
                        dwBufferLength -= charsCopied;
                        dataCopied = TRUE;
                    }
                }
                break;
            }
            break;

        case HTML_STATE_DIR_FOOTER:
            if (dwBufferLength >= sizeof(HTML_DOCUMENT_DIR_END) - 1) {
                memcpy(buffer,
                       HTML_DOCUMENT_DIR_END,
                       sizeof(HTML_DOCUMENT_DIR_END) - 1
                       );
                charsCopied = sizeof(HTML_DOCUMENT_DIR_END) - 1;
                dataCopied = TRUE;
                htmlState = (HTML_STATE)((int)htmlState + 1);
            } else {
                error = ERROR_INSUFFICIENT_BUFFER;
                success = FALSE;
            }
            break;

        case HTML_STATE_FOOTER:

            //
            // copy the HTML document footer - don't copy the end-of-string
            // character ('\0')
            //

            if (dwBufferLength >= sizeof(HTML_DOCUMENT_FOOTER) - 1) {
                memcpy(buffer,
                       HTML_DOCUMENT_FOOTER,
                       sizeof(HTML_DOCUMENT_FOOTER) - 1
                       );
                charsCopied = sizeof(HTML_DOCUMENT_FOOTER) - 1;

                //
                // the document is done
                //

                htmlState = (HTML_STATE)((int)htmlState + 1);
                dataCopied = TRUE;
                done = TRUE;
            } else if (charsCopied==0) {
                error = ERROR_INSUFFICIENT_BUFFER;
                success = FALSE;
            } else {
                // We're out of buffer space. but we've got everything else
                // so we'll just leave
                done = TRUE;
            }
            break;

        case HTML_STATE_END:
            done = TRUE;
            break;

#ifdef EXTENDED_ERROR_HTML
        case HTML_STATE_ERROR_BODY:
            htmlState = HTML_STATE_FOOTER;
            break;
#endif

        default:

            INET_ASSERT(FALSE);

            break;
        }

        if (success) {

            //
            // update the buffer pointer and remaining length by the number of
            // bytes read this time round the loop
            //

            buffer += charsCopied;
            dwBufferLength -= charsCopied;
        }
    } while ((dwBufferLength != 0) && success && !done);

    //
    // if we succeeded, update the amount of data returned, else set the last
    // error, which may be different from the last error set by the underlying
    // API (if any were called)
    //

quit:

    //
    // remember the current state
    //

    if (htmlState != previousHtmlState) {
        RSetHtmlState(hInternet, htmlState);
    }

    //
    // if we quit because we ran out of buffer then only return an error if we
    // didn't copy *any* data
    //

    if ((error == ERROR_INSUFFICIENT_BUFFER) && dataCopied) {
        error = ERROR_SUCCESS;
        success = TRUE;
    }

    //
    // if we succeeded then return the amount of data read, else reset the last
    // error
    //

    if (error == ERROR_SUCCESS) {
        *lpdwBytesReturned -= dwBufferLength;
    } else {
        *lpdwBytesReturned = 0;
        SetLastError(error);
        success = FALSE;

        DEBUG_ERROR(INET, error);

    }

    //
    // clean up
    //

    if (urlCopy != NULL) {
        DEL(urlCopy);
    }

    //
    // return API-level success or failure indication
    //

    DEBUG_LEAVE(success);

    return success;
}


DWORD
QueryHtmlDataAvailable(
    IN HINTERNET hInternet,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )

/*++

Routine Description:

    This function is called when the app is querying available data for a cooked
    (HTML) find handle. Because we have to cook the data before we can determine
    how much there is, we need to allocate a buffer and cook the data there

Arguments:

    hInternet                   - MAPPED address of handle object

    lpdwNumberOfBytesAvailable  - pointer to returned bytes available

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                    Can only handle TypeFtpFindHandleHtml and
                    TypeGopherFindHandleHtml

                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory allocating HTML buffer

--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "QueryHtmlDataAvailable",
                "%#x, %#x",
                hInternet,
                lpdwNumberOfBytesAvailable
                ));

    DWORD error;

    switch (((HANDLE_OBJECT *)hInternet)->GetHandleType()) {
    case TypeFtpFindHandleHtml:
        error = ((FTP_FIND_HANDLE_OBJECT *)hInternet)->
                    QueryHtmlDataAvailable(lpdwNumberOfBytesAvailable);
        break;

#ifdef EXTENDED_ERROR_HTML
    case TypeFtpFileHandleHtml:
        error = ((FTP_ERROR_HANDLE_OBJECT *)hInternet)->
                    QueryHtmlDataAvailable(lpdwNumberOfBytesAvailable);
        break;
#endif

    case TypeGopherFindHandleHtml:
        error = ((GOPHER_FIND_HANDLE_OBJECT *)hInternet)->
                    QueryHtmlDataAvailable(lpdwNumberOfBytesAvailable);
        break;

    default:
        error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}

//
// private functions
//


PRIVATE
DWORD
MakeHtmlDirEntry(
    IN HINTERNET_HANDLE_TYPE HandleType,
    IN LPSTR PathPrefix,
    IN LPVOID DirBuffer,
    IN LPBYTE EntryBuffer,
    IN OUT LPDWORD BufferLength,
    OUT LPSTR* DirEntry
    )

/*++

Routine Description:

    Creates a single-line directory entry for a HTML document. The line is
    formatted thus (e.g.):

    "   12,345,678 Fri Jun 23 95  4:43pm pagefile.sys                   "
    "    Directory Fri Jun 23 95 10:00am foobar.directory               "
    "                                    unknown.filesize               "

Arguments:

    HandleType      - type of the handle - gopher or FTP (find + HTML)

    PathPrefix      - string used to build (FTP) absolute paths

    DirBuffer       - pointer to buffer containing GOPHER_FIND_DATA or
                      WIN32_FIND_DATA

    EntryBuffer     - pointer to buffer where HTML dir entry will be written

    BufferLength    - IN: number of bytes available in EntryBuffer
                      OUT: number of bytes written to EntryBuffer

    DirEntry        - if we couldn't copy the directory entry to the HTML buffer
                      then we return a pointer to a copy of the formatted
                      directory entry. The caller should remember this and use
                      it the next time the function is called, before getting
                      the next dir entry

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    EntryBuffer is not large enough to hold this entry

--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "MakeHtmlDirEntry",
                "%s (%d), %q, %#x, %#x, %#x [%d], %#x",
                InternetMapHandleType(HandleType),
                HandleType,
                PathPrefix,
                DirBuffer,
                EntryBuffer,
                BufferLength,
                *BufferLength,
                DirEntry
                ));

    BOOL isDir;
    BOOL isSearch;
    LPSTR entryName;
    DWORD entrySize;
    FILETIME entryTime;
    SYSTEMTIME systemTime;
    char timeBuf[80];
    char sizeBuf[15];   // 4,294,967,295
    char entryBuf[sizeof(HTML_DOCUMENT_DIR_ENTRY) + INTERNET_MAX_PATH_LENGTH];
    char urlBuf[INTERNET_MAX_URL_LENGTH];
    LPSTR url;
    LPSTR pSizeDir;
    DWORD nPrinted;
    DWORD error;
    BOOL haveTimeAndSize;
    DWORD urlLength;

    //
    // ensure we are dealing with the correct handle type
    //

    INET_ASSERT((HandleType == TypeFtpFindHandleHtml)
                || (HandleType == TypeGopherFindHandleHtml));

    //
    // get the common information we will use to create a directory entry line
    //

    if (HandleType == TypeFtpFindHandleHtml) {
        isDir = ((LPWIN32_FIND_DATA)DirBuffer)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        isSearch = FALSE;
        entryName = ((LPWIN32_FIND_DATA)DirBuffer)->cFileName;
        entryTime = ((LPWIN32_FIND_DATA)DirBuffer)->ftLastWriteTime;
        entrySize = ((LPWIN32_FIND_DATA)DirBuffer)->nFileSizeLow;
        haveTimeAndSize = TRUE;

        DWORD slen = strlen(PathPrefix);
        DWORD len = min(slen, sizeof(urlBuf) - 1);

        memcpy(urlBuf, PathPrefix, len);
        slen = strlen(entryName);
        slen = min(slen, (sizeof(urlBuf) - 1) - len);
        memcpy(&urlBuf[len], entryName, slen);
        len += slen;
        if (isDir && (len < sizeof(urlBuf) - 1)) {
            urlBuf[len] = '/';
            ++len;
        }
        urlBuf[len] = '\0';
        url = urlBuf;
    } else {
        isDir = IS_GOPHER_DIRECTORY(((LPGOPHER_FIND_DATA)DirBuffer)->GopherType);
        isSearch = IS_GOPHER_INDEX_SERVER(((LPGOPHER_FIND_DATA)DirBuffer)->GopherType);
        entryName = ((LPGOPHER_FIND_DATA)DirBuffer)->DisplayString;
        GopherLocatorToUrl(((LPGOPHER_FIND_DATA)DirBuffer)->Locator,
                           urlBuf,
                           sizeof(urlBuf),
                           &urlLength
                           );
        url = urlBuf;
        if (IS_GOPHER_PLUS(((LPGOPHER_FIND_DATA)DirBuffer)->GopherType)) {
            haveTimeAndSize = TRUE;
            entryTime = ((LPGOPHER_FIND_DATA)DirBuffer)->LastModificationTime;
            entrySize = ((LPGOPHER_FIND_DATA)DirBuffer)->SizeLow;
        } else {
            haveTimeAndSize = FALSE;
            entryTime.dwLowDateTime = 0;
            entryTime.dwHighDateTime = 0;
            entrySize = 0;
        }
    }

    //
    // if we have a non-zero entry time AND we can convert it to a system time
    // then create an internationalized time string
    //

    if ((entryTime.dwLowDateTime != 0)
    && (entryTime.dwHighDateTime != 0)
    && FileTimeToSystemTime(&entryTime, &systemTime)) {

        int n;

        //
        // BUGBUG - if either of these fail for any reason, we should fill the
        //          string with the requisite-t-t-t-t-t number of spaces
        //

        n = GetDateFormat(0,    // lcid
                          0,    // dwFlags
                          &systemTime,
                          "MM/dd/yyyy ",
                          timeBuf,
                          sizeof(timeBuf)
                          );
        if (n > 0) {

            //
            // number of characters written to string becomes index in string
            // at which to write time string
            //

            --n;
        }
        GetTimeFormat(0,    // lcid
                      TIME_NOSECONDS,
                      &systemTime,
                      "hh:mmtt",
                      &timeBuf[n],
                      sizeof(timeBuf) - n
                      );

        //
        // convert any non-ANSI characters to the OEM charset
        //

        CharToOem(timeBuf, timeBuf);
    } else {
        memcpy(timeBuf, "                ", 17);
    }

    //
    // convert the entry size to a human-readable form
    //

    if (isDir) {

        //
        // directories show up as DOS-style <DIR> entries, except we use the
        // full "Directory" text
        //

        //
        // BUGBUG - needs to be in a resource. Also, if we are adding images
        //          <IMG> then we don't need to specify dir - it will be obvious
        //          from the display (but what about text-only? what about
        //          viewers that don't care to/can't load the image?)
        //

        if (!szDir[0]) {
            LoadString(GlobalDllHandle, IDS_TAG_DIRECTORY, szDir, sizeof(szDir));
        }
        pSizeDir = szDir;
    } else if (isSearch) {

        //
        // if this is a gopher item and it is an index server (7) then indicate
        // the fact using this string
        //

        if (!szSearch[0]) {
            LoadString(GlobalDllHandle, IDS_TAG_SEARCH, szSearch, sizeof(szSearch));
        }
        pSizeDir = szSearch;
    } else if (haveTimeAndSize) {
        pSizeDir = NiceNum(sizeBuf, entrySize, 14);
    } else {
        pSizeDir = "              ";
    }

    //
    // now create the HTML directory entry
    //

    //
    // BUGBUG - at this point we need to call the MIME function to retrieve the
    //          URL for the image file that goes with this file type
    //

    nPrinted = wsprintf(entryBuf,
                        isDir
                            ? HTML_DOCUMENT_DIR_ENTRY
                            : HTML_DOCUMENT_FILE_ENTRY,
                        timeBuf,
                        pSizeDir,
                        url,
                        entryName
                        );
    if (nPrinted <= *BufferLength) {
        memcpy(EntryBuffer, entryBuf, nPrinted);
        *BufferLength = nPrinted;
        error = ERROR_SUCCESS;
    } else {

        //
        // there is not enough space in the buffer to store this formatted
        // directory entry. So since we've gone to the trouble of creating
        // the string, we make a copy of it and return it to the caller. If
        // NEW_STRING fails, then we will lose this directory entry, but it's
        // not really a problem. (The alternative would be to return e.g.
        // ERROR_NOT_ENOUGH_MEMORY and probably fail the entire request)
        //

        DEBUG_PRINT(INET,
                    WARNING,
                    ("failed to copy %q\n",
                    entryBuf
                    ));

        *DirEntry = NEW_STRING(entryBuf);

        if (*DirEntry == NULL) {

            DEBUG_PRINT(INET,
                        WARNING,
                        ("failed to make copy of %q!\n",
                        entryBuf
                        ));

        }
        error = ERROR_INSUFFICIENT_BUFFER;
    }

    DEBUG_LEAVE(error);

    return error;
}
