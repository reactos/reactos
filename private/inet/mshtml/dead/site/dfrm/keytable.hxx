//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       keytable.hxx
//
//  Contents:   Keyboard response jump table declarations
//
//  History:    03/03/95    laszlog :   created
//
//----------------------------------------------------------------------------

#ifndef _KEYTABLE_HXX_
#define _KEYTABLE_HXX_   1

/**********
lKeyData
---------------------
Value of lParam. Specifies the repeat count, scan code, extended-key flag, context code, previous key-state flag, and transition-state flag, as shown in the following table: 
----------------------
Bit Value   Description
-----------------------
0-15        Specifies the repeat count. The value is the number of times the keystroke is repeated as a result of the user holding down the key.
16-23       Specifies the scan code. The value depends on the original equipment manufacturer (OEM).
24          Specifies whether the key is an extended key, such as the right-hand ALT and CTRL keys that appear on an enhanced 101- or 102-key keyboard. The value is 1 if it is an extended key; otherwise, it is 0.
25-28       Reserved; do not use.
29          Specifies the context code. The value is always 0 for a WM_KEYDOWN message.
30          Specifies the previous key state. The value is 1 if the key is down before the message is sent, or it is 0 if the key is up.
31          Specifies the transition state. The value is always 0 for a WM_KEYDOWN message.
***************/

//  We use bits 25-28 to encode CTRL-ALT-SHIFT state for the keystroke.
//  That's the reason for the below #define values

#define SHIFT_DOWN      0x0200
#define CONTROL_DOWN    0x0400
#define ALT_DOWN        0x0800

#define KEY_DOWN    1
#define KEY_UP      0

#define NO_MODIFIERS    0

#define MODIFIERS(flags)    (flags & (SHIFT_DOWN | CONTROL_DOWN | ALT_DOWN))

class CBaseFrame;

#define KEYMETHOD(func) HRESULT __stdcall func

typedef HRESULT (__stdcall CBase::*KEYHANDLER)(long lGeneric);

struct KEY_MAP
{
    UINT        vk;
    BOOL        fDown;
    UINT        fModifiers;     //  bits for Ctrl, Alt and Shift modifiers
                                //  review: do we need more? IME? AltGr?
    KEYHANDLER  pfnKeyHandler;
    long        lParam;
};




#endif // _KEYTABLE_HXX_

//
//  end of file keytable.hxx
//
//----------------------------------------------------------------------------
