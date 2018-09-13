/*++ BUILD Version: 0001 ++*/

#if ! defined( _COLORS_ )

#define _COLORS_



//
// Colorizable string colors type.
//

typedef
struct
_STRINGCOLORS {

    COLORREF    FgndColor;
    COLORREF    BkgndColor;

}   STRINGCOLORS, *PSTRINGCOLORS;

//
// Colorizable string text type.
//

typedef
struct
_STRINGTEXT {

    UINT    Length;
    TCHAR   Text[ MAX_PATH ];

}   STRINGTEXT, *PSTRINGTEXT;


//
// Colorizable item type.
//
typedef enum _COLOR_ITEM {
    SourceWindow,
    DummyWindow,
    WatchWindow,
    LocalsWindow,
    RegistersWindow,
    DisassemblerWindow,
    CommandWindow,
    FloatingPointWindow,
    MemoryWindow,
    CallsWindow,
    BreakpointLine,
    CurrentLine,
    CurrentBreakLine,
    UnInstantiatedBreakpoint,
    TaggedLine,
    TextSelection,
    Keyword,
    Identifier,
    Comment,
    Number,
    Real,
    String,
    ActiveEdit,
    ChangeHistory

}   COLOR_ITEM;


//
// Number of colorizable string IDs.
//

#define MAX_STRINGS     ( 24 )

//
// Colorizable string colors.
//

extern STRINGCOLORS StringColors[ ];

//
// User defined custom colors.
//

extern COLORREF     CustomColors[ ];

//
//  COLORREF
//  GetItemForegroundColor(
//      IN COLOR_ITEM   ColorItem
//      );
//

#define GetItemForegroundColor( ColorItem )                             \
    ( StringColors[ ColorItem ].FgndColor )

//
//  COLORREF
//  GetItemBackgroundColor(
//      IN COLOR_ITEM   ColorItem
//      );
//

#define GetItemBackgroundColor( ColorItem )                             \
    ( StringColors[ ColorItem ].BkgndColor )

//
//  LPCOLORREF
//  GetItemsColors(
//      );
//

#define GetItemsColors( )                                               \
    ( StringColors )

//
//  int
//  GetItemsCount(
//      );
//

#define GetItemsCount( )                                                \
    ( MAX_STRINGS )

//
//  LPCOLORREF
//  GetCustomColors(
//      );
//

#define GetCustomColors( )                                              \
    ( CustomColors )

//
//  int
//  GetCustomColorsCount(
//      );
//

#define GetCustomColorsCount( )                                         \
    ( NUM_CUSTOM_COLORS )

//
//  VOID
//  SetItemsColors(
//      IN PSTRINGCOLORS    NewStringColors
//      );
//

#define SetItemsColors( NewStringColors )                               \
    ( memcpy( StringColors, NewStringColors, sizeof( StringColors ))

//
//  VOID
//  SetCustomColors(
//      IN PCOLORREF        NewCustomColors
//      );
//

#define SetCustomColors( NewCustomColors )                              \
        memcpy( CustomColors, NewCustomColors, sizeof( CustomColors ));

BOOL
SelectColor(
    HWND hWnd
    );

#endif // _COLORS_
