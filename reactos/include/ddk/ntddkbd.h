#ifndef _NTDDKBD_
#define _NTDDKBD_

typedef struct _KEYBOARD_INPUT_DATA {

    //
    // Unit number.  E.g., for \Device\KeyboardPort0 the unit is '0',
    // for \Device\KeyboardPort1 the unit is '1', and so on.
    //

    USHORT UnitId;
    USHORT MakeCode;
    USHORT Flags;
    USHORT Reserved;
    ULONG ExtraInformation;

} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

#endif 						// _NTDDKBD_
