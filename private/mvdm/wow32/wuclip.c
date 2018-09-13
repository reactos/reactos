/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUCLIP.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  WOW Clipboard functionality designed and developed by ChandanC
 *
--*/


#include "precomp.h"
#pragma hdrstop

struct _CBFORMATS  ClipboardFormats;

DLLENTRYPOINTS  OleStringConversion[WOW_OLESTRINGCONVERSION_COUNT] =
                                    {"ConvertObjDescriptor", NULL};



UINT CFOLEObjectDescriptor;
UINT CFOLELinkSrcDescriptor;


MODNAME(wuclip.c);


/*++
    BOOL ChangeClipboardChain(<hwnd>, <hwndNext>)
    HWND <hwnd>;
    HWND <hwndNext>;

    The %ChangeClipboardChain% function removes the window specified by the
    <hwnd> parameter from the chain of clipboard viewers and makes the window
    specified by the <hwndNext> parameter the descendant of the <hwnd>
    parameter's ancestor in the chain.

    <hwnd>
        Identifies the window that is to be removed from the chain. The handle
        must previously have been passed to the SetClipboardViewer function.

    <hwndNext>
        Identifies the window that follows <hwnd> in the clipboard-viewer
        chain (this is the handle returned by the %SetClipboardViewer% function,
        unless the sequence was changed in response to a WM_CHANGECBCHAIN
        message).

    The return value specifies the status of the <hwnd> window. It is TRUE if
    the window is found and removed. Otherwise, it is FALSE.
--*/

ULONG FASTCALL WU32ChangeClipboardChain(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCHANGECLIPBOARDCHAIN16 parg16;

    GETARGPTR(pFrame, sizeof(CHANGECLIPBOARDCHAIN16), parg16);

    ul = GETBOOL16(ChangeClipboardChain(
            HWND32(parg16->f1),
            HWND32(parg16->f2)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL CloseClipboard(VOID)

    The %CloseClipboard% function closes the clipboard. The %CloseClipboard%
    function should be called when a window has finished examining or changing
    the clipboard. It lets other applications access the clipboard.

    This function has no parameters.

    The return value specifies whether the clipboard is closed. It is TRUE if
    the clipboard is closed. Otherwise, it is FALSE.
--*/

ULONG FASTCALL WU32CloseClipboard(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETBOOL16(CloseClipboard());

    RETURN(ul);
}


/*++
    No REF header file
--*/

ULONG FASTCALL WU32CountClipboardFormats(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETINT16(CountClipboardFormats());

    RETURN(ul);
}


/*++
    BOOL EmptyClipboard(VOID)

    The %EmptyClipboard% function empties the clipboard and frees handles to
    data in the clipboard. It then assigns ownership of the clipboard to the
    window that currently has the clipboard open.

    This function has no parameters.

    The return value specifies the status of the clipboard. It is TRUE if the
    clipboard is emptied. It is FALSE if an error occurs.

    The clipboard must be open when the %EmptyClipboard% function is called.
--*/

ULONG FASTCALL WU32EmptyClipboard(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETBOOL16(EmptyClipboard());

    W32EmptyClipboard ();

    RETURN(ul);
}


/*++
    WORD EnumClipboardFormats(<wFormat>)
    WORD <wFormat>;

    The %EnumClipboardFormats% function enumerates the formats found in a list
    of available formats that belong to the clipboard. On each call to this
    function, the <wFormat> parameter specifies a known available format, and
    the function returns the format that appears next in the list. The first
    format in the list can be retrieved by setting <wFormat> to zero.

    <wFormat>
        Specifies a known format.

    The return value specifies the next known clipboard data format. It is zero
    if <wFormat> specifies the last format in the list of available formats. It
    is zero if the clipboard is not open.

    Before it enumerates the formats by using the %EnumClipboardFormats%
    function, an application must open the clipboard by using the
    %OpenClipboard% function.

    The order that an application uses for putting alternative formats for the
    same data into the clipboard is the same order that the enumerator uses when
    returning them to the pasting application. The pasting application should
    use the first format enumerated that it can handle. This gives the donor a
    chance to recommend formats that involve the least loss of data.
--*/

ULONG FASTCALL WU32EnumClipboardFormats(PVDMFRAME pFrame)
{
    ULONG ul;
    register PENUMCLIPBOARDFORMATS16 parg16;

    GETARGPTR(pFrame, sizeof(ENUMCLIPBOARDFORMATS16), parg16);

    ul = GETWORD16(EnumClipboardFormats(
            WORD32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HANDLE GetClipboardData(<wFormat>)
    WORD <wFormat>;

    The %GetClipboardData% function retrieves data from the clipboard in the
    format given by the <wFormat> parameter. The clipboard must have been opened
    previously.

    <wFormat>
        Specifies a data format. For a description of the data formats, see the
        SetClipboardData function, later in this chapter.

    The return value identifies the memory block that contains the data from the
    clipboard. The handle type depends on the type of data specified by the
    <wFormat> parameter. It is NULL if there is an error.

    The available formats can be enumerated in advance by using the
    %EnumClipboardFormats% function.

    The data handle returned by %GetClipboardData% is controlled by the
    clipboard, not by the application. The application should copy the data
    immediately, instead of relying on the data handle for long-term use. The
    application should not free the data handle or leave it locked.

    Windows supports two formats for text, CF_TEXT and CF_OEMTEXT. CF_TEXT is
    the default Windows text clipboard format, while Windows uses the CF_OEMTEXT
    format for text in non-Windows applications. If you call %GetClipboardData%
    to retrieve data in one text format and the other text format is the only
    available text format, Windows automatically converts the text to the
    requested format before supplying it to your application.

    If the clipboard contains data in the CF_PALETTE (logical color palette)
    format, the application should assume that any other data in the clipboard
    is realized against that logical palette.
--*/

ULONG FASTCALL WU32GetClipboardData(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    HANDLE  hMem32, hMeta32 = 0;
    HMEM16  hMem16=0, hMeta16 = 0;
    VPVOID  vp;
    LPBYTE  lpMem32;
    LPBYTE  lpMem16;
    int     cb;
    register PGETCLIPBOARDDATA16 parg16;

    GETARGPTR(pFrame, sizeof(GETCLIPBOARDDATA16), parg16);

    LOGDEBUG(6, ("WOW::WUICBGetClipboardData(): CF_FORMAT is %04x\n", parg16->f1));

    switch (parg16->f1) {

        // This is intentional to let it thru to the "case statements".
        // ChandanC 5/11/92.

        default:
            if ((parg16->f1 == CFOLEObjectDescriptor) || (parg16->f1 == CFOLELinkSrcDescriptor)) {
                hMem32 = GetClipboardData(WORD32(parg16->f1));
                if (hMem32) {
                    hMem16 = (HMEM16) W32ConvertObjDescriptor(hMem32, CFOLE_UNICODE_TO_ANSI);
                }
                WU32ICBStoreHandle(parg16->f1, hMem16);
                break;
            }

        case CF_DIB:
        case CF_TEXT:
        case CF_DSPTEXT:
        case CF_SYLK:
        case CF_DIF:
        case CF_TIFF:
        case CF_OEMTEXT:
        case CF_PENDATA:
        case CF_RIFF:
        case CF_WAVE:
        case CF_OWNERDISPLAY:
            hMem16 = WU32ICBGetHandle(parg16->f1);
            if (!hMem16) {
                hMem32 = GetClipboardData(WORD32(parg16->f1));

                if (hMem16 = WU32ICBGetHandle(parg16->f1)) {

                    //
                    // We couldn't find the hMem16 using WU32ICBGetHandle
                    // before we called Win32 GetClipboardData, but we can
                    // now, so that means it was cut/copied from a task in
                    // this WOW using delayed rendering, so that the actual
                    // non-NULL hMem16 wasn't SetClipboardData until we
                    // just called GetClipboardData.  Since we now have
                    // a valid cached copy of the data in 16-bit land,
                    // we can just return that.
                    //

                    break;
                }

                if (hMem32) {
                    lpMem32 = GlobalLock(hMem32);
                    cb = GlobalSize(hMem32);
		    vp = GlobalAllocLock16(GMEM_MOVEABLE | GMEM_DDESHARE, cb, &hMem16);
		    // 16-bit memory may have moved - refresh flat pointers
		    FREEARGPTR(parg16);
		    FREEVDMPTR(pFrame);
		    GETFRAMEPTR(((PTD)CURRENTPTD())->vpStack, pFrame);
		    GETARGPTR(pFrame, sizeof(GETCLIPBOARDDATA16), parg16);
                    if (vp) {
                        GETMISCPTR(vp, lpMem16);
                        RtlCopyMemory(lpMem16, lpMem32, cb);
                        GlobalUnlock16(hMem16);
                        FLUSHVDMPTR(vp, cb, lpMem16);
                        FREEMISCPTR(lpMem16);
                    }
                    GlobalUnlock(hMem32);
                }

                WU32ICBStoreHandle(parg16->f1, hMem16);
            }
            break;

        case CF_HDROP:
            // This is the case when app is retrieving cf_hdrop from the 
            // clipboard, thus we will convert the dropfiles structure
            // from 32 to 16-bit one
            hMem16 = WU32ICBGetHandle(parg16->f1);
            if (!hMem16) {
                hMem32 = GetClipboardData(WORD32(parg16->f1));
                if (hMem32) {
                    hMem16 = CopyDropFilesFrom32(hMem32);
                }
                WU32ICBStoreHandle(parg16->f1, hMem16);
            }
            break;

        case CF_DSPBITMAP:
        case CF_BITMAP:
            hMem16 = GETHBITMAP16(GetClipboardData(WORD32(parg16->f1)));
            break;

        case CF_PALETTE:
            hMem16 = GETHPALETTE16(GetClipboardData(WORD32(parg16->f1)));
            break;

        case CF_DSPMETAFILEPICT:
        case CF_METAFILEPICT:
            hMem16 = WU32ICBGetHandle(parg16->f1);
            if (!(hMem16)) {
                hMem32 = GetClipboardData(WORD32(parg16->f1));

                if (hMem16 = WU32ICBGetHandle(parg16->f1)) {

                    //
                    // We couldn't find the hMem16 using WU32ICBGetHandle
                    // before we called Win32 GetClipboardData, but we can
                    // now, so that means it was cut/copied from a task in
                    // this WOW using delayed rendering, so that the actual
                    // non-NULL hMem16 wasn't SetClipboardData until we
                    // just called GetClipboardData.  Since we now have
                    // a valid cached copy of the data in 16-bit land,
                    // we can just return that.
                    //

                    break;
                }

                if (hMem32) {
                    lpMem32 = GlobalLock(hMem32);
                    vp = GlobalAllocLock16(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(METAFILEPICT16), &hMem16);
		    // 16-bit memory may have moved - refresh flat pointers
		    FREEARGPTR(parg16);
		    FREEVDMPTR(pFrame);
		    GETFRAMEPTR(((PTD)CURRENTPTD())->vpStack, pFrame);
		    GETARGPTR(pFrame, sizeof(GETCLIPBOARDDATA16), parg16);
		    if (vp) {
                        GETMISCPTR(vp, lpMem16);
                        FixMetafile32To16 ((LPMETAFILEPICT) lpMem32, (LPMETAFILEPICT16) lpMem16);
                        FREEMISCPTR(lpMem16);

                        hMeta32 = ((LPMETAFILEPICT) lpMem32)->hMF;
                        if (hMeta32) {
			    hMeta16 = WinMetaFileFromHMF(hMeta32, FALSE);
			    // 16-bit memory may have moved
			    FREEARGPTR(parg16);
			    FREEVDMPTR(pFrame);
			    GETFRAMEPTR(((PTD)CURRENTPTD())->vpStack, pFrame);
			    GETARGPTR(pFrame, sizeof(GETCLIPBOARDDATA16), parg16);
			}

                        GETMISCPTR(vp, lpMem16);
                        STOREWORD(((LPMETAFILEPICT16) lpMem16)->hMF, hMeta16);
                        GlobalUnlock16(hMem16);
                        FLUSHVDMPTR(vp, sizeof(METAFILEPICT16), lpMem16);
                        FREEMISCPTR(lpMem16);
                    }
                    GlobalUnlock(hMem32);
                }
                WU32ICBStoreHandle(parg16->f1, hMem16);
            }
            break;
    }

    ul = (ULONG) hMem16;

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    int GetClipboardFormatName(<wFormat>, <lpFormatName>, <nMaxCount>)
    WORD <wFormat>;
    LPSTR <lpFormatName>;
    int <nMaxCount>;

    The %GetClipboardFormatName% function retrieves from the clipboard the name
    of the registered format specified by the <wFormat> parameter. The name is
    copied to the buffer pointed to by the <lpFormatName> parameter.

    <wFormat>
        Specifies the type of format to be retrieved. It must not specify any of
        the predefined clipboard formats.

    <lpFormatName>
        Points to the buffer that is to receive the format name.

    <nMaxCount>
        Specifies the maximum length (in bytes) of the string to be copied
        to the buffer. If the actual name is longer, it is truncated.

    The return value specifies the actual length of the string copied to the
    buffer. It is zero if the requested format does not exist or is a predefined
    format.
--*/

ULONG FASTCALL WU32GetClipboardFormatName(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2 = NULL;
    register PGETCLIPBOARDFORMATNAME16 parg16;

    GETARGPTR(pFrame, sizeof(GETCLIPBOARDFORMATNAME16), parg16);
    ALLOCVDMPTR(parg16->f2, parg16->f3, psz2);

    ul = GETINT16(GetClipboardFormatName(
            WORD32(parg16->f1),
            psz2,
            INT32(parg16->f3)
    ));

    FLUSHVDMPTR(parg16->f2, strlen(psz2)+1, psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HWND GetClipboardOwner(VOID)

    The %GetClipboardOwner% function retrieves the window handle of the current
    owner of the clipboard.

    This function has no parameters.

    The return value identifies the window that owns the clipboard. It is NULL
    if the clipboard is not owned.

    The clipboard can still contain data even if the clipboard is not currently
    owned.
--*/

ULONG FASTCALL WU32GetClipboardOwner(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETHWND16(GetClipboardOwner());

    RETURN(ul);
}


/*++
    HWND GetClipboardViewer(VOID)

    The %GetClipboardViewer% function retrieves the window handle of the first
    window in the clipboard-viewer chain.

    This function has no parameters.

    The return value identifies the window currently responsible for displaying
    the clipboard. It is NULL if there is no viewer.
--*/

ULONG FASTCALL WU32GetClipboardViewer(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETHWND16(GetClipboardViewer());

    RETURN(ul);
}


/*++
    int GetPriorityClipboardFormat(<lpPriorityList>, <cEntries>)
    LPWORD <lpPriorityList>;
    int <cEntries>;

    The %GetPriorityClipboardFormat% function returns the first clipboard format
    in a list for which data exist in the clipboard.

    <lpPriorityList>
        Points to an integer array that contains a list of clipboard formats in
        priority order. For a description of the data formats, see the
        SetClipboardData function later in this chapter.

    <cEntries>
        Specifies the number of entries in <lpPriorityList>. This value
        must not be greater than the actual number of entries in the list.

    The return value is the highest priority clipboard format in the list for
    which data exist. If no data exist in the clipboard, this function returns
    NULL. If data exist in the clipboard which did not match any format in the
    list, the return value is -1.
--*/

ULONG FASTCALL WU32GetPriorityClipboardFormat(PVDMFRAME pFrame)
{
    ULONG ul;
    UINT *pu1;
    register PGETPRIORITYCLIPBOARDFORMAT16 parg16;
    INT      BufferT[256]; // comfortably large array


    GETARGPTR(pFrame, sizeof(GETPRIORITYCLIPBOARDFORMAT16), parg16);
    pu1 = STACKORHEAPALLOC(parg16->f2 * sizeof(INT), sizeof(BufferT), BufferT);
    getuintarray16(parg16->f1, parg16->f2, pu1);

    if (pu1) {
        ul = GETINT16(GetPriorityClipboardFormat(
             pu1,
             INT32(parg16->f2)
        ));
    } else {
        ul = (ULONG)-1;
    }

    STACKORHEAPFREE(pu1, BufferT);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL IsClipboardFormatAvailable(<wFormat>)
    WORD <wFormat>;

    The %IsClipboardFormatAvailable% function specifies whether data of a
    certain type exist in the clipboard.

    <wFormat>
        Specifies a registered clipboard format. For information on clipboard
        formats, see the description of the SetClipboardData function, later in
        this chapter.

    The return value specifies the outcome of the function. It is TRUE if data
    having the specified format are present. Otherwise, it is FALSE.

    This function is typically called during processing of the WM_INITMENU or
    WM_INITMENUPOPUP message to determine whether the clipboard contains data
    that the application can paste. If such data are present, the application
    typically enables the Paste command (in its Edit menu).
--*/

ULONG FASTCALL WU32IsClipboardFormatAvailable(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISCLIPBOARDFORMATAVAILABLE16 parg16;

    GETARGPTR(pFrame, sizeof(ISCLIPBOARDFORMATAVAILABLE16), parg16);

    // Hack-a-roo!  PhotoShop 2.5 has a bug in its code for handling large DIB's
    // on the clipboard and will fault if it encounters one.  On WFW, it usually
    // won't encounter one because most apps (in this case alt-Prtscrn button)
    // copy BITMAPS, not DIBS, to the clipboard.  On NT, anytime an app writes
    // a BITMAP to the clipboard it gets converted to a DIB & vice versa-making
    // more clipboard data formats available to inquiring apps.  Unfortunately,
    // Photoshop checks for DIBS before BITMAPS and finds one on Win 
    // Versions >= 4.0.                                             a-craigj

    // if this is a DIB check && the app is PhotoShop...
    if((WORD32(parg16->f1) == CF_DIB) && 
       (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_NODIBSHERE)) {

        // ...see if there is a bitmap format available too
        if(IsClipboardFormatAvailable(CF_BITMAP)) {

            // if so return FALSE which will cause Photoshop to ask for a
            // BITMAP format next
            ul = FALSE;
        }

        // otherwise we'll check for a DIB anyway & hope it's a small one
        else {
            ul = GETBOOL16(IsClipboardFormatAvailable(CF_DIB));
        }
    }

    // no hack path
    else {
        ul = GETBOOL16(IsClipboardFormatAvailable(WORD32(parg16->f1)));
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL OpenClipboard(<hwnd>)
    HWND <hwnd>;

    The %OpenClipboard% function opens the clipboard. Other applications will
    not be able to modify the clipboard until the %CloseClipboard% function is
    called.

    <hwnd>
        Identifies the window to be associated with the open clipboard.

    The return value is TRUE if the clipboard is opened, or FALSE if another
    application or window has the clipboard opened.

    The window specified by the <hwnd> parameter will not become the owner of
    the clipboard until the %EmptyCLipboard% function is called.
--*/

ULONG FASTCALL WU32OpenClipboard(PVDMFRAME pFrame)
{
    ULONG ul;
    register POPENCLIPBOARD16 parg16;

    GETARGPTR(pFrame, sizeof(OPENCLIPBOARD16), parg16);

    ul = GETBOOL16(OpenClipboard(
            HWND32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    WORD RegisterClipboardFormat(<lpFormatName>)
    LPSTR <lpFormatName>;

    The %RegisterClipboardFormat% function registers a new clipboard format
    whose name is pointed to by the <lpFormatName> parameter. The registered
    format can be used in subsequent clipboard functions as a valid format in
    which to render data, and it will appear in the clipboard's list of
    formats.

    <lpFormatName>
        Points to a character string that names the new format. The string must
        be a null-terminated string.

    The return value specifies the newly registered format. If the identical
    format name has been registered before, even by a different application, the
    format's reference count is increased and the same value is returned as when
    the format was originally registered. The return value is zero if the format
    cannot be registered.

    The format value returned by the %RegisterClipboardFormat% function is
    within the range of 0xC000 to 0xFFFF.
--*/

ULONG FASTCALL WU32RegisterClipboardFormat(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    register PREGISTERCLIPBOARDFORMAT16 parg16;

    GETARGPTR(pFrame, sizeof(REGISTERCLIPBOARDFORMAT16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    ul = GETWORD16(RegisterClipboardFormat(
            psz1
    ));

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HANDLE SetClipboardData(<wFormat>, <hData>)
    WORD <wFormat>;
    HANDLE <hData>;

    The %SetClipboardData% function sets the data in the clipboard. The
    application must have called the %OpenClipboard% function before calling
    the %SetClipboardData% function.

    <wFormat>
        Specifies the format of the data. It can be any one of the
        system-defined formats, or a format registered by the
        %RegisterClipboardFormat% function. For a list of system-defined
        formats,

    <hData>
        Identifies the data to be placed into the clipboard. For all formats
        except CF_BITMAP and CF_PALETTE, this parameter must be a handle to
        memory allocated by the %GlobalAlloc% function. For CF_BITMAP format,
        the <hData> parameter is a handle to a bitmap (see %LoadBitmap%). For
        CF_PALETTE format, the <hData> parameter is a handle to a palette (see
        %CreatePalette%).

        If this parameter is NULL, the owner of the clipboard will be sent a
        WM_RENDERFORMAT message when it needs to supply the data.

    The return value is a handle to the data if the function is succesful, or
    NULL if an error occurred.

    If the <hData> parameter contains a handle to memory allocated by the
    %GlobalAlloc% function, the application must not use this handle once it
    has called the %SetClipboardData% function.

    The following list contains the system-defined clipboard formats:

    CF_BITMAP
        The data is a bitmap.

    CF_DIB
        The data is a memory block containing a %BITMAPINFO% structure followed
        by the bitmap data.

    CF_DIF
        The data is in Software Arts' Data Interchange Format.

    CF_DSPBITMAP
        The data is a bitmap representation of a private format. This data is
        displayed in bitmap format in lieu of the privately formatted data.

    CF_DSPMETAFILEPICT
        The data is a metafile representation of a private data format. This
        data is displayed in metafile-picture format in lieu of the privately
        formatted data.

    CF_DSPTEXT
        The data is a textual representation of a private data format. This data
        is displayed in text format in lieu of the privately formatted data.

    CF_METAFILEPICT
        The data is a metafile (see description of the %METAFILEPICT%
        structure).

    CF_OEMTEXT
        The data is an array of text characters in the OEM character set. Each
        line ends with a carriage return/linefeed (CR-LF) combination. A null
        character signals the end of the data.

    CF_OWNERDISPLAY
        The data is in a private format that the clipboard owner must display.

    CF_PALETTE
        The data is a color palette.

    CF_SYLK
        The data is in Microsoft Symbolic Link (SYLK) format.

    CF_TEXT
        The data is an array of text characters. Each line ends with a carriage
        return/linefeed (CR-LF) combination. A null character signals the end of
        the data.

    CF_TIFF
        The data is in Tag Image File Format.

    Private data formats in the range of CF_PRIVATEFIRST to CF_PRIVATELAST are
    not automatically freed when the data is deleted from the clipboard. Data
    handles associated with these formats should be freed upon receiving a
    WM_DESTROYCLIPBOARD message.

    Private data formats in the range of CF_GDIOBJFIRST to CF_GDIOBJLAST will
    be automatically deleted with a call to %DeleteObject% when the data is
    deleted from the clipboard.

    If the Windows clipboard application is running, it will not update its
    window to show the data placed in the clipboard by the %SetClipboardData%
    until after the %CloseClipboard% function is called.

    31-Oct-1990 [ralphw] Miscelanious material, needs to be moved to other
    function descriptions/overviews.

        Whenever an application places data in the clipboard that depends on or
        assumes a color palette, it should also place the palette in the
        clipboard as well.

        If the clipboard contains data in the CF_PALETTE (logical color palette)
        format, the application should assume that any other data in the
        clipboard is realized against that logical palette.

        The clipboard-viewer application (CLIPBRD.EXE) always uses as its
        current palette any object in CF_PALETTE format that is in the clipboard
        when it displays the other formats in the clipboard.

        Windows supports two formats for text, CF_TEXT and CF_OEMTEXT. CF_TEXT
        is the default Windows text clipboard format, while Windows uses the
        CF_OEMTEXT format for text in non-Windows applications. If you call
        %GetClipboardData% to retrieve data in one text format and the other
        text format is the only available text format, Windows automatically
        converts the text to the requested format before supplying it to your
        application.

        An application registers other standard formats, such as Rich Text
        Format (RTF), by name using the %RegisterClipboardFormat% function
        rather than by a symbolic constant. For information on these external
        formats, see the README.TXT file.
--*/

ULONG FASTCALL WU32SetClipboardData(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    HANDLE hMem32 = NULL, hMF32 = NULL;
    HAND16 hMem16, hMeta16 = 0;
    LPBYTE lpMem16, lpMem32;
    INT     cb;
    VPVOID  vp;


    register PSETCLIPBOARDDATA16 parg16;

    GETARGPTR(pFrame, sizeof(SETCLIPBOARDDATA16), parg16);

    LOGDEBUG(6, ("WOW::WUICBSetClipboardData(): CF_FORMAT is %04x\n", parg16->f1));

    switch (parg16->f1) {

        default:
            if ((parg16->f1 == CFOLEObjectDescriptor) || (parg16->f1 == CFOLELinkSrcDescriptor)) {
                if (parg16->f2) {
                    hMem32 = W32ConvertObjDescriptor((HANDLE) parg16->f2, CFOLE_ANSI_TO_UNICODE);
                }
                ul = (ULONG) SetClipboardData(WORD32(parg16->f1), hMem32);
		WU32ICBStoreHandle(parg16->f1, parg16->f2);
                break;
            }

        // It is intentional to let it thru to the "case statements".
        // ChandanC 5/11/92.

        case CF_DIB:
        case CF_TEXT:
        case CF_DSPTEXT:
        case CF_SYLK:
        case CF_DIF:
        case CF_TIFF:
        case CF_OEMTEXT:
        case CF_PENDATA:
        case CF_RIFF:
        case CF_WAVE:
        case CF_OWNERDISPLAY:
            hMem16 = parg16->f2;
            if (hMem16) {
                vp = GlobalLock16(hMem16, &cb);
                if (vp) {
                    GETMISCPTR(vp, lpMem16);
                    hMem32 = Copyh16Toh32 (cb, lpMem16);
                    GlobalUnlock16(hMem16);
                    FREEMISCPTR(lpMem16);
                }
            }

            ul = (ULONG) SetClipboardData(WORD32(parg16->f1), hMem32);

            WU32ICBStoreHandle(parg16->f1, hMem16);
            break;

        case CF_HDROP:
            // support cf_hdrop format by converting the dropfiles structure
            hMem16 = parg16->f2;
            if (hMem16) {
                hMem32 = CopyDropFilesFrom16(hMem16);
            }
            ul = (ULONG)SetClipboardData(WORD32(parg16->f1), hMem32);
            WU32ICBStoreHandle(parg16->f1, hMem16);
            break;

        case CF_DSPBITMAP:
        case CF_BITMAP:
            ul = (ULONG) SetClipboardData(WORD32(parg16->f1),
                        HBITMAP32(parg16->f2));
            break;


        case CF_PALETTE:
            ul = (ULONG) SetClipboardData(WORD32(parg16->f1),
                        HPALETTE32(parg16->f2));
            break;

        case CF_DSPMETAFILEPICT:
        case CF_METAFILEPICT:
            hMem16 = parg16->f2;
            if (hMem16) {
                vp = GlobalLock16(hMem16, &cb);
                if (vp) {
                    GETMISCPTR(vp, lpMem16);
                    hMem32 = WOWGLOBALALLOC(GMEM_DDESHARE,sizeof(METAFILEPICT));
                    WOW32ASSERT(hMem32);
                    if (hMem32) {
                        lpMem32 = GlobalLock(hMem32);
                        ((LPMETAFILEPICT) lpMem32)->mm = FETCHSHORT(((LPMETAFILEPICT16) lpMem16)->mm);
                        ((LPMETAFILEPICT) lpMem32)->xExt = (LONG) FETCHSHORT(((LPMETAFILEPICT16) lpMem16)->xExt);
                        ((LPMETAFILEPICT) lpMem32)->yExt = (LONG) FETCHSHORT(((LPMETAFILEPICT16) lpMem16)->yExt);
                        hMeta16 = FETCHWORD(((LPMETAFILEPICT16) lpMem16)->hMF);
                        if (hMeta16) {
                            hMF32 = (HMETAFILE) HMFFromWinMetaFile(hMeta16, FALSE);
                        }
                        ((LPMETAFILEPICT) lpMem32)->hMF = hMF32;
                        GlobalUnlock(hMem32);
                    }
                    GlobalUnlock16(hMem16);
                    FREEMISCPTR(lpMem16);
                }

            }

            ul = (ULONG) SetClipboardData(WORD32(parg16->f1), hMem32);

            WU32ICBStoreHandle(parg16->f1, hMem16);
            break;

    }

    if (parg16->f2) {
        ul = parg16->f2;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}



/*++
    HWND SetClipboardViewer(<hwnd>)
    HWND <hwnd>;

    The %SetClipboardViewer% function adds the window specified by the <hwnd>
    parameter to the chain of windows that are notified (via the
    WM_DRAWCLIPBOARD message) whenever the contents of the clipboard are
    changed.

    <hwnd>
        Identifies the window to receive clipboard-viewer chain messages.

    The return value identifies the next window in the clipboard-viewer chain.
    This handle should be saved in static memory and used in responding to
    clipboard-viewer chain messages.

    Windows that are part of the clipboard-viewer chain must respond to
    WM_CHANGECBCHAIN, WM_DRAWCLIPBOARD, and WM_DESTROY messages.

    If an application wishes to remove itself from the clipboard-viewer chain,
    it must call the %ChangeClipboardChain% function.
--*/

ULONG FASTCALL WU32SetClipboardViewer(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETCLIPBOARDVIEWER16 parg16;

    GETARGPTR(pFrame, sizeof(SETCLIPBOARDVIEWER16), parg16);

    ul = GETHWND16(SetClipboardViewer(HWND32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


VOID WU32ICBStoreHandle(WORD wFormat, HMEM16 hMem16)
{
    HAND16  h16, hMeta16;
    PCBNODE Temp, Temp1;
    LPBYTE  lpMem16;
    VPVOID  vp;
    int     cb;


    if ((wFormat == CF_METAFILEPICT) || (wFormat == CF_DSPMETAFILEPICT)) {
        if (wFormat == CF_METAFILEPICT) {
            h16 = ClipboardFormats.Pre1[wFormat];
        }
        else {
            h16 = ClipboardFormats.Pre2[3];
        }

        if (h16) {
            vp = GlobalLock16(h16, &cb);
            if (vp) {
                GETMISCPTR(vp, lpMem16);
                hMeta16 = FETCHWORD(((LPMETAFILEPICT16) lpMem16)->hMF);
                GlobalUnlockFree16(GlobalLock16(hMeta16, NULL));
            }
            GlobalUnlockFree16(vp);

            if (wFormat == CF_METAFILEPICT) {
                ClipboardFormats.Pre1[wFormat] = 0;
            }
            else {
                ClipboardFormats.Pre2[3] = 0;
            }
        }
    }

    if ((wFormat >= CF_TEXT ) && (wFormat <= CF_WAVE)) {
        if (ClipboardFormats.Pre1[wFormat]) {
            GlobalUnlockFree16(GlobalLock16(ClipboardFormats.Pre1[wFormat], NULL));
        }
        ClipboardFormats.Pre1[wFormat] = hMem16;
    }
    else if ((wFormat >= CF_OWNERDISPLAY) && (wFormat <= CF_DSPMETAFILEPICT)) {
        wFormat = (wFormat & (WORD) 3);

        if (ClipboardFormats.Pre2[wFormat]) {
            GlobalUnlockFree16(GlobalLock16(ClipboardFormats.Pre2[wFormat], NULL));
        }

        ClipboardFormats.Pre2[wFormat] = hMem16;
    }
    else if (wFormat == CF_HDROP) {

        if (ClipboardFormats.hmem16Drop) {
            GlobalUnlockFree16(GlobalLock16(ClipboardFormats.hmem16Drop, NULL));
        }
        ClipboardFormats.hmem16Drop = hMem16;
    }
    else {
        Temp = ClipboardFormats.NewFormats;
        if (Temp) {
            while ((Temp->Next) && (Temp->Id != wFormat)) {
                Temp = Temp->Next;
            }

            if (Temp->Id == wFormat) {

                // free a previous handle if it exists
                if (Temp->hMem16) {
                    GlobalUnlockFree16(GlobalLock16(Temp->hMem16, NULL));
                }

                Temp->hMem16 = hMem16;
            }
            else {
                Temp1 = (PCBNODE) malloc_w (sizeof(CBNODE));
                if (Temp1) {
                    Temp->Next = Temp1;
                    Temp1->Id = wFormat;
                    Temp1->hMem16 = hMem16;
                    Temp1->Next = NULL;

                    LOGDEBUG(6,("WOW::WU32ICBStoreHandle: Adding a new node for private clipboard data format %04lx\n", wFormat));
                }
            }
        }
        else {
            Temp = (PCBNODE) malloc_w (sizeof(CBNODE));
            if (Temp) {
                ClipboardFormats.NewFormats = Temp;
                Temp->Id = wFormat;
                Temp->hMem16 = hMem16;
                Temp->Next = NULL;

                LOGDEBUG(6,("WOW::WU32ICBStoreHandle: Adding the FIRST node for private clipboard data format %04lx\n", wFormat));
            }
        }
    }
}



HMEM16 WU32ICBGetHandle(WORD wFormat)
{
    HMEM16 hMem16 = 0;
    PCBNODE Temp;

    if ((wFormat >= CF_TEXT) && (wFormat <= CF_WAVE)) {
        hMem16 = ClipboardFormats.Pre1[wFormat];
    }
    else if ((wFormat >= CF_OWNERDISPLAY) && (wFormat <= CF_DSPMETAFILEPICT)) {
        wFormat = (wFormat & (WORD) 3);
        hMem16 = ClipboardFormats.Pre2[wFormat];
    }
    else if (wFormat == CF_HDROP) {
        hMem16 = ClipboardFormats.hmem16Drop;
    }
    else {
        Temp = ClipboardFormats.NewFormats;
        if (Temp) {
            while ((Temp->Next) && (Temp->Id != wFormat)) {
                Temp = Temp->Next;
            }

            if (Temp->Id == wFormat) {
                hMem16 = Temp->hMem16;
            }
            else {
                LOGDEBUG(6,("WOW::WU32ICBGetHandle: Cann't find private clipboard data format %04lx\n", wFormat));
                hMem16 = (WORD) NULL;
            }
        }
    }

    return (hMem16);
}



VOID W32EmptyClipboard ()
{
    PCBNODE Temp, Temp1;
    int wFormat, cb;
    HAND16 hMem16, hMeta16;
    LPBYTE lpMem16;
    VPVOID vp;

    // Empty CF_METAFILEPICT

    hMem16 = ClipboardFormats.Pre1[CF_METAFILEPICT];
    if (hMem16) {
        vp = GlobalLock16(hMem16, &cb);
        if (vp) {
            GETMISCPTR(vp, lpMem16);
            hMeta16 = FETCHWORD(((LPMETAFILEPICT16) lpMem16)->hMF);
            GlobalUnlockFree16(GlobalLock16(hMeta16, NULL));
        }
        GlobalUnlockFree16(vp);
        ClipboardFormats.Pre1[CF_METAFILEPICT] = 0;
    }

    // Empty CF_DSPMETAFILEPICT

    hMem16 = ClipboardFormats.Pre2[3];
    if (hMem16) {
        vp = GlobalLock16(hMem16, &cb);
        if (vp) {
            GETMISCPTR(vp, lpMem16);
            hMeta16 = FETCHWORD(((LPMETAFILEPICT16) lpMem16)->hMF);
            GlobalUnlockFree16(GlobalLock16(hMeta16, NULL));
        }
        GlobalUnlockFree16(vp);
        ClipboardFormats.Pre2[3] = 0;
    }

    // Empty rest of the formats

    for (wFormat=0; wFormat <= CF_WAVE ; wFormat++) {
        if (ClipboardFormats.Pre1[wFormat]) {
            GlobalUnlockFree16(GlobalLock16(ClipboardFormats.Pre1[wFormat], NULL));
            ClipboardFormats.Pre1[wFormat] = 0;
        }
    }

    for (wFormat=0; wFormat < 4 ; wFormat++) {
        if (ClipboardFormats.Pre2[wFormat]) {
            GlobalUnlockFree16(GlobalLock16(ClipboardFormats.Pre2[wFormat], NULL));
            ClipboardFormats.Pre2[wFormat] = 0;
        }
    }

    if (ClipboardFormats.hmem16Drop) {
        GlobalUnlockFree16(GlobalLock16(ClipboardFormats.hmem16Drop, NULL));
    }
    ClipboardFormats.hmem16Drop = 0;


    // These are the private registered data formats. This list is purged when
    // 32 bit USER purges its clipboard cache.

    Temp = ClipboardFormats.NewFormats;
    ClipboardFormats.NewFormats = NULL;

    while (Temp) {

        Temp1 = Temp->Next;

        if (Temp->hMem16) {
            GlobalUnlockFree16(GlobalLock16(Temp->hMem16, NULL));
        }

        free_w(Temp);

        Temp = Temp1;
    }

}


VOID InitCBFormats ()

{
    int wFormat;

    for (wFormat = 0 ; wFormat <= CF_WAVE ; wFormat++) {
        ClipboardFormats.Pre1[wFormat] = 0;
    }

    for (wFormat=0; wFormat < 4; wFormat++) {
        ClipboardFormats.Pre2[wFormat] = 0;
    }

    ClipboardFormats.hmem16Drop = 0;

    // These are the private registered data formats.

    ClipboardFormats.NewFormats = NULL;


    CFOLEObjectDescriptor = RegisterClipboardFormat ("Object Descriptor");
    CFOLELinkSrcDescriptor = RegisterClipboardFormat ("Link Source Descriptor");

#ifndef DBCS
#ifdef DEBUG

    //
    // This would assert in LoadLibraryAndGetProcAddresses if the function
    // or DLL name has changed.
    //

    if (!(OleStringConversion[WOW_OLE_STRINGCONVERSION].lpfn)) {
        LoadLibraryAndGetProcAddresses("OLETHK32.DLL", OleStringConversion, WOW_OLESTRINGCONVERSION_COUNT);
    }

#endif
#endif // !DBCS

}


HGLOBAL W32ConvertObjDescriptor(HANDLE hMem, UINT flag)
{
    HANDLE hMemOut;

    if (!(OleStringConversion[WOW_OLE_STRINGCONVERSION].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("OLETHK32.DLL", OleStringConversion, WOW_OLESTRINGCONVERSION_COUNT)) {
            return (0);
        }
    }

    hMemOut = (HANDLE) (*OleStringConversion[WOW_OLE_STRINGCONVERSION].lpfn) (hMem, flag);

    return (hMemOut);
}
