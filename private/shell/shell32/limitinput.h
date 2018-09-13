
#ifndef __LIMITINPUT_H__
#define __LIMITINPUT_H__

#pragma once


// Limit Input Mask values:
#define LIM_FLAGS           0x00000001      // dwFlags contains valid data.  Otherwise all default values are used.
#define LIM_FILTER          0x00000002      // pszFilter contains valid data.  At least one of the filter and mask fields must be used.  Both can be used it desired.
#define LIM_HINST           0x00000008      // hinst contains valid data 
#define LIM_TITLE           0x00000010      // pszTitle contains valid data.  This data will be shown in bold at the top of any tooltips displayed.
#define LIM_MESSAGE         0x00000020      // pszMessage contains valid data.  This data will be shown in default font below the title if a title is also used.
#define LIM_ICON            0x00000040      // hicon contains valid data.  This icon will be displayed in front of the title if a title is given.
#define LIM_NOTIFY          0x00000080      // hwndNotify contains the window handle that should receive any notification messages.  By default, the parent of hwndEdit recieves notifications.
#define LIM_TIMEOUT         0x00000100      // iTimeout is valid.  Otherwise the default timeout of 10 seconds is used.
#define LIM_TIPWIDTH        0x00000200      // cxTipWidth is valid.  Otherwiser the default is 500 pixels.


// Limit Input Flags values:
#define LIF_INCLUDEFILTER   0x00000000      // default value.  pszFilter is a string of allowable characters.
#define LIF_EXCLUDEFILTER   0x00000001      // pszFilter is a string of excluded characters.
#define LIF_CATEGORYFILTER  0x00000002      // pszFilter is not a pointer, but rather its a bitfield indicating types or characters.  If combined with LIF_EXCLUDEFILTER these are excluded categories, otherwise they are allowed categories.

#define LIF_WARNINGBELOW    0x00000000      // default value.  Balloon tooltips will be shown below the window by default.
#define LIF_WARNINGABOVE    0x00000004      // Ballon tooltips will be shown above the window by default.
#define LIF_WARNINGCENTERED 0x00000008      // Ballon tooltips will be shown pointing to the center of the window.
#define LIF_WARNINGOFF      0x00000010      // no balloon tooltip will be displayed upon invalid input.

#define LIF_FORCEUPPERCASE  0x00000020      // all characters will be converted to upper case.  Cannot be use with LIF_FORCELOWERCASE.
#define LIF_FORCELOWERCASE  0x00000040      // all characters will be converted to lower case.  Cannot be use with LIF_FORCEUPPERCASE.

#define LIF_MEESAGEBEEP     0x00000000      // default value.  A tone will be played to alert the user if they attemp invalid input.
#define LIF_SILENT          0x00000080      // No tone will be played.

#define LIF_NOTIFYONBADCHAR 0x00000100      // a notify message will be sent to hwndNotify when invalid input is attempted.
#define LIF_HIDETIPONVALID  0x00000200      // if the tooltip is displayed, it should be hidden when the next valid character is entered.  By default, the tip remains visible for iTimeOut milliseconds.

#define LIF_PASTESKIP       0x00000000      // default value.  When pasting, skip the bad characters and paste all of the good characters.
#define LIF_PASTESTOP       0x00000400      // When pasting, stop when the first bad character is incountered.  Valid characters in front of this will get pasted.
#define LIF_PASTECANCEL     0x00000800      // When pasting, abort the entire paste if any characters are invalid.

#define LIF_KEEPCLIPBOARD   0x00001000      // When pasting, don't modify the contents of the clipboard when there are invalid characters.  By defualt the clipboard is changed.  How it is changed depends on which LIF_PASTE* flag is used.


// Limit Input Category Filters:
// these flags use the result of GetStringTypeEx with CT_TYPE1:
#define LICF_UPPER          0x00000001      // Uppercase  
#define LICF_LOWER          0x00000002      // Lowercase  
#define LICF_DIGIT          0x00000004      // Decimal digits  
#define LICF_SPACE          0x00000008      // Space characters  
#define LICF_PUNCT          0x00000010      // Punctuation  
#define LICF_CNTRL          0x00000020      // Control characters  
#define LICF_BLANK          0x00000040      // Blank characters  
#define LICF_XDIGIT         0x00000080      // Hexadecimal digits  
#define LICF_ALPHA          0x00000100      // Any linguistic character: alphabetic, syllabary, or ideographic  
// these flags check for a few things that GetStringTypeEx doesn't check
#define LICF_BINARYDIGIT    0x00010000      // 0-1
#define LICF_OCTALDIGIT     0x00020000      // 0-7
#define LICF_ATOZUPPER      0x00100000      // A-Z (use LICF_ALPHA for language independent check)
#define LICF_ATOZLOWER      0x00200000      // a-z (use LICF_ALPHA for language independent check)
#define LICF_ATOZ           (LICF_ATOZUPPER|LICF_ATOZLOWER)     // a-z, A-Z

typedef struct tagLIMITINPUT
{
    DWORD       dwMask;
    DWORD       dwFlags;
    HINSTANCE   hinst;
    LPWSTR      pszFilter;      // pointer to a string, or the ID of a string resource if hinst is also given, or LPSTR_TEXTCALLBACK if the parent window should be notified to provide a string.
    LPWSTR      pszTitle;       // pointer to a string, or the ID of a string resource if hinst is also given, or LPSTR_TEXTCALLBACK if the parent window should be notified to provide a string.
    LPWSTR      pszMessage;     // pointer to a string, or the ID of a string resource if hinst is also given, or LPSTR_TEXTCALLBACK if the parent window should be notified to provide a string.
    HICON       hIcon;          // handle to an icon, or I_ICONCALLBACK if the notify window should be asked to provide an icon.
    HWND        hwndNotify;     // handle to a window to process notify messages
    INT         iTimeout;       // time in milliseconds to display the tooltip
    INT         cxTipWidth;     // max width of the tooltip in pixels.  Defaults to 500.
} LIMITINPUT, * LPLIMITINPUT;


typedef struct tagNMLIDISPINFO
{
    NMHDR       hdr;            // stanard notification header structure
    LIMITINPUT  li;             // the mask member indicates which fields must be filled out.
} NMLIDISPINFO, * LPNMLIDISPINFO, NMLIFILTERINFO, * LPNMLIFILTERINFO; 

#define LIN_GETDISPINFO     0x01            // notify code sent to retrieve tooltip display info
#define LIN_GETFILTERINFO   0x02            // notify code sent to retrieve filter or mask info

typedef struct tagNMLIBADCHAR
{
    NMHDR       hdr;            // stanard notification header structure
    WPARAM      wParam;         // wParam sent in WM_CHAR message
    LPARAM      lParam;         // lParam sent in WM_CHAR message
} NMLIBADCHAR, * LPNMLIBADCHAR; 

#define LIN_BADCHAR         0x03            // notify code sent when a character is filtered out

BOOL WINAPI SHLimitInputEdit( HWND hwndEdit, LPLIMITINPUT pli );
BOOL WINAPI SHLimitInputCombo( HWND hwndComboBox, LPLIMITINPUT pli );

#define I_ICONCALLBACK      ((HICON)-1L)

#endif // __LIMITINPUT_H__