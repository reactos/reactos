/*++

Copyright (c) 1993-1998 Microsoft Corporation

Module Name:

    resource.c

Abstract:

    Routines that manipulate resources (strings, messages, etc).

Author:

    Ted Miller (tedm) 6-Feb-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


VOID
SetDlgText(
    IN HWND hwndDlg,
    IN INT  iControl,
    IN UINT nStartString,
    IN UINT nEndString
    )
/*++

Routine Description:

    This routine concatenates a number of string resources and does a
    SetWindowText() for a dialog text control.

Arguments:

    hwndDlg - Handle to dialog window

    iControl - Dialog control ID to receive text

    nStartString - ID of first string resource to concatenate

    nEndString - ID of last string resource to concatenate

Return Value:

    None.

Remarks:

    String IDs must be consecutive.

--*/
{
    TCHAR StringBuffer[SDT_MAX_TEXT];
    UINT i;
    INT  Len = 0;

    for(i = nStartString;
        ((i <= nEndString) && (Len < (SDT_MAX_TEXT - 1)));
        i++)
    {
        Len += LoadString(MyDllModuleHandle,
                          i,
                          StringBuffer + Len,
                          SDT_MAX_TEXT - Len
                         );
    }

    if(!Len) {
        StringBuffer[0] = TEXT('\0');
    }

    SetDlgItemText(hwndDlg, iControl, StringBuffer);
}


PTSTR
MyLoadString(
    IN UINT StringId
    )

/*++

Routine Description:

    Retreive a string from the string resources of this module.

Arguments:

    StringId - supplies string table identifier for the string.

Return Value:

    Pointer to buffer containing string. If the string was not found
    or some error occurred retreiving it, this buffer will be empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    PTSTR Buffer, p;
    int Length, RequiredLength;

    //
    // Start out with a reasonably-sized buffer so that we'll rarely need to
    // grow the buffer and retry (Length is in terms of characters, not bytes).
    //
    Length = LINE_LEN;

    while(TRUE) {

        Buffer = MyMalloc(Length * sizeof(TCHAR));
        if(!Buffer) {
            return NULL;
        }

        RequiredLength = LoadString(MyDllModuleHandle,
                                    StringId,
                                    Buffer,
                                    Length
                                   );
        if(!RequiredLength) {
            *Buffer = TEXT('\0');
            Length = 1;
            break;
        }

        //
        // Because of the brain-dead way LoadString works, there's no way to
        // tell for sure whether your buffer was big enough in the case where
        // the length returned just fits in the buffer you supplied (the API
        // silently truncates in this case).  Thus, if RequiredLength is exactly
        // the size of our supplied buffer (minus terminating null, which
        // LoadString doesn't count), we increase the buffer size by LINE_LEN
        // characters and try again, to make sure we get the whole string.
        //
        if(RequiredLength < (Length - 1)) {
            //
            // Looks like we got the whole string.  Set the length to be the
            // required length + 1 character, to accommodate the terminating
            // null character.
            //
            Length = RequiredLength + 1;
            break;
        } else {
            MyFree(Buffer);
            Length += LINE_LEN;
        } 
    }

    //
    // Resize the buffer to its correct size.  If this fails (which it shouldn't)
    // it's no big deal, it just means we're using a larger buffer for this string
    // than we need to.
    //
    if(p = MyRealloc(Buffer, Length * sizeof(TCHAR))) {
        Buffer = p;
    }

    return Buffer;
}


PTSTR
FormatStringMessageV(
    IN UINT     FormatStringId,
    IN va_list *ArgumentList
    )

/*++

Routine Description:

    Retreive a string from the string resources of this module and
    format it using FormatMessage.

Arguments:

    StringId - supplies string table identifier for the string.

    ArgumentList - supplies list of strings to be substituted in the
        format string.

Return Value:

    Pointer to buffer containing formatted message. If the string was not found
    or some error occurred retreiving it, this buffer will be empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    PTSTR FormatString;
    va_list arglist;
    PTSTR Message;
    PTSTR Return;
    DWORD d;

    //
    // First, load the format string.
    //
    FormatString = MyLoadString(FormatStringId);
    if(!FormatString) {
        return(NULL);
    }

    //
    // Now format the message using the arguements the caller passed.
    //
    d = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            FormatString,
            0,
            0,
            (PTSTR)&Message,
            0,
            ArgumentList
            );

    MyFree(FormatString);

    if(!d) {
        return(NULL);
    }

    //
    // Make duplicate using our memory system so user can free with MyFree().
    //
    Return = DuplicateString(Message);
    LocalFree((HLOCAL)Message);
    return(Return);
}


PTSTR
FormatStringMessage(
    IN UINT FormatStringId,
    ...
    )

/*++

Routine Description:

    Retreive a string from the string resources of this module and
    format it using FormatMessage.

Arguments:

    StringId - supplies string table identifier for the string.

Return Value:

    Pointer to buffer containing formatted message. If the string was not found
    or some error occurred retreiving it, this buffer will be empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    va_list arglist;
    PTSTR p;

    va_start(arglist,FormatStringId);
    p = FormatStringMessageV(FormatStringId,&arglist);
    va_end(arglist);

    return(p);
}


PTSTR
FormatStringMessageFromStringV(
    IN PTSTR    FormatString,
    IN va_list *ArgumentList
    )

/*++

Routine Description:

    Format the input string using FormatMessage.

Arguments:

    FormatString - supplies the format string.

    ArgumentList - supplies list of strings to be substituted in the
        format string.

Return Value:

    Pointer to buffer containing formatted message. If some error occurred
    formatting the string, this buffer will be empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    va_list arglist;
    PTSTR Message;
    PTSTR Return;
    DWORD d;

    //
    // Format the message using the arguements the caller passed.
    //
    d = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
            FormatString,
            0,
            0,
            (PTSTR)&Message,
            0,
            ArgumentList
            );

    if(!d) {
        return(NULL);
    }

    //
    // Make duplicate using our memory system so user can free with MyFree().
    //
    Return = DuplicateString(Message);
    LocalFree((HLOCAL)Message);
    return(Return);
}


PTSTR
FormatStringMessageFromString(
    IN PTSTR FormatString,
    ...
    )

/*++

Routine Description:

    Format the input string using FormatMessage.

Arguments:

    FormatString - supplies the format string.

Return Value:

    Pointer to buffer containing formatted message. If some error occurred
    formatting the string, this buffer will be empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    va_list arglist;
    PTSTR p;

    va_start(arglist,FormatString);
    p = FormatStringMessageFromStringV(FormatString,&arglist);
    va_end(arglist);

    return(p);
}


INT
FormatMessageBox(
    IN HANDLE hinst,
    IN HWND   hwndParent,
    IN UINT   TextMessageId,
    IN PCTSTR Title,
    IN UINT   Style,
    ...
    )
/*++

Routine Description:

    This routine formats two message strings--one containing messagebox text,
    and the other containing a messagebox caption.  The message box is then
    displayed.

    The message ids can be either a message in this dll's message table
    resources or a win32 error code, in which case a description of
    that error is retreived from the system.

Arguments:

    hinst - Supplies the handle of the module containing string resources to
        be used.

    hwndParent - Supplies the handle of window to be the parent of the message box.

    TextMessageId - Supplies message-table identifier or win32 error code
        for the messagebox text.

    TitleMessageId - Supplies message-table identifier or win32 error code
        for the messagebox caption.

    Style - Supplies style flags for the message box.

    ... - Supplies arguments to be inserted in the message text.

Return Value:

    The return value is zero if there is not enough memory to create the message box, or
    if a failure occurred while creating the message box.

    If the function succeeds, the return value is one of the following menu-item values
    returned by the dialog box:

        IDABORT   Abort button was selected.
        IDCANCEL  Cancel button was selected.
        IDIGNORE  Ignore button was selected.
        IDNO      No button was selected.
        IDOK      OK button was selected.
        IDRETRY   Retry button was selected.
        IDYES     Yes button was selected.

    If a message box has a Cancel button, the function returns the IDCANCEL value if
    either the ESC key is pressed or the Cancel button is selected. If the message box
    has no Cancel button, pressing ESC has no effect.

--*/
{
    va_list arglist;
    PTSTR Text = NULL;
    INT ret;

    //
    // We should never be called if we're not interactive.
    //
    MYASSERT(!(GlobalSetupFlags & PSPGF_NONINTERACTIVE));

    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        return 0;
    }

    try {

        va_start(arglist, Style);
        Text  = RetreiveAndFormatMessageV(TextMessageId, &arglist);
        va_end(arglist);

        if(Text) {
            //
            // We are currently always beeping
            // BUGBUG (setupx) Is this the right thing to do?
            //
            MessageBeep(Style & (MB_ICONHAND|MB_ICONEXCLAMATION|MB_ICONQUESTION|MB_ICONASTERISK));
            ret = MessageBox(hwndParent, Text, Title, Style);
        } else {
            ret = 0;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ret = 0;
    }

    if(Text) {
        MyFree(Text);
    }

    return ret;
}


PTSTR
RetreiveAndFormatMessageV(
    IN UINT     MessageId,
    IN va_list *ArgumentList
    )

/*++

Routine Description:

    Format a message string using a message string and caller-supplied
    arguments.

    The message id can be either a message in this dll's message table
    resources or a win32 error code, in which case a description of
    that error is retreived from the system.

Arguments:

    MessageId - supplies message-table identifier or win32 error code
        for the message.

    ArgumentList - supplies arguments to be inserted in the message text.

Return Value:

    Pointer to buffer containing formatted message. If the message was not found
    or some error occurred retreiving it, this buffer will be empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    DWORD d;
    PTSTR Buffer;
    PTSTR Message;
    TCHAR ModuleName[MAX_PATH];
    TCHAR ErrorNumber[24];
    PTCHAR p;
    PTSTR Args[2];

    d = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER
          | ((MessageId < MSG_FIRST) ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE),
            (PVOID)MyDllModuleHandle,
            MessageId,
            MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),
            (PTSTR)&Buffer,
            0,
            ArgumentList
            );

    if(!d) {
        if(GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {
            return(NULL);
        }

        wsprintf(ErrorNumber,TEXT("%x"),MessageId);
        Args[0] = ErrorNumber;

        Args[1] = ModuleName;

        if(GetModuleFileName(MyDllModuleHandle,ModuleName,MAX_PATH)) {
            if(p = _tcsrchr(ModuleName,TEXT('\\'))) {
                Args[1] = p+1;
            }
        } else {
            ModuleName[0] = 0;
        }

        d = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL,
                ERROR_MR_MID_NOT_FOUND,
                MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),
                (PTSTR)&Buffer,
                0,
                (va_list *)Args
                );

        if(!d) {
            //
            // Give up.
            //
            return(NULL);
        }
    }

    //
    // Make duplicate using our memory system so user can free with MyFree().
    //
    Message = DuplicateString(Buffer);

    LocalFree((HLOCAL)Buffer);

    return(Message);
}


PTSTR
RetreiveAndFormatMessage(
    IN UINT MessageId,
    ...
    )

/*++

Routine Description:

    Format a message string using a message string and caller-supplied
    arguments.

    The message id can be either a message in this dll's message table
    resources or a win32 error code, in which case a description of
    that error is retreived from the system.

Arguments:

    MessageId - supplies message-table identifier or win32 error code
        for the message.

    ... - supplies arguments to be inserted in the message text.

Return Value:

    Pointer to buffer containing formatted message. If the message was not found
    or some error occurred retreiving it, this buffer will be empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    va_list arglist;
    PTSTR p;

    va_start(arglist,MessageId);
    p = RetreiveAndFormatMessageV(MessageId,&arglist);
    va_end(arglist);

    return(p);
}
