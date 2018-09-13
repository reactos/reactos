/*++ BUILD Version: 0001

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Fonts.h

Abstract:


Author:

    David J. Gilman (davegi) 30-Jul-1992
         Griffith Wm. Kadnier (v-griffk) 11-Sep-1992

Environment:

    CRT, Windows, User Mode

--*/

#if ! defined( _FONTS_ )

#define _FONTS_


extern LOGFONT  WindowLogFont[ ];

extern BOOL     UseSystemFontFlag;

//  WORD
//  GetCurrentWindowType(
//      );


#define GetCurrentWindowType( )                                         \
    ( Docs[Views[curView].Doc].docType )




//  LPLOGFONT GetDefaultWindowLogfont();

#define GetDefaultWindowLogfont( )                                      \
    ( WindowLogFont[ DEFAULT_WINDOW_FONT ])



#define SetWindowLogfont( WindowType, NewWindowLogfont)                \
    {                                                                   \
    memcpy( WindowLogFont[WindowType], NewWindowLogFont, sizeof(LOGFONT));         \
    UseSystemFontFlag = FALSE;                                          \
    }


//  VOID SetDefaultWindowLogfont (IN LPLOGFONT NewDefaultWindowLogfont);

#define SetDefaultWindowLogfont(NewDefaultWindowLogfont)                \
    {                                                                   \
    memcpy( WindowLogFont[DEFAULT_WINDOW_FONT], NewWindowLogFont, sizeof(LOGFONT));         \
    UseSystemFontFlag = FALSE;                                          \
    }




BOOL SelectFont (HWND hWnd);

#endif // _FONTS_
