/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "wingdi.h"
#include "msi.h"
#include "msipriv.h"
#include "msidefs.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


const WCHAR szMsiDialogClass[] = {
    'M','s','i','D','i','a','l','o','g','C','l','o','s','e','C','l','a','s','s',0
};
const static WCHAR szStatic[] = { 'S','t','a','t','i','c',0 };

struct msi_control_tag;
typedef struct msi_control_tag msi_control;
typedef UINT (*msi_handler)( msi_dialog *, msi_control *, WPARAM );

struct msi_control_tag
{
    struct msi_control_tag *next;
    HWND hwnd;
    msi_handler handler;
    LPWSTR property;
    WCHAR name[1];
};

typedef struct msi_font_tag
{
    struct msi_font_tag *next;
    HFONT hfont;
    WCHAR name[1];
} msi_font;

struct msi_dialog_tag
{
    MSIPACKAGE *package;
    msi_dialog_event_handler event_handler;
    BOOL finished;
    INT scale;
    DWORD attributes;
    HWND hwnd;
    LPWSTR default_font;
    msi_font *font_list;
    msi_control *control_list;
    WCHAR name[1];
};

typedef UINT (*msi_dialog_control_func)( msi_dialog *dialog, MSIRECORD *rec );
struct control_handler 
{
    LPCWSTR control_type;
    msi_dialog_control_func func;
};

static UINT msi_dialog_checkbox_handler( msi_dialog *, msi_control *, WPARAM );
static void msi_dialog_checkbox_sync_state( msi_dialog *, msi_control * );
static UINT msi_dialog_button_handler( msi_dialog *, msi_control *, WPARAM );
static UINT msi_dialog_edit_handler( msi_dialog *, msi_control *, WPARAM );


INT msi_dialog_scale_unit( msi_dialog *dialog, INT val )
{
    return (dialog->scale * val + 5) / 10;
}

/*
 * msi_dialog_get_style
 *
 * Extract the {\style} string from the front of the text to display and
 *  update the pointer.
 */
static LPWSTR msi_dialog_get_style( LPCWSTR *text )
{
    LPWSTR ret = NULL;
    LPCWSTR p = *text, q;
    DWORD len;

    if( *p++ != '{' )
        return ret;
    q = strchrW( p, '}' );
    if( !q )
        return ret;
    *text = ++q;
    if( *p++ != '\\' )
        return ret;
    len = q - p;
    
    ret = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if( !ret )
        return ret;
    strncpyW( ret, p, len );
    ret[len-1] = 0;
    return ret;
}

static UINT msi_dialog_add_font( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    msi_font *font;
    LPCWSTR face, name;
    LOGFONTW lf;
    INT style;
    HDC hdc;

    /* create a font and add it to the list */
    name = MSI_RecordGetString( rec, 1 );
    font = HeapAlloc( GetProcessHeap(), 0,
                      sizeof *font + strlenW( name )*sizeof (WCHAR) );
    strcpyW( font->name, name );
    font->next = dialog->font_list;
    dialog->font_list = font;

    memset( &lf, 0, sizeof lf );
    face = MSI_RecordGetString( rec, 2 );
    lf.lfHeight = MSI_RecordGetInteger( rec, 3 );
    style = MSI_RecordGetInteger( rec, 5 );
    if( style & msidbTextStyleStyleBitsBold )
        lf.lfWeight = FW_BOLD;
    if( style & msidbTextStyleStyleBitsItalic )
        lf.lfItalic = TRUE;
    if( style & msidbTextStyleStyleBitsUnderline )
        lf.lfUnderline = TRUE;
    if( style & msidbTextStyleStyleBitsStrike )
        lf.lfStrikeOut = TRUE;
    lstrcpynW( lf.lfFaceName, face, LF_FACESIZE );

    /* adjust the height */
    hdc = GetDC( dialog->hwnd );
    if (hdc)
    {
        lf.lfHeight = -MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC( dialog->hwnd, hdc );
    }

    font->hfont = CreateFontIndirectW( &lf );

    TRACE("Adding font style %s\n", debugstr_w(font->name) );

    return ERROR_SUCCESS;
}

static msi_font *msi_dialog_find_font( msi_dialog *dialog, LPCWSTR name )
{
    msi_font *font;

    for( font = dialog->font_list; font; font = font->next )
        if( !strcmpW( font->name, name ) )  /* FIXME: case sensitive? */
            break;

    return font;
}

static UINT msi_dialog_set_font( msi_dialog *dialog, HWND hwnd, LPCWSTR name )
{
    msi_font *font;

    font = msi_dialog_find_font( dialog, name );
    if( font )
        SendMessageW( hwnd, WM_SETFONT, (WPARAM) font->hfont, TRUE );
    else
        ERR("No font entry for %s\n", debugstr_w(name));
    return ERROR_SUCCESS;
}

static UINT msi_dialog_build_font_list( msi_dialog *dialog )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ','`','T','e','x','t','S','t','y','l','e','`',' ',0
    };
    UINT r;
    MSIQUERY *view = NULL;

    TRACE("dialog %p\n", dialog );

    r = MSI_OpenQuery( dialog->package->db, &view, query );
    if( r != ERROR_SUCCESS )
        return r;

    r = MSI_IterateRecords( view, NULL, msi_dialog_add_font, dialog );
    msiobj_release( &view->hdr );

    return r;
}

static msi_control *msi_dialog_add_control( msi_dialog *dialog,
                MSIRECORD *rec, LPCWSTR szCls, DWORD style )
{
    DWORD x, y, width, height, attributes;
    LPCWSTR text, name;
    LPWSTR font = NULL, title = NULL;
    msi_control *control = NULL;

    style |= WS_CHILD | WS_GROUP;

    name = MSI_RecordGetString( rec, 2 );
    control = HeapAlloc( GetProcessHeap(), 0,
                         sizeof *control + strlenW(name)*sizeof(WCHAR) );
    strcpyW( control->name, name );
    control->next = dialog->control_list;
    dialog->control_list = control;
    control->handler = NULL;
    control->property = NULL;

    x = MSI_RecordGetInteger( rec, 4 );
    y = MSI_RecordGetInteger( rec, 5 );
    width = MSI_RecordGetInteger( rec, 6 );
    height = MSI_RecordGetInteger( rec, 7 );
    attributes = MSI_RecordGetInteger( rec, 8 );
    text = MSI_RecordGetString( rec, 10 );

    TRACE("Dialog %s control %s\n", debugstr_w(dialog->name), debugstr_w(text));

    x = msi_dialog_scale_unit( dialog, x );
    y = msi_dialog_scale_unit( dialog, y );
    width = msi_dialog_scale_unit( dialog, width );
    height = msi_dialog_scale_unit( dialog, height );

    if( attributes & 1 )
        style |= WS_VISIBLE;
    if( ~attributes & 2 )
        style |= WS_DISABLED;
    if( text )
    {
        font = msi_dialog_get_style( &text );
        deformat_string( dialog->package, text, &title );
    }
    control->hwnd = CreateWindowW( szCls, title, style,
                          x, y, width, height, dialog->hwnd, NULL, NULL, NULL );
    msi_dialog_set_font( dialog, control->hwnd,
                         font ? font : dialog->default_font );
    HeapFree( GetProcessHeap(), 0, font );
    HeapFree( GetProcessHeap(), 0, title );
    return control;
}

static UINT msi_dialog_text_control( msi_dialog *dialog, MSIRECORD *rec )
{
    TRACE("%p %p\n", dialog, rec);

    msi_dialog_add_control( dialog, rec, szStatic, 0 );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_button_control( msi_dialog *dialog, MSIRECORD *rec )
{
    const static WCHAR szButton[] = { 'B','U','T','T','O','N', 0 };
    msi_control *control;

    TRACE("%p %p\n", dialog, rec);

    control = msi_dialog_add_control( dialog, rec, szButton, 0 );
    control->handler = msi_dialog_button_handler;

    return ERROR_SUCCESS;
}

static UINT msi_dialog_checkbox_control( msi_dialog *dialog, MSIRECORD *rec )
{
    const static WCHAR szButton[] = { 'B','U','T','T','O','N', 0 };
    msi_control *control;
    LPCWSTR prop;

    TRACE("%p %p\n", dialog, rec);

    control = msi_dialog_add_control( dialog, rec, szButton,
                                      BS_CHECKBOX | BS_MULTILINE );
    control->handler = msi_dialog_checkbox_handler;
    prop = MSI_RecordGetString( rec, 9 );
    if( prop )
        control->property = dupstrW( prop );
    msi_dialog_checkbox_sync_state( dialog, control );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_line_control( msi_dialog *dialog, MSIRECORD *rec )
{
    TRACE("%p %p\n", dialog, rec);

    msi_dialog_add_control( dialog, rec, szStatic, SS_ETCHEDHORZ | SS_SUNKEN );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_scrolltext_control( msi_dialog *dialog, MSIRECORD *rec )
{
    const static WCHAR szEdit[] = { 'E','D','I','T',0 };

    TRACE("%p %p\n", dialog, rec);

    msi_dialog_add_control( dialog, rec, szEdit,
                 ES_MULTILINE | WS_VSCROLL | ES_READONLY | ES_AUTOVSCROLL );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_bitmap_control( msi_dialog *dialog, MSIRECORD *rec )
{
    TRACE("%p %p\n", dialog, rec);

    msi_dialog_add_control( dialog, rec, szStatic,
                            SS_BITMAP | SS_LEFT | SS_CENTERIMAGE );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_combo_control( msi_dialog *dialog, MSIRECORD *rec )
{
    static const WCHAR szCombo[] = { 'C','O','M','B','O','B','O','X',0 };

    msi_dialog_add_control( dialog, rec, szCombo,
                            SS_BITMAP | SS_LEFT | SS_CENTERIMAGE );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_edit_control( msi_dialog *dialog, MSIRECORD *rec )
{
    const static WCHAR szEdit[] = { 'E','D','I','T',0 };
    msi_control *control;
    LPCWSTR prop;
    LPWSTR val;

    control = msi_dialog_add_control( dialog, rec, szEdit, 0 );
    control->handler = msi_dialog_edit_handler;
    prop = MSI_RecordGetString( rec, 9 );
    if( prop )
        control->property = dupstrW( prop );
    val = load_dynamic_property( dialog->package, control->property, NULL );
    SetWindowTextW( control->hwnd, val );
    HeapFree( GetProcessHeap(), 0, val );
    return ERROR_SUCCESS;
}

static const WCHAR szText[] = { 'T','e','x','t',0 };
static const WCHAR szButton[] = { 'P','u','s','h','B','u','t','t','o','n',0 };
static const WCHAR szLine[] = { 'L','i','n','e',0 };
static const WCHAR szBitmap[] = { 'B','i','t','m','a','p',0 };
static const WCHAR szCheckBox[] = { 'C','h','e','c','k','B','o','x',0 };
static const WCHAR szScrollableText[] = {
    'S','c','r','o','l','l','a','b','l','e','T','e','x','t',0 };
static const WCHAR szComboBox[] = { 'C','o','m','b','o','B','o','x',0 };
static const WCHAR szEdit[] = { 'E','d','i','t',0 };
static const WCHAR szMaskedEdit[] = { 'M','a','s','k','e','d','E','d','i','t',0 };

struct control_handler msi_dialog_handler[] =
{
    { szText, msi_dialog_text_control },
    { szButton, msi_dialog_button_control },
    { szLine, msi_dialog_line_control },
    { szBitmap, msi_dialog_bitmap_control },
    { szCheckBox, msi_dialog_checkbox_control },
    { szScrollableText, msi_dialog_scrolltext_control },
    { szComboBox, msi_dialog_combo_control },
    { szEdit, msi_dialog_edit_control },
    { szMaskedEdit, msi_dialog_edit_control },
};

#define NUM_CONTROL_TYPES (sizeof msi_dialog_handler/sizeof msi_dialog_handler[0])

static UINT msi_dialog_create_controls( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    LPCWSTR control_type;
    UINT i;

    /* find and call the function that can create this type of control */
    control_type = MSI_RecordGetString( rec, 3 );
    for( i=0; i<NUM_CONTROL_TYPES; i++ )
        if (!strcmpiW( msi_dialog_handler[i].control_type, control_type ))
            break;
    if( i != NUM_CONTROL_TYPES )
        msi_dialog_handler[i].func( dialog, rec );
    else
        ERR("no handler for element type %s\n", debugstr_w(control_type));

    return ERROR_SUCCESS;
}

static UINT msi_dialog_fill_controls( msi_dialog *dialog )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','C','o','n','t','r','o','l',' ',
        'W','H','E','R','E',' ',
           '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',0};
    UINT r;
    MSIQUERY *view = NULL;
    MSIPACKAGE *package = dialog->package;

    TRACE("%p %s\n", dialog, debugstr_w(dialog->name) );

    /* query the Control table for all the elements of the control */
    r = MSI_OpenQuery( package->db, &view, query, dialog->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));
        return ERROR_INVALID_PARAMETER;
    }

    r = MSI_IterateRecords( view, 0, msi_dialog_create_controls, dialog );
    msiobj_release( &view->hdr );

    return r;
}

static msi_control *msi_dialog_find_control( msi_dialog *dialog, LPCWSTR name )
{
    msi_control *control;

    for( control = dialog->control_list; control; control = control->next )
        if( !strcmpW( control->name, name ) ) /* FIXME: case sensitive? */
            break;
    return control;
}

static msi_control *msi_dialog_find_control_by_hwnd( msi_dialog *dialog, HWND hwnd )
{
    msi_control *control;

    for( control = dialog->control_list; control; control = control->next )
        if( hwnd == control->hwnd )
            break;
    return control;
}

static UINT msi_dialog_set_control_condition( MSIRECORD *rec, LPVOID param )
{
    static const WCHAR szHide[] = { 'H','i','d','e',0 };
    static const WCHAR szShow[] = { 'S','h','o','w',0 };
    static const WCHAR szDisable[] = { 'D','i','s','a','b','l','e',0 };
    static const WCHAR szEnable[] = { 'E','n','a','b','l','e',0 };
    msi_dialog *dialog = param;
    msi_control *control;
    LPCWSTR name, action, condition;
    UINT r;

    name = MSI_RecordGetString( rec, 2 );
    action = MSI_RecordGetString( rec, 3 );
    condition = MSI_RecordGetString( rec, 4 );
    r = MSI_EvaluateConditionW( dialog->package, condition );
    control = msi_dialog_find_control( dialog, name );
    if( r && control )
    {
        TRACE("%s control %s\n", debugstr_w(action), debugstr_w(name));

        /* FIXME: case sensitive? */
        if(!strcmpW(action, szHide))
            ShowWindow(control->hwnd, SW_HIDE);
        else if(!strcmpW(action, szShow))
            ShowWindow(control->hwnd, SW_SHOW);
        else if(!strcmpW(action, szDisable))
            EnableWindow(control->hwnd, FALSE);
        else if(!strcmpW(action, szEnable))
            EnableWindow(control->hwnd, TRUE);
        else
            FIXME("Unhandled action %s\n", debugstr_w(action));
    }

    return ERROR_SUCCESS;
}

static UINT msi_dialog_evaluate_control_conditions( msi_dialog *dialog )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ',
        'C','o','n','t','r','o','l','C','o','n','d','i','t','i','o','n',' ',
      'W','H','E','R','E',' ',
        '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',0
    };
    UINT r;
    MSIQUERY *view = NULL;
    MSIPACKAGE *package = dialog->package;

    TRACE("%p %s\n", dialog, debugstr_w(dialog->name) );

    /* query the Control table for all the elements of the control */
    r = MSI_OpenQuery( package->db, &view, query, dialog->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));
        return ERROR_INVALID_PARAMETER;
    }

    r = MSI_IterateRecords( view, 0, msi_dialog_set_control_condition, dialog );
    msiobj_release( &view->hdr );

    return r;
}

/* figure out the height of 10 point MS Sans Serif */
static INT msi_dialog_get_sans_serif_height( HWND hwnd )
{
    static const WCHAR szSansSerif[] = {
        'M','S',' ','S','a','n','s',' ','S','e','r','i','f',0 };
    LOGFONTW lf;
    TEXTMETRICW tm;
    BOOL r;
    LONG height = 0;
    HFONT hFont, hOldFont;
    HDC hdc;

    hdc = GetDC( hwnd );
    if (hdc)
    {
        memset( &lf, 0, sizeof lf );
        lf.lfHeight = MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        strcpyW( lf.lfFaceName, szSansSerif );
        hFont = CreateFontIndirectW(&lf);
        if (hFont)
        {
            hOldFont = SelectObject( hdc, hFont );
            r = GetTextMetricsW( hdc, &tm );
            if (r)
                height = tm.tmHeight;
            SelectObject( hdc, hOldFont );
            DeleteObject( hFont );
        }
        ReleaseDC( hwnd, hdc );
    }
    return height;
}

static LRESULT msi_dialog_oncreate( HWND hwnd, LPCREATESTRUCTW cs )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','D','i','a','l','o','g',' ',
        'W','H','E','R','E',' ',
           '`','D','i','a','l','o','g','`',' ','=',' ','\'','%','s','\'',0};
    static const WCHAR df[] = {
        'D','e','f','a','u','l','t','U','I','F','o','n','t',0 };
    msi_dialog *dialog = (msi_dialog*) cs->lpCreateParams;
    MSIPACKAGE *package = dialog->package;
    MSIQUERY *view = NULL;
    MSIRECORD *rec = NULL;
    DWORD width, height;
    LPCWSTR text;
    LPWSTR title = NULL;
    UINT r;

    TRACE("%p %p\n", dialog, package);

    dialog->hwnd = hwnd;
    SetWindowLongPtrW( hwnd, GWLP_USERDATA, (LONG_PTR) dialog );

    /* fetch the associated record from the Dialog table */
    r = MSI_OpenQuery( package->db, &view, query, dialog->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));
        return -1;
    }
    MSI_ViewExecute( view, NULL );
    MSI_ViewFetch( view, &rec );
    MSI_ViewClose( view );
    msiobj_release( &view->hdr );

    if( !rec )
    {
        TRACE("No record found for dialog %s\n", debugstr_w(dialog->name));
        return -1;
    }

    dialog->scale = msi_dialog_get_sans_serif_height(dialog->hwnd);

    width = MSI_RecordGetInteger( rec, 4 );
    height = MSI_RecordGetInteger( rec, 5 );
    dialog->attributes = MSI_RecordGetInteger( rec, 6 );
    text = MSI_RecordGetString( rec, 7 );

    width = msi_dialog_scale_unit( dialog, width );
    height = msi_dialog_scale_unit( dialog, height ) + 25; /* FIXME */

    dialog->default_font = load_dynamic_property( dialog->package, df, NULL );

    deformat_string( dialog->package, text, &title );
    SetWindowTextW( hwnd, title );
    SetWindowPos( hwnd, 0, 0, 0, width, height,
                  SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW );

    HeapFree( GetProcessHeap(), 0, title );
    msiobj_release( &rec->hdr );

    msi_dialog_build_font_list( dialog );
    msi_dialog_fill_controls( dialog );
    msi_dialog_evaluate_control_conditions( dialog );

    return 0;
}

static UINT msi_dialog_send_event( msi_dialog *dialog, LPCWSTR event, LPCWSTR arg )
{
    LPWSTR event_fmt = NULL, arg_fmt = NULL;

    TRACE("Sending control event %s %s\n", debugstr_w(event), debugstr_w(arg));

    deformat_string( dialog->package, event, &event_fmt );
    deformat_string( dialog->package, arg, &arg_fmt );

    dialog->event_handler( dialog->package, event_fmt, arg_fmt, dialog );

    HeapFree( GetProcessHeap(), 0, event_fmt );
    HeapFree( GetProcessHeap(), 0, arg_fmt );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_set_property( msi_dialog *dialog, LPCWSTR event, LPCWSTR arg )
{
    static const WCHAR szNullArg[] = { '{','}',0 };
    LPWSTR p, prop, arg_fmt = NULL;
    UINT len;

    len = strlenW(event);
    prop = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR));
    strcpyW( prop, &event[1] );
    p = strchrW( prop, ']' );
    if( p && p[1] == 0 )
    {
        *p = 0;
        if( strcmpW( szNullArg, arg ) )
            deformat_string( dialog->package, arg, &arg_fmt );
        MSI_SetPropertyW( dialog->package, prop, arg_fmt );
    }
    else
        ERR("Badly formatted property string - what happens?\n");
    HeapFree( GetProcessHeap(), 0, prop );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_control_event( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    LPCWSTR condition, event, arg;
    UINT r;

    condition = MSI_RecordGetString( rec, 5 );
    r = MSI_EvaluateConditionW( dialog->package, condition );
    if( r )
    {
        event = MSI_RecordGetString( rec, 3 );
        arg = MSI_RecordGetString( rec, 4 );
        if( event[0] == '[' )
            msi_dialog_set_property( dialog, event, arg );
        else
            msi_dialog_send_event( dialog, event, arg );
    }

    return ERROR_SUCCESS;
}

static UINT msi_dialog_button_handler( msi_dialog *dialog,
                                       msi_control *control, WPARAM param )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ','C','o','n','t','r','o','l','E','v','e','n','t',' ',
      'W','H','E','R','E',' ',
         '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',' ',
      'A','N','D',' ',
         '`','C','o','n','t','r','o','l','_','`',' ','=',' ','\'','%','s','\'',' ',
      'O','R','D','E','R',' ','B','Y',' ','`','O','r','d','e','r','i','n','g','`',0
    };
    MSIQUERY *view = NULL;
    UINT r;

    if( HIWORD(param) != BN_CLICKED )
        return ERROR_SUCCESS;

    r = MSI_OpenQuery( dialog->package->db, &view, query,
                       dialog->name, control->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed\n");
        return 0;
    }

    r = MSI_IterateRecords( view, 0, msi_dialog_control_event, dialog );
    msiobj_release( &view->hdr );

    return r;
}

static UINT msi_dialog_get_checkbox_state( msi_dialog *dialog,
                msi_control *control )
{
    WCHAR state[2] = { 0 };
    DWORD sz = 2;

    MSI_GetPropertyW( dialog->package, control->property, state, &sz );
    return atoiW( state ) ? 1 : 0;
}

static void msi_dialog_set_checkbox_state( msi_dialog *dialog,
                msi_control *control, UINT state )
{
    WCHAR szState[2] = { '0', 0 };

    if( state )
        szState[0]++;
    MSI_SetPropertyW( dialog->package, control->property, szState );
}

static void msi_dialog_checkbox_sync_state( msi_dialog *dialog,
                msi_control *control )
{
    UINT state;

    state = msi_dialog_get_checkbox_state( dialog, control );
    SendMessageW( control->hwnd, BM_SETCHECK,
                  state ? BST_CHECKED : BST_UNCHECKED, 0 );
}

static UINT msi_dialog_checkbox_handler( msi_dialog *dialog,
                msi_control *control, WPARAM param )
{
    UINT state;

    if( HIWORD(param) != BN_CLICKED )
        return ERROR_SUCCESS;

    TRACE("clicked checkbox %s, set %s\n", debugstr_w(control->name),
          debugstr_w(control->property));

    state = msi_dialog_get_checkbox_state( dialog, control );
    state = state ? 0 : 1;
    msi_dialog_set_checkbox_state( dialog, control, state );
    msi_dialog_checkbox_sync_state( dialog, control );

    return msi_dialog_button_handler( dialog, control, param );
}

static UINT msi_dialog_edit_handler( msi_dialog *dialog,
                msi_control *control, WPARAM param )
{
    UINT sz, r;
    LPWSTR buf;

    if( HIWORD(param) != EN_CHANGE )
        return ERROR_SUCCESS;

    TRACE("edit %s contents changed, set %s\n", debugstr_w(control->name),
          debugstr_w(control->property));

    sz = 0x20;
    buf = HeapAlloc( GetProcessHeap(), 0, sz*sizeof(WCHAR) );
    while( buf )
    {
        r = GetWindowTextW( control->hwnd, buf, sz );
        if( r < (sz-1) )
            break;
            sz *= 2;
        buf = HeapReAlloc( GetProcessHeap(), 0, buf, sz*sizeof(WCHAR) );
    }

    MSI_SetPropertyW( dialog->package, control->property, buf );

    HeapFree( GetProcessHeap(), 0, buf );

    return ERROR_SUCCESS;
}

static LRESULT msi_dialog_oncommand( msi_dialog *dialog, WPARAM param, HWND hwnd )
{
    msi_control *control;

    TRACE("%p %p %08x\n", dialog, hwnd, param);

    control = msi_dialog_find_control_by_hwnd( dialog, hwnd );
    if( control )
    {
        if( control->handler )
        {
            control->handler( dialog, control, param );
            msi_dialog_evaluate_control_conditions( dialog );
        }
    }
    else
        ERR("button click from nowhere\n");
    return 0;
}

static LRESULT WINAPI MSIDialog_WndProc( HWND hwnd, UINT msg,
                WPARAM wParam, LPARAM lParam )
{
    msi_dialog *dialog = (LPVOID) GetWindowLongPtrW( hwnd, GWLP_USERDATA );

    switch (msg)
    {
    case WM_CREATE:
        return msi_dialog_oncreate( hwnd, (LPCREATESTRUCTW)lParam );

    case WM_COMMAND:
        return msi_dialog_oncommand( dialog, wParam, (HWND)lParam );

    case WM_DESTROY:
        dialog->hwnd = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* functions that interface to other modules within MSI */

msi_dialog *msi_dialog_create( MSIPACKAGE* package, LPCWSTR szDialogName,
                                msi_dialog_event_handler event_handler )
{
    msi_dialog *dialog;
    HWND hwnd;

    TRACE("%p %s\n", package, debugstr_w(szDialogName));

    /* allocate the structure for the dialog to use */
    dialog = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                        sizeof *dialog + sizeof(WCHAR)*strlenW(szDialogName) );
    if( !dialog )
        return NULL;
    strcpyW( dialog->name, szDialogName );
    dialog->package = package;
    dialog->event_handler = event_handler;

    /* create the dialog window, don't show it yet */
    hwnd = CreateWindowW( szMsiDialogClass, szDialogName, WS_OVERLAPPEDWINDOW,
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                     NULL, NULL, NULL, dialog );
    if( !hwnd )
    {
        ERR("Failed to create dialog %s\n", debugstr_w( szDialogName ));
        msi_dialog_destroy( dialog );
        return NULL;
    }

    return dialog;
}

void msi_dialog_end_dialog( msi_dialog *dialog )
{
    dialog->finished = 1;
}

UINT msi_dialog_run_message_loop( msi_dialog *dialog )
{
    MSG msg;

    if( dialog->attributes & msidbDialogAttributesVisible )
    {
        ShowWindow( dialog->hwnd, SW_SHOW );
        UpdateWindow( dialog->hwnd );
    }

    if( dialog->attributes & msidbDialogAttributesModal )
    {
        while( !dialog->finished && GetMessageW( &msg, 0, 0, 0 ) )
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
    }
    else
        return ERROR_IO_PENDING;

    return ERROR_SUCCESS;
}

void msi_dialog_check_messages( msi_dialog *dialog, HANDLE handle )
{
    MSG msg;
    DWORD r;

    do
    {
        while( PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
        if( !handle )
            break;
        r = MsgWaitForMultipleObjects( 1, &handle, 0, INFINITE, QS_ALLEVENTS );
    }
    while( WAIT_OBJECT_0 != r );
}

void msi_dialog_do_preview( msi_dialog *dialog )
{
    dialog->attributes |= msidbDialogAttributesVisible;
    dialog->attributes &= ~msidbDialogAttributesModal;
    msi_dialog_run_message_loop( dialog );
}

void msi_dialog_destroy( msi_dialog *dialog )
{
    if( dialog->hwnd )
        ShowWindow( dialog->hwnd, SW_HIDE );
    
    /* destroy the list of controls */
    while( dialog->control_list )
    {
        msi_control *t = dialog->control_list;
        dialog->control_list = t->next;
        /* leave dialog->hwnd - destroying parent destroys child windows */
        HeapFree( GetProcessHeap(), 0, t->property );
        HeapFree( GetProcessHeap(), 0, t );
    }

    /* destroy the list of fonts */
    while( dialog->font_list )
    {
        msi_font *t = dialog->font_list;
        dialog->font_list = t->next;
        DeleteObject( t->hfont );
        HeapFree( GetProcessHeap(), 0, t );
    }
    HeapFree( GetProcessHeap(), 0, dialog->default_font );

    if( dialog->hwnd )
        DestroyWindow( dialog->hwnd );

    dialog->package = NULL;
    HeapFree( GetProcessHeap(), 0, dialog );
}

void msi_dialog_register_class( void )
{
    WNDCLASSW cls;

    ZeroMemory( &cls, sizeof cls );
    cls.lpfnWndProc   = MSIDialog_WndProc;
    cls.hInstance     = NULL;
    cls.hIcon         = LoadIconW(0, (LPWSTR)IDI_APPLICATION);
    cls.hCursor       = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    cls.lpszMenuName  = NULL;
    cls.lpszClassName = szMsiDialogClass;

    RegisterClassW( &cls );
}

void msi_dialog_unregister_class( void )
{
    UnregisterClassW( szMsiDialogClass, NULL );
}
