/****************************** Module Header ******************************\
* Module Name: kbd.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Keyboard table values that form the basis for languages and keyboard types.
* The basis is US, kbd type 4 - all others are a variation on this.
* This file is included by all kbd**.h files.
*
* History:
* 10-Jan-1991 GregoryW
* 23-Apr-1991 IanJa         VSC_TO_VK _* macros from oemtab.c
\***************************************************************************/

#ifndef _KBD_
#define _KBD_

/****************************************************************************\
*
* Keyboard Layers.   Used in kdb??.dll and in usersrv.dll
*
\****************************************************************************/

/*
 * Key Event (KE) structure
 * Stores a Virtual Key event
 */
typedef struct tagKE {
    union {
        BYTE bScanCode;    // Virtual Scan Code (Set 1)
        WCHAR wchInjected; // Unicode char from SendInput()
    };
    USHORT usFlaggedVk;    // Vk | Flags
    DWORD  dwTime;         // time in milliseconds
} KE, *PKE;

typedef BOOL (* KEPROC)(PKE pKe);

/*
 * KE.usFlaggedVk values, also used in the keyboard layer tables.
 */
#define KBDEXT        (USHORT)0x0100
#define KBDMULTIVK    (USHORT)0x0200
#define KBDSPECIAL    (USHORT)0x0400
#define KBDNUMPAD     (USHORT)0x0800
#define KBDUNICODE    (USHORT)0x1000
#define KBDINJECTEDVK (USHORT)0x2000
#define KBDBREAK      (USHORT)0x8000

/*
 * Key message lParam bits
 */
#define EXTENDED_BIT   0x01000000
#define DONTCARE_BIT   0x02000000
#define FAKE_KEYSTROKE 0x02000000
#define ALTNUMPAD_BIT  0x04000000 // copied from windows\inc\wincon.w

/*
 * Keyboard Shift State defines. These correspond to the bit mask defined
 * by the VkKeyScan() API.
 */
#define KBDBASE        0
#define KBDSHIFT       1
#define KBDCTRL        2
#define KBDALT         4
// three symbols KANA, ROYA, LOYA are for FE
#define KBDKANA        8
#define KBDROYA        0x10
#define KBDLOYA        0x20
#define KBDGRPSELTAP   0x80

/*
 * Handy diacritics
 */
#define GRAVE           0x0300
#define ACUTE           0x0301
#define CIRCUMFLEX      0x0302
#define TILDE           0x0303
#define MACRON          0x0304
#define OVERSCORE       0x0305
#define BREVE           0x0306
#define DOT_ABOVE       0x0307
#define UMLAUT          0x0308
#define DIARESIS        UMLAUT
#define HOOK_ABOVE      0x0309
#define RING            0x030A
#define DOUBLE_ACUTE    0x030B
#define HACEK           0x030C

#define CEDILLA         0x0327
#define OGONEK          0x0328
#define TONOS           0x0384
#define DIARESIS_TONOS  0x0385


#define wszGRAVE           L"\x0300"
#define wszACUTE           L"\x0301"
#define wszCIRCUMFLEX      L"\x0302"
#define wszTILDE           L"\x0303"
#define wszMACRON          L"\x0304"
#define wszOVERSCORE       L"\x0305"
#define wszBREVE           L"\x0306"
#define wszDOT_ABOVE       L"\x0307"
#define wszUMLAUT          L"\x0308"
#define wszHOOK_ABOVE      L"\x0309"
#define wszRING            L"\x030A"
#define wszDOUBLE_ACUTE    L"\x030B"
#define wszHACEK           L"\x030C"

#define wszCEDILLA         L"\x0327"
#define wszOGONEK          L"\x0328"
#define wszTONOS           L"\x0384"
#define wszDIARESIS_TONOS  L"\x0385"

#define IDS_FROM_SCANCODE(prefix, base) \
        (0xc000 + ((0x ## prefix) >= 0xE0 ? 0x100 : 0) + (0x ## base))

/***************************************************************************\
* MODIFIER KEYS
*
* All keyboards have "Modifier" keys which are used to alter the behaviour of
* some of the other keys.  These shifter keys are usually:
*   Shift  (left and/or right Shift key)
*   Ctrl   (left and/or right Ctrl key)
*   Alt    (left and/or right Alt key)
*   AltGr  (right Alt key only)
*
* NOTE:
*   All keyboards use the Shift key.
*   All keyboards use a Ctrl key to generate ASCII control characters.
*   All keyboards with a number pad use the Alt key and the NumPad to
*     generate characters by number.
*   Keyboards using AltGr as a Modifier Key usually translate the Virtual
*     ScanCode to Virtual Keys VK_CTRL + VK_ALT at input time: the Modifier
*     tables should be written to treat Ctrl + Alt as a valid shifter
*     key combination in these cases.
*
* By holding down 0 or more of these Modifier keys, a "shift state" is
* obtained : the shift state may affect the translation of Virtual Scancodes
* to Virtual Keys and/or the translation of Virtuals Key to Characters.
*
* EXAMPLES:
*
* Each key on a particular keyboard may be marked with up to five different
* characters in five different positions:
*
*              .-------.
*             /|       |\
*            : | 2   4 | :
*            | |       | |
*            | |       | |
*            | | 1   3 | |
*            | |_______| |
*            | /       \ |
*            |/    5    \|
*            `-----------'
*
* A key may also be able to generate a character that is not marked on it:
* these are ASCII Control chars, lower-case letters and/or "invisible keys".
*                                                  .-------.
*      An example of an "Invisible Key":          /|       |\
*                                                : | >     | :
*  The German M24 keyboard 2 should produce the  | |       | |
*  '|' character when ALT SHIFT is is held down  | |       | |
*  while the '<' key (shown here) is pressed:    | | <   \ | |
*  This keyboard has four other invisible        | |_______| |
*  characters.  France, Italy and Spain also     | /       \ |
*  support invisible characters on the M24       |/         \|
*  Keyboard 2 with ALT SHIFT depressed.          `-----------'
*
* The keyboard table must list the keys that contribute to it's shift state,
* and indicate which combinations are valid.  This is done with
*    aCharModifiers[]  - convert combinations of Modifier Keys to Bitmasks.
* and
*    aModification[];  - convert Modifier Bitmasks to enumerated Modifications
*
* AN EXAMPLE OF VALID AND INVALID MODIFIER KEY COMBINATIONS
*
*    The US English keyboard has 3 Modifier keys:
*      Shift (left or right); Ctrl (left or right); and Alt (left or right).
*
*    The only valid combinations of these Modifier Keys are:
*      none pressed      : Character at position (1) on the key.
*      Shift             : Character at position (2) on the key.
*      Ctrl              : Ascii Control characters
*      Shift + Ctrl      : Ascii Control characters
*      Alt               : Character-by-number on the numpad
*
*    The invalid combinations (that do not generate any characters) are:
*      Shift + Alt
*      Alt + Ctrl
*      Shift + Alt + Ctrl
*
* Something (???) :
* -----------------
*      Modifier keys              Character produced
*      -------------------------  ------------------
*   0  No shifter key depressed   position 1
*   1  Shift key is depressed     position 2
*   2  AltGr (r.h. Alt) depressed position 4 or 5 (whichever is marked)
*
* However, note that 3 shifter keys (SHIFT, can be combined in a
* characters, depending on the Keyboards
* Consider the following keyboards:
*
*     .-------.            STRANGE KBD         PECULIAR KBD
*    /|       |\           ==================  ==================
*   : | 2   4 | :    1   -
*   | |       | |    2   - SHIFT               SHIFT
*   | |       | |    3   - MENU                MENU
*   | | 1   3 | |    4   - SHIFT + MENU        SHIFT + MENU
*   | |_______| |    5   -    no such keys     CTRL  + MENU
*   | /       \ |
*   |/    5    \|
*   `-----------'
* Both STRANGE and PECULIAR keyboards could have aVkToBits[] =
*   { VK_SHIFT  , KBDSHIFT }, // 0x01
*   { VK_CONTROL, KBDCTRL  }, // 0x02
*   { VK_MENU   , KBDALT   }, // 0x04
*   { 0,          0        }
*
* The STRANGE keyboard has 4 distinct shift states, while the PECULIAR kbd
* has 5.  However, note that 3 shifter bits can be combined in a
* total of 2^3 == 8 ways.  Each such combination must be related to one (or
* none) of the enumerated shift states.
* Each shifter key combination can be represented by three binary bits:
*  Bit 0  is set if VK_SHIFT is down
*  Bit 1  is set if VK_CONTROL is down
*  Bit 2  is set if VK_MENU is down
*
* Example: If the STRANGE keyboard generates no characters in combination
* when just the ALT key is held down, nor when the SHIFT, CTRL and ALT keys
* are all held down, then the tables might look like this:
*
*                                VK_MENU,
*                        VK_CTRL,                    0
*    };
*    aModification[] = {
*        0,            //   0       0       0     = 000  <none>
*        1,            //   0       0       1     = 001  SHIFT
*        SHFT_INVALID, //   0       1       0     = 010  ALT
*        2,            //   0       1       1     = 011  SHIFT ALT
*        3,            //   1       0       0     = 100  CTRL
*        4,            //   1       0       1     = 101  SHIFT CTRL
*        5,            //   1       1       0     = 110  CTRL ALT
*        SHFT_INVALID  //   1       1       1     = 111  SHIFT CTRL ALT
*    };
*
*
\***************************************************************************/

/***************************************************************************\
* VK_TO_BIT - associate a Virtual Key with a Modifier bitmask.
*
* Vk        - the Virtual key (eg: VK_SHIFT, VK_RMENU, VK_CONTROL etc.)
*             Special Values:
*                0        null terminator
* ModBits   - a combination of KBDALT, KBDCTRL, KBDSHIFT and kbd-specific bits
*             Any kbd-specific shift bits must be the lowest-order bits other
*             than KBDSHIFT, KBDCTRL and KBDALT (0, 1 & 2)
*
* Those languages that use AltGr (VK_RMENU) to shift keys convert it to
* CTRL+ALT with the KBDSPECIAL bit in the ausVK[] entry for VK_RMENU
* and by having an entry in aVkToPfnOem[] to simulate the right Vk sequence.
*
\***************************************************************************/
typedef struct {
    BYTE Vk;
    BYTE ModBits;
} VK_TO_BIT, *PVK_TO_BIT;

/***************************************************************************\
* pModNumber  - a table to map shift bits to enumerated shift states
*
* Table attributes: Ordered table
*
* Maps all possible shifter key combinations to an enumerated shift state.
* The size of the table depends on the value of the highest order bit used
* in aCharModifiers[*].ModBits
*
* Special values for aModification[*]
*   SHFT_INVALID - no characters produced with this shift state.
LATER: (ianja) no SHFT_CTRL - control characters encoded in tables like others
*   SHFT_CTRL    - standard control character production (all keyboards must
*                  be able to produce CTRL-C == 0x0003 etc.)
*   Other        - enumerated shift state (not less than 0)
*
* This table is indexed by the Modifier Bits to obtain an Modification Number.
*
*                        CONTROL MENU SHIFT
*
*    aModification[] = {
*        0,            //   0     0     0     = 000  <none>
*        1,            //   0     0     1     = 001  SHIFT
*        SHFT_INVALID, //   0     1     0     = 010  ALT
*        2,            //   0     1     1     = 011  SHIFT ALT
*        3,            //   1     0     0     = 100  CTRL
*        4,            //   1     0     1     = 101  SHIFT CTRL
*        5,            //   1     1     0     = 110  CTRL ALT
*        SHFT_INVALID  //   1     1     1     = 111  SHIFT CTRL ALT
*    };
*
\***************************************************************************/
typedef struct {
    PVK_TO_BIT pVkToBit;     // Virtual Keys -> Mod bits
    WORD       wMaxModBits;  // max Modification bit combination value
    BYTE       ModNumber[];  // Mod bits -> Modification Number
} MODIFIERS, *PMODIFIERS;

WORD GetModifierBits(PMODIFIERS pModifiers, LPBYTE afKeyState);
WORD GetModificationNumber(PMODIFIERS pModifiers, WORD wModBits);

// FE Modifiers_VK
extern PMODIFIERS gpModifiers_VK;
extern MODIFIERS Modifiers_VK_STANDARD;
extern MODIFIERS Modifiers_VK_IBM02;

#define SHFT_INVALID 0x0F

/***************************************************************************\
* apulCvt_VK[] - obtain VK translation table from shift state
*     A VK translation table is used to change the value of the Virtual Key
*     according to the shift state.   OEM only (not locale-specific)
\***************************************************************************/
extern PULONG *gapulCvt_VK;
extern ULONG *gapulCvt_VK_101[];
extern ULONG *gapulCvt_VK_84[];
// gapulCvt_VK_IBM02[] is for FE
extern ULONG *gapulCvt_VK_IBM02[];

/***************************************************************************\
* awNumPadCvt[]   - Translate cursor movement keys to numpad keys
\***************************************************************************/
extern MODIFIERS Modifiers_VK;
extern BYTE aVkNumpad[];

/***************************************************************************\
* VSC_VK     - Associate a Virtual Scancode with a Virtual Key
*  Vsc - Virtual Scancode
*  Vk  - Virtual Key | flags
* Used by VKFromVSC() for scancodes prefixed 0xE0 or 0xE1
\***************************************************************************/
typedef struct _VSC_VK {
    BYTE Vsc;
    USHORT Vk;
} VSC_VK, *PVSC_VK;

/***************************************************************************\
* VK_VSC     - Associate a Virtual Key with a Virtual Scancode
*  Vk  - Virtual Key
*  Vsc - Virtual Scancode
* Used by MapVirtualKey for Virtual Keys not appearing in ausVK[]
\***************************************************************************/
typedef struct _VK_VSC {
    BYTE Vk;
    BYTE Vsc;
} VK_VSC, *PVK_VSC;

/***************************************************************************\
*
* VK_TO_WCHARS<n> - Associate a Virtual Key with <n> UNICODE characters
*
* VirtualKey  - The Virtual Key.
* wch[]       - An array of characters, one for each shift state that
*               applies to the specified Virtual Key.
*
* Special values for VirtualKey:
*    -1        - This entry contains dead chars for the previous entry
*    0         - Terminates a VK_TO_WCHARS[] table
*
* Special values for Attributes:
*    CAPLOK    - The CAPS-LOCK key affects this key like SHIFT
*    SGCAPS    - CapsLock uppercases the unshifted char (Swiss-German)
*
* Special values for wch[*]:
*    WCH_NONE  - No character is generated by pressing this key with the
*                current shift state.
*    WCH_DEAD  - The character is a dead-key: the next VK_TO_WCHARS[] entry
*                will contain the values of the dead characters (diaresis)
*                that can be produced by the Virtual Key.
*    WCH_LGTR  - The character is a ligature.  The characters generated by
*                this keystroke are found in the ligature table.
*
\***************************************************************************/
#define WCH_NONE 0xF000
#define WCH_DEAD 0xF001
#define WCH_LGTR 0xF002

#define CAPLOK      0x01
#define SGCAPS      0x02
#define CAPLOKALTGR 0x04
// KANALOK is for FE
#define KANALOK     0x08
#define GRPSELTAP   0x80

/*
 * Macro for VK to WCHAR with "n" shift states
 */
#define TYPEDEF_VK_TO_WCHARS(n) typedef struct _VK_TO_WCHARS##n {  \
                                    BYTE  VirtualKey;      \
                                    BYTE  Attributes;      \
                                    WCHAR wch[n];          \
                                } VK_TO_WCHARS##n, *PVK_TO_WCHARS##n;

/*
 * To facilitate coding the table scanning routine.
 */

/*
 * Table element types (for various numbers of shift states), used
 * to facilitate static initializations of tables.
 * VK_TO_WCHARS1 and PVK_TO_WCHARS1 may be used as the generic type
 */
TYPEDEF_VK_TO_WCHARS(1) // VK_TO_WCHARS1, *PVK_TO_WCHARS1;
TYPEDEF_VK_TO_WCHARS(2) // VK_TO_WCHARS2, *PVK_TO_WCHARS2;
TYPEDEF_VK_TO_WCHARS(3) // VK_TO_WCHARS3, *PVK_TO_WCHARS3;
TYPEDEF_VK_TO_WCHARS(4) // VK_TO_WCHARS4, *PVK_TO_WCHARS4;
TYPEDEF_VK_TO_WCHARS(5) // VK_TO_WCHARS5, *PVK_TO_WCHARS5;
TYPEDEF_VK_TO_WCHARS(6) // VK_TO_WCHARS6, *PVK_TO_WCHARS5;
TYPEDEF_VK_TO_WCHARS(7) // VK_TO_WCHARS7, *PVK_TO_WCHARS7;
// these three (8,9,10) are for FE
TYPEDEF_VK_TO_WCHARS(8) // VK_TO_WCHARS8, *PVK_TO_WCHARS8;
TYPEDEF_VK_TO_WCHARS(9) // VK_TO_WCHARS9, *PVK_TO_WCHARS9;
TYPEDEF_VK_TO_WCHARS(10) // VK_TO_WCHARS10, *PVK_TO_WCHARS10;

/***************************************************************************\
*
* VK_TO_WCHAR_TABLE - Describe a table of VK_TO_WCHARS1
*
* pVkToWchars     - points to the table.
* nModifications  - the number of shift-states supported by this table.
*                   (this is the number of elements in pVkToWchars[*].wch[])
*
* A keyboard may have several such tables: all keys with the same number of
*    shift-states are grouped together in one table.
*
* Special values for pVktoWchars:
*     NULL     - Terminates a VK_TO_WCHAR_TABLE[] list.
*
\***************************************************************************/

typedef struct _VK_TO_WCHAR_TABLE {
    PVK_TO_WCHARS1 pVkToWchars;
    BYTE           nModifications;
    BYTE           cbSize;
} VK_TO_WCHAR_TABLE, *PVK_TO_WCHAR_TABLE;

/***************************************************************************\
*
* Dead Key (diaresis) tables
*
* LATER IanJa: supplant by an NLS API that composes Diacritic+Base -> WCHAR
*
\***************************************************************************/
typedef struct {
    DWORD  dwBoth;  // diacritic & char
    WCHAR  wchComposed;
    USHORT uFlags;
} DEADKEY, *PDEADKEY;

#define DEADTRANS(ch, accent, comp, flags) { MAKELONG(ch, accent), comp, flags}

/*
 * Bit values for uFlags
 */
#define DKF_DEAD  0x0001

/***************************************************************************\
*
* Ligature table
*
\***************************************************************************/
/*
 * Macro for ligature with "n" characters
 */
#define TYPEDEF_LIGATURE(n) typedef struct _LIGATURE##n {     \
                                    BYTE  VirtualKey;         \
                                    WORD  ModificationNumber; \
                                    WCHAR wch[n];             \
                                } LIGATURE##n, *PLIGATURE##n;

/*
 * To facilitate coding the table scanning routine.
 */

/*
 * Table element types (for various numbers of ligatures), used
 * to facilitate static initializations of tables.
 *
 * LIGATURE1 and PLIGATURE1 are used as the generic type
 */
TYPEDEF_LIGATURE(1) // LIGATURE1, *PLIGATURE1;
TYPEDEF_LIGATURE(2) // LIGATURE2, *PLIGATURE2;
TYPEDEF_LIGATURE(3) // LIGATURE3, *PLIGATURE3;
TYPEDEF_LIGATURE(4) // LIGATURE4, *PLIGATURE4;
TYPEDEF_LIGATURE(5) // LIGATURE5, *PLIGATURE5;

/***************************************************************************\
* VSC_LPWSTR - associate a Virtual Scancode with a Text string
*
* Uses:
*   GetKeyNameText(), aKeyNames[]  Map virtual scancode to name of key
*
\***************************************************************************/
typedef struct {
    BYTE   vsc;
    LPWSTR pwsz;
} VSC_LPWSTR, *PVSC_LPWSTR;

/*
 * Along with ligature support we're adding a proper version number.
 * The previous version number (actually just unused bits...) was
 * always zero.  The version number will live in the high word of
 * fLocaleFlags.
 */
#define KBD_VERSION         1
#define GET_KBD_VERSION(p)  (HIWORD((p)->fLocaleFlags))

/*
 * Attributes such as AltGr, LRM_RLM, ShiftLock are stored in the the low word
 * of fLocaleFlags (layout specific) or in gdwKeyboardAttributes (all layouts)
 */
#define KLLF_ALTGR       0x0001
#define KLLF_SHIFTLOCK   0x0002
#define KLLF_LRM_RLM     0x0004

/*
 * Some attributes are per-layout (specific to an individual layout), some
 * attributes are per-user (apply globally to all layouts).  Some are both.
 */
#define KLLF_LAYOUT_ATTRS (KLLF_SHIFTLOCK | KLLF_ALTGR | KLLF_LRM_RLM)
#define KLLF_GLOBAL_ATTRS (KLLF_SHIFTLOCK)

/*
 * Flags passed in to the KeyboardLayout API (KLF_*) as can be converted to
 * internal (KLLF_*) attributes:
 */
#define KLL_ATTR_FROM_KLF(x)         ((x) >> 15)
#define KLL_LAYOUT_ATTR_FROM_KLF(x)  (KLL_ATTR_FROM_KLF(x) & KLLF_LAYOUT_ATTRS)
#define KLL_GLOBAL_ATTR_FROM_KLF(x)  (KLL_ATTR_FROM_KLF(x) & KLLF_GLOBAL_ATTRS)

/*
 * If KLF_SHIFTLOCK & KLF_LRM_RLM are defined, we can check the KLLF_* values
 */
#ifdef KLF_SHIFTLOCK
#if KLLF_SHIFTLOCK != KLL_ATTR_FROM_KLF(KLF_SHIFTLOCK)
    #error KLLF_SHIFTLOCK != KLL_ATTR_FROM_KLF(KLF_SHIFTLOCK)
#endif
#endif // KLF_SHIFTLOCK
#ifdef KLF_LRM_RLM
#if KLLF_LRM_RLM != KLL_ATTR_FROM_KLF(KLF_LRM_RLM)
    #error KLLF_LRM_RLM != KLL_ATTR_FROM_KLF(KLF_LRM_RLM)
#endif
#endif // KLF_LRM_RLM

/***************************************************************************\
* KBDTABLES
*
* This structure describes all the tables that implement the keyboard layer.
*
* When switching to a new layer, we get a new KBDTABLES structure: all key
* processing tables are accessed indirectly through this structure.
*
\***************************************************************************/

typedef struct tagKbdLayer {
    /*
     * Modifier keys
     */
    PMODIFIERS pCharModifiers;

    /*
     * Characters
     */
    VK_TO_WCHAR_TABLE *pVkToWcharTable;  // ptr to tbl of ptrs to tbl

    /*
     * Diacritics
     */
    PDEADKEY pDeadKey;

    /*
     * Names of Keys
     */
    VSC_LPWSTR *pKeyNames;
    VSC_LPWSTR *pKeyNamesExt;
    LPWSTR     *pKeyNamesDead;

    /*
     * Scan codes to Virtual Keys
     */
    USHORT *pusVSCtoVK;
    BYTE    bMaxVSCtoVK;
    PVSC_VK pVSCtoVK_E0;  // Scancode has E0 prefix
    PVSC_VK pVSCtoVK_E1;  // Scancode has E1 prefix

    /*
     * Locale-specific special processing
     */
    DWORD fLocaleFlags;

    /*
     * Ligatures
     */
    BYTE       nLgMax;
    BYTE       cbLgEntry;
    PLIGATURE1 pLigature;
} KBDTABLES, *PKBDTABLES;

/*
 * OEM-specific special processing (keystroke simulators and filters)
 */
extern KEPROC aKEProcOEM[];

/*
 * FarEast-specific special...
 */
typedef struct _VK_FUNCTION_PARAM {
    BYTE  NLSFEProcIndex;
    ULONG NLSFEProcParam;
} VK_FPARAM, *PVK_FPARAM;

typedef struct _VK_TO_FUNCTION_TABLE {
    BYTE Vk;
    BYTE NLSFEProcType;
    BYTE NLSFEProcCurrent;
    // Index[0] : Base
    // Index[1] : Shift
    // Index[2] : Control
    // Index[3] : Shift+Control
    // Index[4] : Alt
    // Index[5] : Shift+Alt
    // Index[6] : Control+Alt
    // Index[7] : Shift+Control+Alt
    BYTE NLSFEProcSwitch;   // 8 bits
    VK_FPARAM NLSFEProc[8];
    VK_FPARAM NLSFEProcAlt[8];
} VK_F, *PVK_F;

typedef struct tagKbdNlsLayer {
    USHORT OEMIdentifier;
    USHORT LayoutInformation;
    UINT  NumOfVkToF;
    VK_F   *pVkToF;
    //
    // The pusMouseVKey array provides a translation from the virtual key
    // value to an index.  The index is used to select the appropriate
    // routine to process the virtual key, as well as to select extra
    // information that is used by this routine during its processing.
    // If this value is NULL, following default will be used.
    //
    // ausMouseVKey[] = {
    //     VK_CLEAR,           // Numpad 5: Click active button
    //     VK_PRIOR,           // Numpad 9: Up & Right
    //     VK_NEXT,            // Numpad 3: Down & Right
    //     VK_END,             // Numpad 1: Down & Left
    //     VK_HOME,            // Numpad 7: Up & Left
    //     VK_LEFT,            // Numpad 4: Left
    //     VK_UP,              // Numpad 8: Up
    //     VK_RIGHT,           // Numpad 6: Right
    //     VK_DOWN,            // Numpad 2: Down
    //     VK_INSERT,          // Numpad 0: Active button down
    //     VK_DELETE,          // Numpad .: Active button up
    //     VK_MULTIPLY,        // Numpad *: Select both buttons
    //     VK_ADD,             // Numpad +: Double click active button
    //     VK_SUBTRACT,        // Numpad -: Select right button
    //     VK_DEVIDE|KBDEXT,   // Numpad /: Select left button
    //     VK_NUMLOCK|KBDEXT}; // Num Lock
    //
    INT     NumOfMouseVKey;
    USHORT *pusMouseVKey;
} KBDNLSTABLES, *PKBDNLSTABLES;

//
// OEM Ids - KBDNLSTABLES.OEMIdentifier
//
// PSS ID Number: Q130054
// Article last modified on 05-16-1995
//
// 3.10 1.20 | 3.50 1.20
// WINDOWS   | WINDOWS NT
//
// ---------------------------------------------------------------------
// The information in this article applies to:
// - Microsoft Windows Software Development Kit (SDK) for Windows
//   version 3.1
// - Microsoft Win32 Software Development Kit (SDK) version 3.5
// - Microsoft Win32s version 1.2
// ---------------------------------------------------------------------
// SUMMARY
// =======
// Because of the variety of computer manufacturers (NEC, Fujitsu, IBMJ, and
// so on) in Japan, sometimes Windows-based applications need to know which
// OEM (original equipment manufacturer) manufactured the computer that is
// running the application. This article explains how.
//
// MORE INFORMATION
// ================
// There is no documented way to detect the manufacturer of the computer that
// is currently running an application. However, a Windows-based application
// can detect the type of OEM Windows by using the return value of the
// GetKeyboardType() function.
//
// If an application uses the GetKeyboardType API, it can get OEM ID by
// specifying "1" (keyboard subtype) as argument of the function. Each OEM ID
// is listed here:
//
// OEM Windows       OEM ID
// ------------------------------
// Microsoft         00H (DOS/V)
// all AX            01H
// EPSON             04H
// Fujitsu           05H
// IBMJ              07H
// Matsushita        0AH
// NEC               0DH
// Toshiba           12H
//
// Application programs can use these OEM IDs to distinguish the type of OEM
// Windows. Note, however, that this method is not documented, so Microsoft
// may not support it in the future version of Windows.
//
// As a rule, application developers should write hardware-independent code,
// especially when making Windows-based applications. If they need to make a
// hardware-dependent application, they must prepare the separated program
// file for each different hardware architecture.
//
// Additional reference words: 3.10 1.20 3.50 1.20 kbinf
// KBCategory: kbhw
// KBSubcategory: wintldev
// =============================================================================
// Copyright Microsoft Corporation 1995.
//
#define NLSKBD_OEM_MICROSOFT          0x00
#define NLSKBD_OEM_AX                 0x01
#define NLSKBD_OEM_EPSON              0x04
#define NLSKBD_OEM_FUJITSU            0x05
#define NLSKBD_OEM_IBM                0x07
#define NLSKBD_OEM_MATSUSHITA         0x0A
#define NLSKBD_OEM_NEC                0x0D
#define NLSKBD_OEM_TOSHIBA            0x12
#define NLSKBD_OEM_DEC                0x18 // only NT
//
// Microsoft (default) - keyboards hardware/layout
//
#define MICROSOFT_KBD_101_TYPE           0
#define MICROSOFT_KBD_AX_TYPE            1
#define MICROSOFT_KBD_106_TYPE           2
#define MICROSOFT_KBD_002_TYPE           3
#define MICROSOFT_KBD_001_TYPE           4
#define MICROSOFT_KBD_FUNC              12
//
// AX consortium - keyboards hardware/layout
//
#define AX_KBD_DESKTOP_TYPE              1
//
// Fujitsu - keyboards hardware/layout
//
#define FMR_KBD_JIS_TYPE                 0
#define FMR_KBD_OASYS_TYPE               1
#define FMV_KBD_OASYS_TYPE               2
//
// NEC - keyboards hardware/layout
//
#define NEC_KBD_NORMAL_TYPE              1
#define NEC_KBD_N_MODE_TYPE              2
#define NEC_KBD_H_MODE_TYPE              3
#define NEC_KBD_LAPTOP_TYPE              4
#define NEC_KBD_106_TYPE                 5
//
// Toshiba - keyboards hardware/layout
//
#define TOSHIBA_KBD_DESKTOP_TYPE        13
#define TOSHIBA_KBD_LAPTOP_TYPE         15
//
// DEC - keyboards hardware/layout
//
#define DEC_KBD_ANSI_LAYOUT_TYPE         1 // only NT
#define DEC_KBD_JIS_LAYOUT_TYPE          2 // only NT

//
// Keyboard layout information - KBDNLSTABLE.LayoutInformation
//

//
// If this flag is on, System sends notification to keyboard
// drivers (leyout/kernel mode). when IME (Input-Mehod-Editor)
// status become changed.
//
#define NLSKBD_INFO_SEND_IME_NOTIFICATION  0x0001

//
// If this flag is on, System will use VK_HOME/VK_KANA instead of
// VK_NUMLOCK/VK_SCROLL for Accessibility toggle keys.
// + Typically, NEC PC-9800 Series will use this bit, because
//   they does not have 'NumLock' and 'ScrollLock' keys.
//
#define NLSKBD_INFO_ACCESSIBILITY_KEYMAP   0x0002

//
// If this flag is on, System will return 101 or 106 Japanese
// keyboard type/subtype id, when GetKeyboardType() is called.
//
#define NLSKBD_INFO_EMURATE_101_KEYBOARD   0x0010
#define NLSKBD_INFO_EMURATE_106_KEYBOARD   0x0020

//
// Keyboard layout function types
//
// - VK_F.NLSFEProcType
//
#define KBDNLS_TYPE_NULL      0
#define KBDNLS_TYPE_NORMAL    1
#define KBDNLS_TYPE_TOGGLE    2

//
// - VK_F.NLSFEProcCurrent
//
#define KBDNLS_INDEX_NORMAL   1
#define KBDNLS_INDEX_ALT      2

//
// - VK_F.NLSFEProc[]
//
#define KBDNLS_NULL             0 // Invalid function
#define KBDNLS_NOEVENT          1 // Drop keyevent
#define KBDNLS_SEND_BASE_VK     2 // Send Base VK_xxx
#define KBDNLS_SEND_PARAM_VK    3 // Send Parameter VK_xxx
#define KBDNLS_KANALOCK         4 // VK_KANA (with hardware lock)
#define KBDNLS_ALPHANUM         5 // VK_DBE_ALPHANUMERIC
#define KBDNLS_HIRAGANA         6 // VK_DBE_HIRAGANA
#define KBDNLS_KATAKANA         7 // VK_DBE_KATAKANA
#define KBDNLS_SBCSDBCS         8 // VK_DBE_SBCSCHAR/VK_DBE_DBCSCHAR
#define KBDNLS_ROMAN            9 // VK_DBE_ROMAN/VK_DBE_NOROMAN
#define KBDNLS_CODEINPUT       10 // VK_DBE_CODEINPUT/VK_DBE_NOCODEINPUT
#define KBDNLS_HELP_OR_END     11 // VK_HELP or VK_END [NEC PC-9800 Only]
#define KBDNLS_HOME_OR_CLEAR   12 // VK_HOME or VK_CLEAR [NEC PC-9800 Only]
#define KBDNLS_NUMPAD          13 // VK_NUMPAD? for Numpad key [NEC PC-9800 Only]
#define KBDNLS_KANAEVENT       14 // VK_KANA [Fujitsu FMV oyayubi Only]
#define KBDNLS_CONV_OR_NONCONV 15 // VK_CONVERT and VK_NONCONVERT [Fujitsu FMV oyayubi Only]

typedef BOOL (* NLSKEPROC)(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam);
typedef BOOL (* NLSVKFPROC)(PVK_F pVkToF, PKE pKe, ULONG_PTR dwExtraInfo);

//
// Keyboard Type = 7 : Japanese Keyboard
// Keyboard Type = 8 : Korean Keyboard
//
#define JAPANESE_KEYBOARD(Id)  ((Id).Type == 7)
#define KOREAN_KEYBOARD(Id)    ((Id).Type == 8)

// Fujitsu Oyayubi-shift keyboard
#define FUJITSU_KBD_CONSOLE(Id)  (JAPANESE_KEYBOARD(Id) && \
                                  (Id).Subtype == ((NLSKBD_OEM_FUJITSU<<4)|FMV_KBD_OASYS_TYPE))
        // This number 0x00020002 is registered in registry key as
        // HKLM\System\CurrentControlSet\Control\Terminal Server\KeyboardType Mapping\JPN
#define FUJITSU_KBD_REMOTE(Id)   (JAPANESE_KEYBOARD(Id) && \
                                  (Id).SubType == 0x00020002)

#define KBD_LAYOUT_LANG(hkl)    (LOBYTE(LOWORD(HandleToUlong(hkl))))

#define JAPANESE_KBD_LAYOUT(hkl)    (KBD_LAYOUT_LANG(hkl) == LANG_JAPANESE)
#define KOREAN_KBD_LAYOUT(hkl)      (KBD_LAYOUT_LANG(hkl) == LANG_KOREAN)

//
// NLS Keyboard functions
//
VOID NlsKbdInitializePerSystem(VOID);
VOID NlsKbdSendIMENotification(DWORD dwImeOpen, DWORD dwImeConversion);

// end of FE specific

/***************************************************************************\
* Macros for ausVK[] values (used below)
*
* These macros prefix each argument with VK_ to produce the name of a Virtual
* Key defined in "winuser.h" (eg: ESCAPE becomes VK_ESCAPE).
\***************************************************************************/
#ifndef KBD_TYPE
#define KBD_TYPE 4
#endif

/*
 * _NE() selects the Virtual Key according to keyboard type
 */
#if   (KBD_TYPE == 1)
#define _NE(v1,v2,v3,v4,v5,v6) (VK_##v1)
#elif (KBD_TYPE == 2)
#define _NE(v1,v2,v3,v4,v5,v6) (VK_##v2)
#elif (KBD_TYPE == 3)
#define _NE(v1,v2,v3,v4,v5,v6) (VK_##v3)
#elif (KBD_TYPE == 4)
#define _NE(v1,v2,v3,v4,v5,v6) (VK_##v4)
#elif (KBD_TYPE == 5)
#define _NE(v1,v2,v3,v4,v5,v6) (VK_##v5)
#elif (KBD_TYPE == 6)
#define _NE(v1,v2,v3,v4,v5,v6) (VK_##v6)
#elif (KBD_TYPE == 7)
#define _NE(v7,v8,v16,v10,v11,v12,v13) (VK_##v7)
#elif (KBD_TYPE == 8)
#define _NE(v7,v8,v16,v10,v11,v12,v13) (VK_##v8)
#elif (KBD_TYPE == 10)
#define _NE(v7,v8,v16,v10,v11,v12,v13) (VK_##v10)
#elif (KBD_TYPE == 11)
#define _NE(v7,v8,v16,v10,v11,v12,v13) (VK_##v11)
#elif (KBD_TYPE == 12)
#define _NE(v7,v8,v16,v10,v11,v12,v13) (VK_##v12)
#elif (KBD_TYPE == 13)
#define _NE(v7,v8,v16,v10,v11,v12,v13) (VK_##v13)
#elif (KBD_TYPE == 16)
#define _NE(v7,v8,v16,v10,v11,v12,v13) (VK_##v16)
#elif (KBD_TYPE == 20)
#define _NE(v20,v21,v22)           (VK_##v20)
#elif (KBD_TYPE == 21)
#define _NE(v20,v21,v22)           (VK_##v21)
#elif (KBD_TYPE == 22)
#define _NE(v20,v21,v22)           (VK_##v22)
#elif (KBD_TYPE == 30)
#define _NE(v30,v33,v34)           (VK_##v30)
#elif (KBD_TYPE == 33)
#define _NE(v30,v33,v34)           (VK_##v33)
#elif (KBD_TYPE == 34)
#define _NE(v30,v33,v34)           (VK_##v34)
#elif (KBD_TYPE == 40)
#define _NE(v40,v41)               (VK_##v40)
#elif (KBD_TYPE == 41)
#define _NE(v40,v41)               (VK_##v41)
#endif

/*
 * _EQ() selects the same Virtual Key for all keyboard types
 */
#if   (KBD_TYPE <= 6)
#define _EQ(         v4      ) (VK_##v4)
#elif (KBD_TYPE >= 7) && (KBD_TYPE <= 16)
#define _EQ(   v8            ) (VK_##v8)
#elif (KBD_TYPE > 20) && (KBD_TYPE <= 22)
#define _EQ(v20              ) (VK_##v20)
#elif (KBD_TYPE >= 30) && (KBD_TYPE <= 34)
#define _EQ(         v30     ) (VK_##v30)
#elif (KBD_TYPE == 37)
#define _EQ(         v37     ) (VK_##v37)
#elif (KBD_TYPE >= 40) && (KBD_TYPE <= 41)
#define _EQ( v40             ) (VK_##v40)
#endif

/*
 * A bit of trickery for virtual key names 'A' to 'Z' and '0' to '9' so
 * that they are not converted to a VK_* name.
 * With this macro, VK_'A' equates to 'A' etc.
 */
#define VK_
#define VK__none_   0xFF
#define VK_ABNT_C1  0xC1
#define VK_ABNT_C2  0xC2

#if (KBD_TYPE <= 6)
/***************************************************************************\
* T** - Values for ausVK[] (Virtual Scan Code to Virtual Key conversion)
*
* These values are for Scancode Set 3 and the USA.
* Other languages substitute their own values where required (files kbd**.h)
*
* Six sets of keyboards are supported, according to KBD_TYPE:
*
* KBD_TYPE   Keyboard (examples)
* ========   =======================================================
*    1       AT&T '301' & '302'; Olivetti 83-key; PC-XT 84-key; etc.
*    2       Olivetti M24 102-key
*    3       HP Vectra (DIN); Olivetti 86-key; etc.
*    4 *     Enhanced 101/102-key; Olivetti A; etc.
*    5       Nokia (Ericsson) type 5 (1050, etc.)
*    6       Nokia (Ericsson) type 6 (9140)
*
* * If KBD_TYPE is not defined, the default is type 4.
*
* KB3270 comments refers to KB 3270 keyboards in native emulation mode (DIP
* switches all OFF), and the Scancode Map used to convert its scancodes to
* standard scancode set 1.
*    KB3270 <= 57      - this entry is reached by mapping from scancode 0x57
*                        to an arbitrary scancode: the VK is what counts
*    KB3270 => HOME    - this scancode is mapped to the scancode for VK_HOME
*    KB3270            - no mapping involved, a scancode for KB3270 only
*
* _EQ() : all keyboard types have the same virtual key for this scancode
* _NE() : different virtual keys for this scancode, depending on kbd type
*
*     +------+ +--------+--------+--------+--------+--------+--------+
*     | Scan | |  kbd   |  kbd   |  kbd   |  kbd   |  kbd   |  kbd   |
*     | code | | type 1 | type 2 | type 3 | type 4 | type 5 | type 6 |
\****+-------+-+--------+--------+--------+--------+--------+--------+******/
#define T00 _EQ(                           _none_                    )
#define T01 _EQ(                           ESCAPE                    )
#define T02 _EQ(                           '1'                       )
#define T03 _EQ(                           '2'                       )
#define T04 _EQ(                           '3'                       )
#define T05 _EQ(                           '4'                       )
#define T06 _EQ(                           '5'                       )
#define T07 _EQ(                           '6'                       )
#define T08 _EQ(                           '7'                       )
#define T09 _EQ(                           '8'                       )
#define T0A _EQ(                           '9'                       )
#define T0B _EQ(                           '0'                       )
#define T0C _EQ(                           OEM_MINUS                 )
#define T0D _NE(OEM_PLUS,OEM_4,   OEM_PLUS,OEM_PLUS,OEM_PLUS,OEM_PLUS)
#define T0E _EQ(                           BACK                      )
#define T0F _EQ(                           TAB                       )
#define T10 _EQ(                           'Q'                       )
#define T11 _EQ(                           'W'                       )
#define T12 _EQ(                           'E'                       )
#define T13 _EQ(                           'R'                       )
#define T14 _EQ(                           'T'                       )
#define T15 _EQ(                           'Y'                       )
#define T16 _EQ(                           'U'                       )
#define T17 _EQ(                           'I'                       )
#define T18 _EQ(                           'O'                       )
#define T19 _EQ(                           'P'                       )
#define T1A _NE(OEM_4,   OEM_6,   OEM_4,   OEM_4,   OEM_4,   OEM_4   )
#define T1B _NE(OEM_6,   OEM_1,   OEM_6,   OEM_6,   OEM_6,   OEM_6   )
#define T1C _EQ(                           RETURN                    )
#define T1D _EQ(                           LCONTROL                  )
#define T1E _EQ(                           'A'                       )
#define T1F _EQ(                           'S'                       )
#define T20 _EQ(                           'D'                       )
#define T21 _EQ(                           'F'                       )
#define T22 _EQ(                           'G'                       )
#define T23 _EQ(                           'H'                       )
#define T24 _EQ(                           'J'                       )
#define T25 _EQ(                           'K'                       )
#define T26 _EQ(                           'L'                       )
#define T27 _NE(OEM_1,   OEM_PLUS,OEM_1,   OEM_1,   OEM_1,   OEM_1   )
#define T28 _NE(OEM_7,   OEM_3,   OEM_7,   OEM_7,   OEM_3,   OEM_3   )
#define T29 _NE(OEM_3,   OEM_7,   OEM_3,   OEM_3,   OEM_7,   OEM_7   )
#define T2A _EQ(                           LSHIFT                    )
#define T2B _EQ(                           OEM_5                     )
#define T2C _EQ(                           'Z'                       )
#define T2D _EQ(                           'X'                       )
#define T2E _EQ(                           'C'                       )
#define T2F _EQ(                           'V'                       )
#define T30 _EQ(                           'B'                       )
#define T31 _EQ(                           'N'                       )
#define T32 _EQ(                           'M'                       )
#define T33 _EQ(                           OEM_COMMA                 )
#define T34 _EQ(                           OEM_PERIOD                )
#define T35 _EQ(                           OEM_2                     )
#define T36 _EQ(                           RSHIFT                    )
#define T37 _EQ(                           MULTIPLY                  )
#define T38 _EQ(                           LMENU                     )
#define T39 _EQ(                           ' '                       )
#define T3A _EQ(                           CAPITAL                   )
#define T3B _EQ(                           F1                        )
#define T3C _EQ(                           F2                        )
#define T3D _EQ(                           F3                        )
#define T3E _EQ(                           F4                        )
#define T3F _EQ(                           F5                        )
#define T40 _EQ(                           F6                        )
#define T41 _EQ(                           F7                        )
#define T42 _EQ(                           F8                        )
#define T43 _EQ(                           F9                        )
#define T44 _EQ(                           F10                       )
#define T45 _EQ(                           NUMLOCK                   )
#define T46 _EQ(                           SCROLL                    )
#define T47 _EQ(                           HOME                      )
#define T48 _EQ(                           UP                        )
#define T49 _EQ(                           PRIOR                     )
#define T4A _EQ(                           SUBTRACT                  )
#define T4B _EQ(                           LEFT                      )
#define T4C _EQ(                           CLEAR                     )
#define T4D _EQ(                           RIGHT                     )
#define T4E _EQ(                           ADD                       )
#define T4F _EQ(                           END                       )
#define T50 _EQ(                           DOWN                      )
#define T51 _EQ(                           NEXT                      )
#define T52 _EQ(                           INSERT                    )
#define T53 _EQ(                           DELETE                    )
#define T54 _EQ(                           SNAPSHOT                  )
#define T55 _EQ(                           _none_                    ) // KB3270 => DOWN
#define T56 _NE(OEM_102, HELP,    OEM_102, OEM_102, _none_,  OEM_PA2 ) // KB3270 => LEFT
#define T57 _NE(F11,     RETURN,  F11,     F11,     _none_,  HELP    ) // KB3270 => ZOOM
#define T58 _NE(F12,     LEFT,    F12,     F12,     _none_,  OEM_102 ) // KB3270 => HELP
#define T59 _EQ(                           CLEAR                     )
#define T5A _EQ(                           OEM_WSCTRL                )// WSCtrl
#define T5B _EQ(                           OEM_FINISH                )// Finish
#define T5C _EQ(                           OEM_JUMP                  )// Jump
#define T5D _EQ(                           EREOF                     )
#define T5E _EQ(                           OEM_BACKTAB               ) // KB3270 <= 7E
#define T5F _EQ(                           OEM_AUTO                  ) // KB3270
#define T60 _EQ(                           _none_                    )
#define T61 _EQ(                           _none_                    )
#define T62 _EQ(                           ZOOM                      ) // KB3270 <= 57
#define T63 _EQ(                           HELP                      ) // KB3270 <= 58
#define T64 _EQ(                           F13                       )
#define T65 _EQ(                           F14                       )
#define T66 _EQ(                           F15                       )
#define T67 _EQ(                           F16                       )
#define T68 _EQ(                           F17                       )
#define T69 _EQ(                           F18                       )
#define T6A _EQ(                           F19                       )
#define T6B _EQ(                           F20                       )
#define T6C _EQ(                           F21                       )
#define T6D _EQ(                           F22                       )
#define T6E _EQ(                           F23                       )
#define T6F _EQ(                           OEM_PA3                   ) // KB3270
#define T70 _EQ(                           _none_                    )
#define T71 _EQ(                           OEM_RESET                 ) // KB3270
#define T72 _EQ(                           _none_                    )
#define T73 _EQ(                           ABNT_C1                   )
#define T74 _EQ(                           _none_                    )
#define T75 _EQ(                           _none_                    ) // KB3270 => RETURN
#define T76 _EQ(                           F24                       )
#define T77 _EQ(                           _none_                    ) // KB3270 => HOME
#define T78 _EQ(                           _none_                    ) // KB3270 => UP
#define T79 _EQ(                           _none_                    ) // KB3270 => DELETE
#define T7A _EQ(                           _none_                    ) // KB3270 => INSERT
#define T7B _EQ(                           OEM_PA1                   ) // KB3270
#define T7C _EQ(                           TAB                       ) // KB3270 => TAB
#define T7D _EQ(                           _none_                    ) // KB3270 => RIGHT
#define T7E _EQ(                           ABNT_C2                   ) // KB3270 => BACKTAB
#define T7F _EQ(                           OEM_PA2                   ) // KB3270

#define X10 _EQ(                           MEDIA_PREV_TRACK          ) // SpeedRacer
#define X19 _EQ(                           MEDIA_NEXT_TRACK          ) // SpeedRacer
#define X1C _EQ(                           RETURN                    )
#define X1D _EQ(                           RCONTROL                  )
#define X20 _EQ(                           VOLUME_MUTE               ) // SpeedRacer
#define X21 _EQ(                           LAUNCH_APP2               ) // SpeedRacer (Calculator?)
#define X22 _EQ(                           MEDIA_PLAY_PAUSE          ) // SpeedRacer
#define X24 _EQ(                           MEDIA_STOP                ) // SpeedRacer
#define X2E _EQ(                           VOLUME_DOWN               ) // SpeedRacer
#define X30 _EQ(                           VOLUME_UP                 ) // SpeedRacer
#define X32 _EQ(                           BROWSER_HOME              ) // SpeedRacer
#define X35 _EQ(                           DIVIDE                    )
#define X37 _EQ(                           SNAPSHOT                  )
#define X38 _EQ(                           RMENU                     )
#define X46 _EQ(                           CANCEL                    )
#define X47 _EQ(                           HOME                      )
#define X48 _EQ(                           UP                        )
#define X49 _EQ(                           PRIOR                     )
#define X4B _EQ(                           LEFT                      )
#define X4D _EQ(                           RIGHT                     )
#define X4F _EQ(                           END                       )
#define X50 _EQ(                           DOWN                      )
#define X51 _NE(NEXT,    F1,      NEXT,    NEXT,    _none_, OEM_PA2  )
#define X52 _EQ(                           INSERT                    )
#define X53 _EQ(                           DELETE                    )
#define X5B _EQ(                           LWIN                      )
#define X5C _EQ(                           RWIN                      )
#define X5D _EQ(                           APPS                      )
#define X5E _EQ(                           POWER                     )
#define X5F _EQ(                           SLEEP                     ) // SpeedRacer
#define X65 _EQ(                           BROWSER_SEARCH            ) // SpeedRacer
#define X66 _EQ(                           BROWSER_FAVORITES         ) // SpeedRacer
#define X67 _EQ(                           BROWSER_REFRESH           ) // SpeedRacer
#define X68 _EQ(                           BROWSER_STOP              ) // SpeedRacer
#define X69 _EQ(                           BROWSER_FORWARD           ) // SpeedRacer
#define X6A _EQ(                           BROWSER_BACK              ) // SpeedRacer
#define X6B _EQ(                           LAUNCH_APP1               ) // SpeedRacer (my computer?)
#define X6C _EQ(                           LAUNCH_MAIL               ) // SpeedRacer
#define X6D _EQ(                           LAUNCH_MEDIA_SELECT       ) // SpeedRacer


        /*
         * The break key is sent to us as E1,LCtrl,NumLock
         * We must convert the E1+LCtrl to BREAK, then ignore the Numlock
         */
#define Y1D _EQ(                           PAUSE                     )

#elif (KBD_TYPE >= 7) && (KBD_TYPE <= 16)
/***********************************************************************************\
* T** - Values for ausVK[] (Virtual Scan Code to Virtual Key conversion)
*
* Three sets of keyboards are supported, according to KBD_TYPE:
*
* KBD_TYPE   Keyboard (examples)
* ========   =====================================
*    7       Japanese IBM type 002 keyboard.
*    8 *     Japanese OADG (106) keyboard.
*   10       Korean 101 (type A) keyboard.
*   11       Korean 101 (type B) keyboard.
*   12       Korean 101 (type C) keyboard.
*   13       Korean 103 keyboard.
*   16       Japanese AX keyboard.
*
*     +------+ +----------+----------+----------+----------+----------+----------+----------+
*     | Scan | |   kbd    |   kbd    |   kbd    |   kbd    |   kbd    |   kbd    |   kbd    |
*     | code | |  type 7  |  type 8  |  type 16 |  type 10 |  type 11 |  type 12 |  type 13 |
\****+-------+-+----------+----------+----------+----------+----------+----------+----------+*/
#define T00 _EQ(           _none_                                                           )
#define T01 _EQ(           ESCAPE                                                           )
#define T02 _EQ(           '1'                                                              )
#define T03 _EQ(           '2'                                                              )
#define T04 _EQ(           '3'                                                              )
#define T05 _EQ(           '4'                                                              )
#define T06 _EQ(           '5'                                                              )
#define T07 _EQ(           '6'                                                              )
#define T08 _EQ(           '7'                                                              )
#define T09 _EQ(           '8'                                                              )
#define T0A _EQ(           '9'                                                              )
#define T0B _EQ(           '0'                                                              )
#define T0C _EQ(           OEM_MINUS                                                        )
#define T0D _NE(OEM_7,     OEM_7,     OEM_PLUS,  OEM_PLUS,  OEM_PLUS,  OEM_PLUS,  OEM_PLUS  )
#define T0E _EQ(           BACK                                                             )
#define T0F _EQ(           TAB                                                              )
#define T10 _EQ(           'Q'                                                              )
#define T11 _EQ(           'W'                                                              )
#define T12 _EQ(           'E'                                                              )
#define T13 _EQ(           'R'                                                              )
#define T14 _EQ(           'T'                                                              )
#define T15 _EQ(           'Y'                                                              )
#define T16 _EQ(           'U'                                                              )
#define T17 _EQ(           'I'                                                              )
#define T18 _EQ(           'O'                                                              )
#define T19 _EQ(           'P'                                                              )
#define T1A _NE(OEM_4,     OEM_3,     OEM_4,     OEM_4,     OEM_4,     OEM_4,     OEM_4     )
#define T1B _NE(OEM_6,     OEM_4,     OEM_6,     OEM_6,     OEM_6,     OEM_6,     OEM_6     )
#define T1C _EQ(           RETURN                                                           )
#define T1D _EQ(           LCONTROL                                                         )
#define T1E _EQ(           'A'                                                              )
#define T1F _EQ(           'S'                                                              )
#define T20 _EQ(           'D'                                                              )
#define T21 _EQ(           'F'                                                              )
#define T22 _EQ(           'G'                                                              )
#define T23 _EQ(           'H'                                                              )
#define T24 _EQ(           'J'                                                              )
#define T25 _EQ(           'K'                                                              )
#define T26 _EQ(           'L'                                                              )
#define T27 _NE(OEM_PLUS,  OEM_PLUS,  OEM_1,     OEM_1,     OEM_1,     OEM_1,     OEM_1     )
#define T28 _NE(OEM_1,     OEM_1,     OEM_7,     OEM_7,     OEM_7,     OEM_7,     OEM_7     )
#define T29 _NE(OEM_3,     DBE_SBCSCHAR,OEM_3,   OEM_3,     OEM_3,     OEM_3,     OEM_3     )
#define T2A _EQ(           LSHIFT                                                           )
#define T2B _NE(OEM_5,     OEM_6,     OEM_5,     OEM_5,     OEM_5,     OEM_5,     OEM_5     )
#define T2C _EQ(           'Z'                                                              )
#define T2D _EQ(           'X'                                                              )
#define T2E _EQ(           'C'                                                              )
#define T2F _EQ(           'V'                                                              )
#define T30 _EQ(           'B'                                                              )
#define T31 _EQ(           'N'                                                              )
#define T32 _EQ(           'M'                                                              )
#define T33 _EQ(           OEM_COMMA                                                        )
#define T34 _EQ(           OEM_PERIOD                                                       )
#define T35 _EQ(           OEM_2                                                            )
#define T36 _EQ(           RSHIFT                                                           )
#define T37 _EQ(           MULTIPLY                                                         )
#define T38 _EQ(           LMENU                                                            )
#define T39 _EQ(           ' '                                                              )
#define T3A _NE(DBE_ALPHANUMERIC,DBE_ALPHANUMERIC,CAPITAL,CAPITAL,CAPITAL,CAPITAL,CAPITAL   )
#define T3B _EQ(           F1                                                               )
#define T3C _EQ(           F2                                                               )
#define T3D _EQ(           F3                                                               )
#define T3E _EQ(           F4                                                               )
#define T3F _EQ(           F5                                                               )
#define T40 _EQ(           F6                                                               )
#define T41 _EQ(           F7                                                               )
#define T42 _EQ(           F8                                                               )
#define T43 _EQ(           F9                                                               )
#define T44 _EQ(           F10                                                              )
#define T45 _EQ(           NUMLOCK                                                          )
#define T46 _EQ(           SCROLL                                                           )
#define T47 _EQ(           HOME                                                             )
#define T48 _EQ(           UP                                                               )
#define T49 _EQ(           PRIOR                                                            )
#define T4A _EQ(           SUBTRACT                                                         )
#define T4B _EQ(           LEFT                                                             )
#define T4C _EQ(           CLEAR                                                            )
#define T4D _EQ(           RIGHT                                                            )
#define T4E _EQ(           ADD                                                              )
#define T4F _EQ(           END                                                              )
#define T50 _EQ(           DOWN                                                             )
#define T51 _EQ(           NEXT                                                             )
#define T52 _EQ(           INSERT                                                           )
#define T53 _EQ(           DELETE                                                           )
#define T54 _EQ(           SNAPSHOT                                                         )
#define T55 _EQ(           _none_                                                           )
#define T56 _NE(_none_,    _none_,    OEM_102,   OEM_102,   OEM_102,   OEM_102,   OEM_102   )
#define T57 _EQ(           F11                                                              )
#define T58 _EQ(           F12                                                              )
#define T59 _EQ(           CLEAR                                                            )
#define T5A _NE(NONAME,    NONAME,    NONCONVERT,OEM_WSCTRL,OEM_WSCTRL,OEM_WSCTRL,OEM_WSCTRL)
#define T5B _NE(NONAME,    NONAME,    CONVERT,   OEM_FINISH,OEM_FINISH,OEM_FINISH,OEM_FINISH)
#define T5C _NE(NONAME,    NONAME,    OEM_AX,    OEM_JUMP,  OEM_JUMP,  OEM_JUMP,  OEM_JUMP  )
#define T5D _EQ(           EREOF                                                            )
#define T5E _NE(_none_,    _none_,    _none_,    OEM_BACKTAB,OEM_BACKTAB,OEM_BACKTAB,OEM_BACKTAB)
#define T5F _NE(NONAME,    NONAME,    NONAME,    OEM_AUTO,  OEM_AUTO,  OEM_AUTO,  OEM_AUTO  )
#define T60 _EQ(           _none_                                                           )
#define T61 _NE(_none_,    _none_,    _none_,    ZOOM,      ZOOM,      ZOOM,      ZOOM      )
#define T62 _NE(_none_,    _none_,    _none_,    HELP,      HELP,      HELP,      HELP      )
#define T63 _EQ(           _none_                                                           )
#define T64 _EQ(           F13                                                              )
#define T65 _EQ(           F14                                                              )
#define T66 _EQ(           F15                                                              )
#define T67 _EQ(           F16                                                              )
#define T68 _EQ(           F17                                                              )
#define T69 _EQ(           F18                                                              )
#define T6A _EQ(           F19                                                              )
#define T6B _EQ(           F20                                                              )
#define T6C _EQ(           F21                                                              )
#define T6D _EQ(           F22                                                              )
#define T6E _EQ(           F23                                                              )
#define T6F _NE(_none_,    _none_,    _none_,    OEM_PA3,   OEM_PA3,   OEM_PA3,   OEM_PA3   )
#define T70 _NE(DBE_KATAKANA,DBE_HIRAGANA,_none_,_none_,    _none_,    _none_,    _none_    )
#define T71 _NE(_none_,    _none_,    _none_,    OEM_RESET, OEM_RESET, OEM_RESET, OEM_RESET )
#define T72 _EQ(           _none_                                                           )
#define T73 _NE(OEM_102,   OEM_102,   _none_,    ABNT_C1,   ABNT_C1,   ABNT_C1,   ABNT_C1   )
#define T74 _EQ(           _none_                                                           )
#define T75 _EQ(           _none_                                                           )
#define T76 _EQ(           F24                                                              )
#define T77 _NE(DBE_SBCSCHAR,_none_,  _none_,    _none_,    _none_,    _none_,    _none_    )
#define T78 _EQ(           _none_                                                           )
#define T79 _NE(CONVERT,   CONVERT,   _none_,    _none_,    _none_,    _none_,    _none_    )
#define T7A _EQ(           _none_                                                           )
#define T7B _NE(NONCONVERT,NONCONVERT,_none_,    OEM_PA1,   OEM_PA1,   OEM_PA1,   OEM_PA1   )
#define T7C _EQ(           TAB                                                              )
#define T7D _NE(_none_,    OEM_5,     _none_,    _none_,    _none_,    _none_,    _none_    )
#define T7E _EQ(           ABNT_C2                                                          )
#define T7F _EQ(           OEM_PA2                                                          )

#define X10 _EQ(           MEDIA_PREV_TRACK                                                 ) // SpeedRacer
#define X19 _EQ(           MEDIA_NEXT_TRACK                                                 ) // SpeedRacer
#define X1C _EQ(           RETURN                                                           )
#define X1D _NE(RCONTROL,  RCONTROL,DBE_KATAKANA,HANJA,     HANGEUL,   RCONTROL,  RCONTROL  )
#define X20 _EQ(           VOLUME_MUTE                                                      ) // SpeedRacer
#define X21 _EQ(           LAUNCH_APP2                                                      ) // SpeedRacer
#define X22 _EQ(           MEDIA_PLAY_PAUSE                                                 ) // SpeedRacer
#define X24 _EQ(           MEDIA_STOP                                                       ) // SpeedRacer
#define X2E _EQ(           VOLUME_DOWN                                                      ) // SpeedRacer
#define X30 _EQ(           VOLUME_UP                                                        ) // SpeedRacer
#define X32 _EQ(           BROWSER_HOME                                                     ) // SpeedRacer
#define X33 _NE(OEM_8,     _none_,    _none_,    _none_,    _none_,    _none_,    _none_    )
#define X35 _EQ(           DIVIDE                                                           )
#define X37 _EQ(           SNAPSHOT                                                         )
#define X38 _NE(DBE_HIRAGANA,RMENU,   KANJI,     HANGEUL,   HANJA,     RMENU,     RMENU     )
#define X42 _EQ(           _none_                                                           )
#define X43 _EQ(           _none_                                                           )
#define X44 _EQ(           _none_                                                           )
#define X46 _EQ(           CANCEL                                                           )
#define X47 _EQ(           HOME                                                             )
#define X48 _EQ(           UP                                                               )
#define X49 _EQ(           PRIOR                                                            )
#define X4B _EQ(           LEFT                                                             )
#define X4D _EQ(           RIGHT                                                            )
#define X4F _EQ(           END                                                              )
#define X50 _EQ(           DOWN                                                             )
#define X51 _EQ(           NEXT                                                             )
#define X52 _EQ(           INSERT                                                           )
#define X53 _EQ(           DELETE                                                           )
#define X5B _EQ(           LWIN                                                             )
#define X5C _EQ(           RWIN                                                             )
#define X5D _EQ(           APPS                                                             )
#define X5E _EQ(           POWER                                                            )
#define X5F _EQ(           SLEEP                                                            )
#define X65 _EQ(           BROWSER_SEARCH                                                   ) // SpeedRacer
#define X66 _EQ(           BROWSER_FAVORITES                                                ) // SpeedRacer
#define X67 _EQ(           BROWSER_REFRESH                                                  ) // SpeedRacer
#define X68 _EQ(           BROWSER_STOP                                                     ) // SpeedRacer
#define X69 _EQ(           BROWSER_FORWARD                                                  ) // SpeedRacer
#define X6A _EQ(           BROWSER_BACK                                                     ) // SpeedRacer
#define X6B _EQ(           LAUNCH_APP1                                                      ) // SpeedRacer
#define X6C _EQ(           LAUNCH_MAIL                                                      ) // SpeedRacer
#define X6D _EQ(           LAUNCH_MEDIA_SELECT                                              ) // SpeedRacer
#define XF1 _NE(_none_,    _none_,    _none_,    HANJA,     HANJA,     HANJA,     HANJA     )
#define XF2 _NE(_none_,    _none_,    _none_,    HANGEUL,   HANGEUL,   HANGEUL,   HANGEUL   )

        /*
         * The break key is sent to us as E1,LCtrl,NumLock
         * We must convert the E1+LCtrl to BREAK, then ignore the Numlock
         */
#define Y1D _EQ(           PAUSE                                                            )

#elif (KBD_TYPE > 20) && (KBD_TYPE <= 22)
/***********************************************************************\
* T** - Values for ausVK[] (Virtual Scan Code to Virtual Key conversion)
*
* Three sets of keyboards are supported, according to KBD_TYPE:
*
* KBD_TYPE   Keyboard (examples)
* ========   =====================================
*   20       Fujitsu FMR JIS keyboard.
*   21       Fujitsu FMR OYAYUBI keyboard.
*   22 *     Fujitsu FMV OYAYUBI keyboard.
*
*     +------+ +----------+----------+----------+
*     | Scan | |  kbd     |  kbd     |  kbd     |
*     | code | | type 20  | type 21  | type 22  |
\****+-------+-+----------+----------+----------+***********************/
#define T00 _EQ(                      _none_    )
#define T01 _EQ(                      ESCAPE    )
#define T02 _EQ(                      '1'       )
#define T03 _EQ(                      '2'       )
#define T04 _EQ(                      '3'       )
#define T05 _EQ(                      '4'       )
#define T06 _EQ(                      '5'       )
#define T07 _EQ(                      '6'       )
#define T08 _EQ(                      '7'       )
#define T09 _EQ(                      '8'       )
#define T0A _EQ(                      '9'       )
#define T0B _EQ(                      '0'       )
#define T0C _EQ(                      OEM_MINUS )
#define T0D _EQ(                      OEM_7     )
#define T0E _NE(OEM_5,     OEM_5,     BACK      )
#define T0F _NE(BACK,      BACK,      TAB       )
#define T10 _NE(TAB,       TAB,       'Q'       )
#define T11 _NE('Q',       'Q',       'W'       )
#define T12 _NE('W',       'W',       'E'       )
#define T13 _NE('E',       'E',       'R'       )
#define T14 _NE('R',       'R',       'T'       )
#define T15 _NE('T',       'T',       'Y'       )
#define T16 _NE('Y',       'Y',       'U'       )
#define T17 _NE('U',       'U',       'I'       )
#define T18 _NE('I',       'I',       'O'       )
#define T19 _NE('O',       'O',       'P'       )
#define T1A _NE('P',       'P',       OEM_3     )
#define T1B _NE(OEM_3,     OEM_3,     OEM_4     )
#define T1C _NE(OEM_4,     OEM_4,     RETURN    )
#define T1D _NE(RETURN,    RETURN,    LCONTROL  )
#define T1E _EQ(                      'A'       )
#define T1F _EQ(                      'S'       )
#define T20 _EQ(                      'D'       )
#define T21 _EQ(                      'F'       )
#define T22 _EQ(                      'G'       )
#define T23 _EQ(                      'H'       )
#define T24 _EQ(                      'J'       )
#define T25 _EQ(                      'K'       )
#define T26 _EQ(                      'L'       )
#define T27 _EQ(                      OEM_PLUS  )
#define T28 _EQ(                      OEM_1     )
#define T29 _NE(OEM_6,     OEM_6,     DBE_SBCSCHAR)
#define T2A _NE('Z',       'Z',       LSHIFT    )
#define T2B _NE('X',       'X',       OEM_6     )
#define T2C _NE('C',       'C',       'Z'       )
#define T2D _NE('V',       'V',       'X'       )
#define T2E _NE('B',       'B',       'C'       )
#define T2F _NE('N',       'N',       'V'       )
#define T30 _NE('M',       'M',       'B'       )
#define T31 _NE(OEM_COMMA, OEM_COMMA, 'N'       )
#define T32 _NE(OEM_PERIOD,OEM_PERIOD,'M'       )
#define T33 _NE(OEM_2,     OEM_2,     OEM_COMMA )
#define T34 _NE(OEM_8,     OEM_8,     OEM_PERIOD)
#define T35 _NE(' ',       ' ',       OEM_2     )
#define T36 _NE(MULTIPLY,  MULTIPLY,  RSHIFT    )
#define T37 _NE(DIVIDE,    DIVIDE,    MULTIPLY  )
#define T38 _NE(ADD,       ADD,       LMENU     )
#define T39 _NE(SUBTRACT,  SUBTRACT,  ' '       )
#define T3A _NE(NUMPAD7,   NUMPAD7,   DBE_ALPHANUMERIC)
#define T3B _NE(NUMPAD8,   NUMPAD8,   F1        )
#define T3C _NE(NUMPAD9,   NUMPAD9,   F2        )
#define T3D _NE(EQUAL,     EQUAL,     F3        )
#define T3E _NE(NUMPAD4,   NUMPAD4,   F4        )
#define T3F _NE(NUMPAD5,   NUMPAD5,   F5        )
#define T40 _NE(NUMPAD6,   NUMPAD6,   F6        )
#define T41 _NE(SEPARATOR, SEPARATOR, F7        )
#define T42 _NE(NUMPAD1,   NUMPAD1,   F8        )
#define T43 _NE(NUMPAD2,   NUMPAD2,   F9        )
#define T44 _NE(NUMPAD3,   NUMPAD3,   F10       )
#define T45 _NE(RETURN,    RETURN,    NUMLOCK   )
#define T46 _NE(NUMPAD0,   NUMPAD0,   SCROLL    )
#define T47 _NE(DECIMAL,   DECIMAL,   HOME      )
#define T48 _NE(INSERT,    INSERT,    UP        )
#define T49 _NE(OEM_00,    OEM_00,    PRIOR     )
#define T4A _NE(OEM_000,   OEM_000,   SUBTRACT  )
#define T4B _NE(DELETE,    DELETE,    LEFT      )
#define T4C _NE(_none_,    _none_,    CLEAR     )
#define T4D _NE(UP,        UP,        RIGHT     )
#define T4E _NE(HOME,      HOME,      ADD       )
#define T4F _NE(LEFT,      LEFT,      END       )
#define T50 _EQ(                      DOWN      )
#define T51 _NE(RIGHT,     RIGHT,     NEXT      )
#define T52 _NE(LCONTROL,  LCONTROL,  INSERT    )
#define T53 _NE(LSHIFT,    LSHIFT,    DELETE    )
#define T54 _NE(_none_,    _none_,    SNAPSHOT  )
#define T55 _NE(CAPITAL,   _none_,    _none_    )
#define T56 _NE(DBE_HIRAGANA,_none_,  _none_    )
#define T57 _NE(NONCONVERT,NONCONVERT,F11       )
#define T58 _NE(CONVERT,   CONVERT,   F12       )
#define T59 _NE(KANJI,     KANJI,     CLEAR     )
#define T5A _NE(DBE_KATAKANA,_none_,  NONAME    )
#define T5B _NE(F12,       F12,       NONAME    )
#define T5C _NE(LMENU,     LMENU,     NONAME    )
#define T5D _NE(F1,        F1,        EREOF     )
#define T5E _NE(F2,        F2,        _none_    )
#define T5F _NE(F3,        F3,        NONAME    )
#define T60 _NE(F4,        F4,        _none_    )
#define T61 _NE(F5,        F5,        _none_    )
#define T62 _NE(F6,        F6,        _none_    )
#define T63 _NE(F7,        F7,        _none_    )
#define T64 _NE(F8,        F8,        F13       )
#define T65 _NE(F9,        F9,        F14       )
#define T66 _NE(F10,       F10,       F15       )
#define T67 _NE(_none_,    OEM_LOYA,  F16       )
#define T68 _NE(_none_,    OEM_ROYA,  F17       )
#define T69 _NE(F11,       F11,       F18       )
#define T6A _NE(_none_,    DBE_ALPHANUMERIC,F19 )
#define T6B _NE(OEM_JISHO, OEM_JISHO, F20       )
#define T6C _NE(OEM_MASSHOU,OEM_MASSHOU,F21     )
#define T6D _NE(_none_,    _none_,    F22       )
#define T6E _NE(PRIOR,     PRIOR,     F23       )
#define T6F _NE(_none_,    DBE_KATAKANA,_none_  )
#define T70 _NE(NEXT,      NEXT,      DBE_HIRAGANA)
#define T71 _EQ(                      _none_    )
#define T72 _NE(CANCEL,    CANCEL,    _none_    )
#define T73 _NE(EXECUTE,   EXECUTE,   OEM_102   )
#define T74 _NE(F13,       F13,       _none_    )
#define T75 _NE(F14,       F14,       _none_    )
#define T76 _NE(F15,       F15,       F24       )
#define T77 _NE(F16,       F16,       _none_    )
#define T78 _NE(CLEAR,     CLEAR,     _none_    )
#define T79 _NE(HELP,      HELP,      CONVERT   )
#define T7A _NE(END,       END,       _none_    )
#define T7B _NE(SCROLL,    SCROLL,    NONCONVERT)
#define T7C _NE(PAUSE,     PAUSE,     TAB       )
#define T7D _NE(SNAPSHOT,  SNAPSHOT,  OEM_5     )
#define T7E _NE(_none_,    _none_,    ABNT_C2   )
#define T7F _NE(_none_,    _none_,    OEM_PA2   )

#define X1C _NE(_none_,    _none_,    RETURN    )
#define X1D _NE(_none_,    _none_,    RCONTROL  )
#define X33 _EQ(                      _none_    )
#define X35 _NE(_none_,    _none_,    DIVIDE    )
#define X37 _NE(_none_,    _none_,    SNAPSHOT  )
#define X38 _NE(_none_,    _none_,    RMENU     )
#define X42 _EQ(                      _none_    )
#define X43 _EQ(                      _none_    )
#define X44 _EQ(                      _none_    )
#define X46 _NE(_none_,    _none_,    CANCEL    )
#define X47 _NE(_none_,    _none_,    HOME      )
#define X48 _NE(_none_,    _none_,    UP        )
#define X49 _NE(_none_,    _none_,    PRIOR     )
#define X4B _NE(_none_,    _none_,    LEFT      )
#define X4D _NE(_none_,    _none_,    RIGHT     )
#define X4F _NE(_none_,    _none_,    END       )
#define X50 _NE(_none_,    _none_,    DOWN      )
#define X51 _NE(_none_,    _none_,    NEXT      )
#define X52 _NE(_none_,    _none_,    INSERT    )
#define X53 _NE(_none_,    _none_,    DELETE    )
#define X5B _NE(_none_,    _none_,    LWIN      )
#define X5C _NE(_none_,    _none_,    RWIN      )
#define X5D _NE(_none_,    _none_,    APPS      )
#define X5E _EQ(                      POWER     )
#define X5F _EQ(                      SLEEP     )
#define X60 _NE(SCROLL,    SCROLL,    _none_    )
#define X61 _NE(HOME,      HOME,      _none_    )
#define X62 _NE(END,       END,       _none_    )
#define X63 _EQ(                      _none_    )
#define X64 _EQ(                      _none_    )
#define X65 _EQ(                      _none_    )
#define X66 _EQ(                      _none_    )
#define X6D _NE(OEM_TOUROKU,OEM_TOUROKU,_none_  )
#define X71 _NE(DBE_SBCSCHAR,DBE_SBCSCHAR,_none_)
#define X74 _EQ(                      _none_    )
#define X75 _EQ(                      _none_    )
#define X76 _EQ(                      _none_    )
#define X77 _EQ(                      _none_    )
#define X78 _EQ(                      _none_    )
#define X79 _EQ(                      _none_    )
#define X7A _EQ(                      _none_    )
#define X7B _EQ(                      _none_    )

        /*
         * The break key is sent to us as E1,LCtrl,NumLock
         * We must convert the E1+LCtrl to BREAK, then ignore the Numlock
         * which must be ignored.  Alternatively, translate Ctrl-Numlock
         * to break, but don't let the CTRL through as a WM_KEYUP/DOWN) ?
         */
#define Y1D _EQ(              PAUSE             )

#elif (KBD_TYPE >= 30) && (KBD_TYPE <= 34)
/***********************************************************************\
* T** - Values for ausVK[] (Virtual Scan Code to Virtual Key conversion)
*
* Three sets of keyboards are supported, according to KBD_TYPE:
*
* KBD_TYPE   Keyboard (examples)
* ========   =====================================
*   30 *     NEC PC-9800 Normal Keyboard.
*   31       NEC PC-9800 Document processor Keyboard.  - not supported on NT5
*   32       NEC PC-9800 106 Keyboard. - same as KBD_TYPE 8
*   33       NEC PC-9800 for Hydra: PC-9800 Keyboard on Windows NT 5.0.
*            NEC PC-98NX for Hydra: PC-9800 Keyboard on Windows 95/NT.
*   34       NEC PC-9800 for Hydra: PC-9800 Keyboard on Windows NT 3.51/4.0.
*
*     +------+ +----------+----------+----------+
*     | Scan | |   kbd    |   kbd    |   kbd    |
*     | code | |  type 30 |  type 33 |  type 34 |
\****+-------+-+----------+----------+----------+***********************/
#define T00 _EQ(_none_                          )
#define T01 _EQ(ESCAPE                          )
#define T02 _EQ('1'                             )
#define T03 _EQ('2'                             )
#define T04 _EQ('3'                             )
#define T05 _EQ('4'                             )
#define T06 _EQ('5'                             )
#define T07 _EQ('6'                             )
#define T08 _EQ('7'                             )
#define T09 _EQ('8'                             )
#define T0A _EQ('9'                             )
#define T0B _EQ('0'                             )
#define T0C _EQ(OEM_MINUS                       )
#define T0D _NE(OEM_7,     OEM_7,     OEM_PLUS  )
#define T0E _EQ(BACK                            )
#define T0F _EQ(TAB                             )
#define T10 _EQ('Q'                             )
#define T11 _EQ('W'                             )
#define T12 _EQ('E'                             )
#define T13 _EQ('R'                             )
#define T14 _EQ('T'                             )
#define T15 _EQ('Y'                             )
#define T16 _EQ('U'                             )
#define T17 _EQ('I'                             )
#define T18 _EQ('O'                             )
#define T19 _EQ('P'                             )
#define T1A _NE(OEM_3,     OEM_3,     OEM_4     )
#define T1B _NE(OEM_4,     OEM_4,     OEM_6     )
#define T1C _EQ(RETURN                          )
#define T1D _EQ(LCONTROL                        )
#define T1E _EQ('A'                             )
#define T1F _EQ('S'                             )
#define T20 _EQ('D'                             )
#define T21 _EQ('F'                             )
#define T22 _EQ('G'                             )
#define T23 _EQ('H'                             )
#define T24 _EQ('J'                             )
#define T25 _EQ('K'                             )
#define T26 _EQ('L'                             )
#define T27 _NE(OEM_PLUS,  OEM_PLUS,  OEM_1     )
#define T28 _NE(OEM_1,     OEM_1,     OEM_7     )
#define T29 _NE(DBE_SBCSCHAR, \
                           DBE_SBCSCHAR, \
                                      OEM_3     )
#define T2A _EQ(LSHIFT                          )
#define T2B _NE(OEM_6,     OEM_6,     OEM_5     )
#define T2C _EQ('Z'                             )
#define T2D _EQ('X'                             )
#define T2E _EQ('C'                             )
#define T2F _EQ('V'                             )
#define T30 _EQ('B'                             )
#define T31 _EQ('N'                             )
#define T32 _EQ('M'                             )
#define T33 _EQ(OEM_COMMA                       )
#define T34 _EQ(OEM_PERIOD                      )
#define T35 _EQ(OEM_2                           )
#define T36 _EQ(RSHIFT                          )
#define T37 _EQ(MULTIPLY                        )
#define T38 _EQ(LMENU                           )
#define T39 _EQ(' '                             )
#define T3A _EQ(CAPITAL                         )
#define T3B _EQ(F1                              )
#define T3C _EQ(F2                              )
#define T3D _EQ(F3                              )
#define T3E _EQ(F4                              )
#define T3F _EQ(F5                              )
#define T40 _EQ(F6                              )
#define T41 _EQ(F7                              )
#define T42 _EQ(F8                              )
#define T43 _EQ(F9                              )
#define T44 _EQ(F10                             )
#define T45 _EQ(NUMLOCK                         )
#define T46 _EQ(SCROLL                          )
#define T47 _EQ(HOME                            )
#define T48 _EQ(UP                              )
#define T49 _EQ(PRIOR                           )
#define T4A _EQ(SUBTRACT                        )
#define T4B _EQ(LEFT                            )
#define T4C _EQ(CLEAR                           )
#define T4D _EQ(RIGHT                           )
#define T4E _EQ(ADD                             )
#define T4F _EQ(END                             )
#define T50 _EQ(DOWN                            )
#define T51 _EQ(NEXT                            )
#define T52 _EQ(INSERT                          )
#define T53 _EQ(DELETE                          )
#define T54 _EQ(SNAPSHOT                        )
#define T55 _NE(_none_,    _none_,    OEM_8     )
#define T56 _EQ(_none_                          )
#define T57 _EQ(F11                             )
#define T58 _EQ(F12                             )
#define T59 _EQ(OEM_NEC_EQUAL                   )
#define T5A _NE(NONAME,    NONAME,    NONCONVERT)
#define T5B _NE(_none_,    _none_,    NONAME    )
#define T5C _EQ(SEPARATOR                       )
#define T5D _EQ(F13                             )
#define T5E _EQ(F14                             )
#define T5F _EQ(F15                             )
#define T60 _EQ(_none_                          )
#define T61 _EQ(_none_                          )
#define T62 _EQ(_none_                          )
#define T63 _EQ(_none_                          )
#define T64 _NE(_none_,    _none_,    F13       )
#define T65 _NE(_none_,    _none_,    F14       )
#define T66 _NE(_none_,    _none_,    F15       )
#define T67 _NE(_none_,    _none_,    F16       )
#define T68 _NE(_none_,    _none_,    F17       )
#define T69 _NE(_none_,    _none_,    F18       )
#define T6A _NE(_none_,    _none_,    F19       )
#define T6B _NE(_none_,    _none_,    F20       )
#define T6C _NE(_none_,    _none_,    F21       )
#define T6D _NE(_none_,    _none_,    F22       )
#define T6E _NE(_none_,    _none_,    F23       )
#define T6F _EQ(_none_                          )
#define T70 _NE(KANA,      KANA,      DBE_HIRAGANA)
#define T71 _EQ(_none_                          )
#define T72 _EQ(_none_                          )
#define T73 _NE(OEM_8,     OEM_8,     _none_    )
#define T74 _NE(_none_,    OEM_NEC_EQUAL, \
                                      _none_    )
#define T75 _NE(_none_,    SEPARATOR, _none_    )
#define T76 _NE(_none_,    _none_,    F24       )
#define T77 _NE(_none_,    _none_,    DBE_SBCSCHAR)
#define T78 _EQ(_none_                          )
#define T79 _EQ(CONVERT                         )
#define T7A _EQ(_none_                          )
#define T7B _EQ(NONCONVERT                      )
#define T7C _NE(TAB,       _none_,    _none_    )
#define T7D _NE(OEM_5,     OEM_5,     _none_    )
#define T7E _NE(ABNT_C2,   ABNT_C2,   _none_    )
#define T7F _NE(OEM_PA2,   OEM_PA2,   _none_    )

#define X1C _EQ(RETURN                          )
#define X1D _NE(RCONTROL,  RCONTROL,  KANA      )
#define X33 _EQ(_none_                          )
#define X35 _EQ(DIVIDE                          )
#define X37 _EQ(SNAPSHOT                        )
#define X38 _NE(_none_,    _none_,    KANJI     )
#define X42 _NE(_none_,    _none_,    RCONTROL  )
#define X43 _NE(_none_,    _none_,    RMENU     )
#define X44 _EQ(_none_                          )
#define X46 _EQ(CANCEL                          )
#define X47 _EQ(HOME                            )
#define X48 _EQ(UP                              )
#define X49 _EQ(PRIOR                           )
#define X4B _EQ(LEFT                            )
#define X4D _EQ(RIGHT                           )
#define X4F _EQ(END                             )
#define X50 _EQ(DOWN                            )
#define X51 _EQ(NEXT                            )
#define X52 _EQ(INSERT                          )
#define X53 _EQ(DELETE                          )
#define X5B _EQ(LWIN                            )
#define X5C _EQ(RWIN                            )
#define X5D _EQ(APPS                            )
#define X5E _EQ(POWER                           )
#define X5F _EQ(SLEEP                           )
#define X60 _EQ(_none_                          )
#define X61 _EQ(_none_                          )
#define X62 _EQ(_none_                          )
#define X63 _EQ(_none_                          )
#define X64 _EQ(_none_                          )
#define X65 _EQ(_none_                          )
#define X66 _EQ(_none_                          )
#define X6D _EQ(_none_                          )
#define X71 _EQ(_none_                          )
#define X74 _EQ(_none_                          )
#define X75 _EQ(_none_                          )
#define X76 _EQ(_none_                          )
#define X77 _EQ(_none_                          )
#define X78 _EQ(_none_                          )
#define X79 _EQ(_none_                          )
#define X7A _EQ(_none_                          )
#define X7B _EQ(_none_                          )
        /*
         * The break key is sent to us as E1,LCtrl,NumLock
         * We must convert the E1+LCtrl to BREAK, then ignore the Numlock
         * which must be ignored.  Alternatively, translate Ctrl-Numlock
         * to break, but don't let the CTRL through as a WM_KEYUP/DOWN) ?
         */
#define Y1D _EQ(PAUSE                          )

#elif (KBD_TYPE == 37)
/***********************************************************************\
* T** - Values for ausVK[] (Virtual Scan Code to Virtual Key conversion)
*
* Three sets of keyboards are supported, according to KBD_TYPE:
*
* KBD_TYPE   Keyboard (examples)
* ========   =====================================
*   37 *     NEC PC-9800 for Hydra: PC-9800 Keyboard on Windows 95.
*
*     +------+ +----------+
*     | Scan | |   kbd    |
*     | code | |  type 37 |
\****+-------+-+----------+*********************************************/
#define T00 _EQ(ESCAPE    )
#define T01 _EQ('1'       )
#define T02 _EQ('2'       )
#define T03 _EQ('3'       )
#define T04 _EQ('4'       )
#define T05 _EQ('5'       )
#define T06 _EQ('6'       )
#define T07 _EQ('7'       )
#define T08 _EQ('8'       )
#define T09 _EQ('9'       )
#define T0A _EQ('0'       )
#define T0B _EQ(OEM_MINUS )
#define T0C _EQ(OEM_7     )
#define T0D _EQ(OEM_5     )
#define T0E _EQ(BACK      )
#define T0F _EQ(TAB       )
#define T10 _EQ('Q'       )
#define T11 _EQ('W'       )
#define T12 _EQ('E'       )
#define T13 _EQ('R'       )
#define T14 _EQ('T'       )
#define T15 _EQ('Y'       )
#define T16 _EQ('U'       )
#define T17 _EQ('I'       )
#define T18 _EQ('O'       )
#define T19 _EQ('P'       )
#define T1A _EQ(OEM_3     )
#define T1B _EQ(OEM_4     )
#define T1C _EQ(RETURN    )
#define T1D _EQ('A'       )
#define T1E _EQ('S'       )
#define T1F _EQ('D'       )
#define T20 _EQ('F'       )
#define T21 _EQ('G'       )
#define T22 _EQ('H'       )
#define T23 _EQ('J'       )
#define T24 _EQ('K'       )
#define T25 _EQ('L'       )
#define T26 _EQ(OEM_PLUS  )
#define T27 _EQ(OEM_1     )
#define T28 _EQ(OEM_6     )
#define T29 _EQ('Z'       )
#define T2A _EQ('X'       )
#define T2B _EQ('C'       )
#define T2C _EQ('V'       )
#define T2D _EQ('B'       )
#define T2E _EQ('N'       )
#define T2F _EQ('M'       )
#define T30 _EQ(OEM_COMMA )
#define T31 _EQ(OEM_PERIOD)
#define T32 _EQ(OEM_2     )
#define T33 _EQ(OEM_8     )
#define T34 _EQ(' '       )
#define T35 _EQ(CONVERT   )
#define T36 _EQ(NEXT      )
#define T37 _EQ(PRIOR     )
#define T38 _EQ(INSERT    )
#define T39 _EQ(DELETE    )
#define T3A _EQ(UP        )
#define T3B _EQ(LEFT      )
#define T3C _EQ(RIGHT     )
#define T3D _EQ(DOWN      )
#define T3E _EQ(HOME      )
#define T3F _EQ(END       )
#define T40 _EQ(SUBTRACT  )
#define T41 _EQ(DIVIDE    )
#define T42 _EQ(NUMPAD7   )
#define T43 _EQ(NUMPAD8   )
#define T44 _EQ(NUMPAD9   )
#define T45 _EQ(MULTIPLY  )
#define T46 _EQ(NUMPAD4   )
#define T47 _EQ(NUMPAD5   )
#define T48 _EQ(NUMPAD6   )
#define T49 _EQ(ADD       )
#define T4A _EQ(NUMPAD1   )
#define T4B _EQ(NUMPAD2   )
#define T4C _EQ(NUMPAD3   )
#define T4D _EQ(OEM_NEC_EQUAL)
#define T4E _EQ(NUMPAD0   )
#define T4F _EQ(SEPARATOR )
#define T50 _EQ(DECIMAL   )
#define T51 _EQ(NONCONVERT)
#define T52 _EQ(F11       )
#define T53 _EQ(F12       )
#define T54 _EQ(F13       )
#define T55 _EQ(F14       )
#define T56 _EQ(F15       )
#define T57 _EQ(_none_    )
#define T58 _EQ(_none_    )
#define T59 _EQ(_none_    )
#define T5A _EQ(_none_    )
#define T5B _EQ(_none_    )
#define T5C _EQ(RETURN    )
#define T5D _EQ(_none_    )
#define T5E _EQ(_none_    )
#define T5F _EQ(_none_    )
#define T60 _EQ(CANCEL    )
#define T61 _EQ(SNAPSHOT  )
#define T62 _EQ(F1        )
#define T63 _EQ(F2        )
#define T64 _EQ(F3        )
#define T65 _EQ(F4        )
#define T66 _EQ(F5        )
#define T67 _EQ(F6        )
#define T68 _EQ(F7        )
#define T69 _EQ(F8        )
#define T6A _EQ(F9        )
#define T6B _EQ(F10       )
#define T6C _EQ(_none_    )
#define T6D _EQ(_none_    )
#define T6E _EQ(_none_    )
#define T6F _EQ(_none_    )
#define T70 _EQ(LSHIFT    )
#define T71 _EQ(CAPITAL   )
#define T72 _EQ(KANA      )
#define T73 _EQ(LMENU     )
#define T74 _EQ(LCONTROL  )
#define T75 _EQ(_none_    )
#define T76 _EQ(_none_    )
#define T77 _EQ(LWIN      )
#define T78 _EQ(RWIN      )
#define T79 _EQ(APPS      )
#define T7A _EQ(_none_    )
#define T7B _EQ(_none_    )
#define T7C _EQ(_none_    )
#define T7D _EQ(RSHIFT    )
#define T7E _EQ(ABNT_C2   )
#define T7F _EQ(OEM_PA2   )

        /*
         * The break key is sent to us as E1,LCtrl,NumLock
         * We must conevrt the E1+LCtrl to BREAK, then ignore the Numlock
         * which must be ignored.  Alternatively, translate Ctrl-Numlock
         * to break, but don't let the CTRL through as a WM_KEYUP/DOWN) ?
         */
#define Y1D _EQ(PAUSE    )

#elif (KBD_TYPE >= 40) && (KBD_TYPE <= 41)
/***********************************************************************\
* T** - Values for ausVK[] (Virtual Scan Code to Virtual Key conversion)
*
* Two sets of keyboards are supported, according to KBD_TYPE:
*
* KBD_TYPE   Keyboard (examples)
* ========   =====================================
*   40 *     DEC LK411-JJ (JIS  layout) keyboard
*   41       DEC LK411-AJ (ANSI layout) keyboard
*
*     +------+ +-----------+-----------+
*     | Scan | |    kbd    |    kbd    |
*     | code | |  LK411-JJ |  LK411-AJ |
\*****+------+-+-----------+-----------+********************************/
#define T00 _EQ(        _none_            )
#define T01 _EQ(        ESCAPE            )
#define T02 _EQ(        '1'               )
#define T03 _EQ(        '2'               )
#define T04 _EQ(        '3'               )
#define T05 _EQ(        '4'               )
#define T06 _EQ(        '5'               )
#define T07 _EQ(        '6'               )
#define T08 _EQ(        '7'               )
#define T09 _EQ(        '8'               )
#define T0A _EQ(        '9'               )
#define T0B _EQ(        '0'               )
#define T0C _EQ(        OEM_MINUS         )
#define T0D _NE( OEM_7,      OEM_PLUS     ) // "^"/"="
#define T0E _EQ(        BACK              )
#define T0F _EQ(        TAB               )
#define T10 _EQ(        'Q'               )
#define T11 _EQ(        'W'               )
#define T12 _EQ(        'E'               )
#define T13 _EQ(        'R'               )
#define T14 _EQ(        'T'               )
#define T15 _EQ(        'Y'               )
#define T16 _EQ(        'U'               )
#define T17 _EQ(        'I'               )
#define T18 _EQ(        'O'               )
#define T19 _EQ(        'P'               )
#define T1A _NE( OEM_3,      OEM_4        ) // "@"/"["
#define T1B _NE( OEM_4,      OEM_6        ) // "["/"]"
#define T1C _EQ(        RETURN            )
#define T1D _EQ(        LCONTROL          )
#define T1E _EQ(        'A'               )
#define T1F _EQ(        'S'               )
#define T20 _EQ(        'D'               )
#define T21 _EQ(        'F'               )
#define T22 _EQ(        'G'               )
#define T23 _EQ(        'H'               )
#define T24 _EQ(        'J'               )
#define T25 _EQ(        'K'               )
#define T26 _EQ(        'L'               )
#define T27 _NE( OEM_PLUS,   OEM_1        ) // ";"
#define T28 _NE( OEM_1,      OEM_7        ) // ":"/"'"
#define T29 _NE( _none_,     DBE_SBCSCHAR ) // LK411AJ uses "<>" as SBCS/DBCS key
#define T2A _EQ(        LSHIFT            )
#define T2B _NE( OEM_6,      OEM_5        ) // "]"/"\"
#define T2C _EQ(        'Z'               )
#define T2D _EQ(        'X'               )
#define T2E _EQ(        'C'               )
#define T2F _EQ(        'V'               )
#define T30 _EQ(        'B'               )
#define T31 _EQ(        'N'               )
#define T32 _EQ(        'M'               )
#define T33 _EQ(        OEM_COMMA         )
#define T34 _EQ(        OEM_PERIOD        )
#define T35 _EQ(        OEM_2             ) // "/"
#define T36 _EQ(        RSHIFT            )
#define T37 _EQ(        MULTIPLY          ) // PF3 : "*"
#define T38 _EQ(        LMENU             ) // Alt(Left)
#define T39 _EQ(        ' '               ) // Space
#define T3A _EQ(        CAPITAL           ) // LOCK : Caps Lock
#define T3B _EQ(        F1                )
#define T3C _EQ(        F2                )
#define T3D _EQ(        F3                )
#define T3E _EQ(        F4                )
#define T3F _EQ(        F5                )
#define T40 _EQ(        F6                )
#define T41 _EQ(        F7                )
#define T42 _EQ(        F8                )
#define T43 _EQ(        F9                )
#define T44 _EQ(        F10               )
#define T45 _EQ(        NUMLOCK           ) // PF1 : Num Lock
#define T46 _EQ(        SCROLL            ) // F19 : Scroll Lock
#define T47 _EQ(        HOME              ) // KP7 : Home
#define T48 _EQ(        UP                ) // KP8 : Up
#define T49 _EQ(        PRIOR             ) // KP9 : Page Up
#define T4A _EQ(        SUBTRACT          ) // PF4 : "-"
#define T4B _EQ(        LEFT              ) // KP4 : Left
#define T4C _EQ(        CLEAR             ) // KP5 : Clear
#define T4D _EQ(        RIGHT             ) // KP6 : Right
#define T4E _EQ(        ADD               ) // KP, : Add
#define T4F _EQ(        END               ) // KP1 : End
#define T50 _EQ(        DOWN              ) // KP2 : Down
#define T51 _EQ(        NEXT              ) // KP3 : Next
#define T52 _EQ(        INSERT            ) // KP0 : Ins
#define T53 _EQ(        DELETE            ) // KP. : Del
#define T54 _EQ(        SNAPSHOT          )
#define T55 _EQ(        _none_            )
#define T56 _EQ(        _none_            )
#define T57 _EQ(        F11               )
#define T58 _EQ(        F12               )
#define T59 _EQ(        _none_            )
#define T5A _EQ(        _none_            )
#define T5B _EQ(        _none_            )
#define T5C _EQ(        _none_            )
#define T5D _EQ(        _none_            )
#define T5E _EQ(        _none_            )
#define T5F _EQ(        _none_            )
#define T60 _EQ(        _none_            )
#define T61 _EQ(        _none_            )
#define T62 _EQ(        _none_            )
#define T63 _EQ(        _none_            )
#define T64 _EQ(        _none_            )
#define T65 _EQ(        _none_            )
#define T66 _EQ(        _none_            )
#define T67 _EQ(        _none_            )
#define T68 _EQ(        _none_            )
#define T69 _EQ(        _none_            )
#define T6A _EQ(        _none_            )
#define T6B _EQ(        _none_            )
#define T6C _EQ(        _none_            )
#define T6D _EQ(        _none_            )
#define T6E _EQ(        _none_            )
#define T6F _EQ(        _none_            )
#define T70 _EQ(       DBE_HIRAGANA       ) // Hiragana/Katakana
#define T71 _EQ(        _none_            )
#define T72 _EQ(        _none_            )
#define T73 _NE( OEM_102,     _none_      ) // LK411JJ, Katakana "Ro"
#define T74 _EQ(        _none_            )
#define T75 _EQ(        _none_            )
#define T76 _EQ(        _none_            )
#define T77 _EQ(        _none_            )
#define T78 _EQ(        _none_            )
#define T79 _EQ(        CONVERT           ) // Henkan
#define T7A _EQ(        _none_            )
#define T7B _EQ(        NONCONVERT        ) // Mu-Henkan
#define T7C _EQ(        _none_            )
#define T7D _NE( OEM_5,       _none_      ) // LK411JJ, Yen(Back-slash)
#define T7E _EQ(        _none_            )
#define T7F _EQ(        _none_            )

#define X0F _EQ(        KANA              ) // Kana
#define X1C _EQ(        RETURN            ) // Enter
#define X1D _EQ(        RCONTROL          ) // Comp : Right Control
#define X33 _EQ(        _none_            )
#define X35 _EQ(        DIVIDE            ) // PF2: "/"
#define X37 _EQ(        SNAPSHOT          ) // F18: PrintScreen
#define X38 _EQ(        RMENU             ) // Alt(Right)
#define X3D _EQ(        F13               )
#define X3E _EQ(        F14               )
#define X3F _EQ(        F15               ) // Help : F15
#define X40 _EQ(        F16               ) // Do :   F16
#define X41 _EQ(        F17               )
#define X42 _EQ(        _none_            )
#define X43 _EQ(        _none_            )
#define X44 _EQ(        _none_            )
#define X46 _EQ(        CANCEL            )
#define X47 _EQ(        HOME              ) // Find : HOME
#define X48 _EQ(        UP                )
#define X49 _EQ(        PRIOR             ) // Prev : PageUp
#define X4B _EQ(        LEFT              )
#define X4D _EQ(        RIGHT             )
#define X4E _EQ(        ADD               ) // KP- (Minus but "Add")
#define X4F _EQ(        END               ) // Select : END
#define X50 _EQ(        DOWN              )
#define X51 _EQ(        NEXT              ) // Next : PageDown
#define X52 _EQ(        INSERT            )
#define X53 _EQ(        DELETE            ) // Remove
#define X5B _EQ(        _none_            )
#define X5C _EQ(        _none_            )
#define X5D _EQ(        _none_            )
#define X5E _EQ(        POWER             )
#define X5F _EQ(        SLEEP             )
        /*
         * The break key is sent to us as E1,LCtrl,NumLock
         * We must convert the E1+LCtrl to BREAK, then ignore the Numlock
         */
#define Y1D _EQ(        PAUSE             )

#endif // KBD_TYPE

#define SCANCODE_LSHIFT      0x2A
#define SCANCODE_RSHIFT      0x36
#define SCANCODE_CTRL        0x1D
#define SCANCODE_ALT         0x38
#define SCANCODE_SIMULATED   (FAKE_KEYSTROKE >> 16)

#define SCANCODE_NUMPAD_FIRST 0x47
#define SCANCODE_NUMPAD_LAST  0x52

#define SCANCODE_LWIN         0x5B
#define SCANCODE_RWIN         0x5C

#define SCANCODE_THAI_LAYOUT_TOGGLE 0x29

/*
 * Hydra FarEast
 */

/*
 * Structure for client keyboard information
 */
typedef struct _CLIENTKEYBOARDTYPE {
    ULONG Type;
    ULONG SubType;
    ULONG FunctionKey;
} CLIENTKEYBOARDTYPE, *PCLIENTKEYBOARDTYPE;


#endif // _KBD_
