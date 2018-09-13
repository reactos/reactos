/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    blddcb.c

Abstract:

    This module implements Win32 comm api buildcommdcb

Author:

    Anthony V. Ercolano (tonye) 10-March-1992

    Actually this code was generously donated by
    ramonsa.  It is basically the code used for
    the mode command.

Revision History:

--*/

#include <basedll.h>

typedef struct _PARSE_CONTEXT {
    PSTR CharIndex;
    PSTR AdvanceIndex;
    PSTR MatchBegin;
    PSTR MatchEnd;
    } PARSE_CONTEXT,*PPARSE_CONTEXT;

static
BOOL
BuildDcb (
    LPCSTR L,
    LPDCB Dcb,
    LPCOMMTIMEOUTS To
    );

static
BOOL
Match(
    PPARSE_CONTEXT C,
    PSTR Pattern
    );

static
VOID
Advance(
    PPARSE_CONTEXT C
    );

static
DWORD
GetNumber(
    PPARSE_CONTEXT C
    );

static
BOOL
ConvertBaudRate (
    DWORD BaudIn,
    PDWORD BaudRate
    );

static
BOOL
ConvertDataBits (
    DWORD DataBitsIn,
    PBYTE DataBitsOut
    );

static
BOOL
ConvertStopBits (
    DWORD StopBitsIn,
    PBYTE StopBits
    );

static
BOOL
ConvertParity (
    CHAR ParityIn,
    PBYTE Parity
    );

static
BOOL
ConvertDtrControl (
    PSTR IdxBegin,
    PSTR IdxEnd,
    PBYTE DtrControl
    );

static
BOOL
ConvertRtsControl (
    PSTR IdxBegin,
    PSTR IdxEnd,
    PBYTE RtsControl
    );

static
VOID
IgnoreDeviceName(
    IN PPARSE_CONTEXT C
    );

static
NTSTATUS
DeviceNameCompare(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );


BOOL
BuildCommDCBAndTimeoutsW(
    LPCWSTR lpDef,
    LPDCB lpDCB,
    LPCOMMTIMEOUTS lpCommTimeouts
    )

/*++

Routine Description:

    This function translates the definition string specified by the
    lpDef parameter into appropriate device-control block codes and
    places these codes into the block pointed to by the lpDCB parameter.
    It also sets the timeouts if specified.

Arguments:

    lpDef - Points to a null terminated character string that specifies
            the device control information for the device.

    lpDCB -  Points to the DCB data structure that is to receive the
             translated string..  The structure defines the control
             settings for the serial communications device.

    lpCommTimeouts - It "TO" included, it will set the timeouts.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    UNICODE_STRING Unicode;
    ANSI_STRING Ansi;
    NTSTATUS Status;
    BOOL AnsiBool;

    RtlInitUnicodeString(
        &Unicode,
        lpDef
        );

    Status = RtlUnicodeStringToAnsiString(
                 &Ansi,
                 &Unicode,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {

        BaseSetLastNTError(Status);
        return FALSE;

    }

    AnsiBool = BuildCommDCBAndTimeoutsA(
                   (LPCSTR)Ansi.Buffer,
                   lpDCB,
                   lpCommTimeouts
                   );

    RtlFreeAnsiString(&Ansi);
    return AnsiBool;

}

BOOL
BuildCommDCBAndTimeoutsA(
    LPCSTR lpDef,
    LPDCB lpDCB,
    LPCOMMTIMEOUTS lpCommTimeouts
    )

/*++

Routine Description:

    This function translates the definition string specified by the
    lpDef parameter into appropriate device-control block codes and
    places these codes into the block pointed to by the lpDCB parameter.
    It can also set the timeout value.

Arguments:

    lpDef - Points to a null terminated character string that specifies
            the device control information for the device.

    lpDCB -  Points to the DCB data structure that is to receive the
             translated string..  The structure defines the control
             settings for the serial communications device.

    lpCommTimeouts - If TO included in string then timeouts are also set.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    if (!BuildDcb(
             lpDef,
             lpDCB,
             lpCommTimeouts
             )) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    } else {

        return TRUE;

    }

}

BOOL
BuildCommDCBW(
    LPCWSTR lpDef,
    LPDCB lpDCB
    )

/*++

Routine Description:

    This function translates the definition string specified by the
    lpDef parameter into appropriate device-control block codes and
    places these codes into the block pointed to by the lpDCB parameter.

Arguments:

    lpDef - Points to a null terminated character string that specifies
            the device control information for the device.

    lpDCB -  Points to the DCB data structure that is to receive the
             translated string..  The structure defines the control
             settings for the serial communications device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    UNICODE_STRING Unicode;
    ANSI_STRING Ansi;
    NTSTATUS Status;
    BOOL AnsiBool;

    RtlInitUnicodeString(
        &Unicode,
        lpDef
        );

    Status = RtlUnicodeStringToAnsiString(
                 &Ansi,
                 &Unicode,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {

        BaseSetLastNTError(Status);
        return FALSE;

    }

    AnsiBool = BuildCommDCBA(
                   (LPCSTR)Ansi.Buffer,
                   lpDCB
                   );

    RtlFreeAnsiString(&Ansi);
    return AnsiBool;

}

BOOL
BuildCommDCBA(
    LPCSTR lpDef,
    LPDCB lpDCB
    )

/*++

Routine Description:

    This function translates the definition string specified by the
    lpDef parameter into appropriate device-control block codes and
    places these codes into the block pointed to by the lpDCB parameter.

Arguments:

    lpDef - Points to a null terminated character string that specifies
            the device control information for the device.

    lpDCB -  Points to the DCB data structure that is to receive the
             translated string..  The structure defines the control
             settings for the serial communications device.

Return Value:

    The return value is TRUE if the function is successful or FALSE
    if an error occurs.

--*/

{

    COMMTIMEOUTS JunkTimeouts;

    if (!BuildDcb(
             lpDef,
             lpDCB,
             &JunkTimeouts
             )) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    } else {

        return TRUE;

    }

}

static
BOOL
BuildDcb (
    LPCSTR L,
    LPDCB Dcb,
    LPCOMMTIMEOUTS To
    )

/*++

Routine Description:


Arguments:

    L - A pointer to the string to convert to a DCB.
    Dcb - The dcb to fill in.

Return Value:

    FALSE if the string has some error, TRUE otherwise.

--*/

{

    BOOL        SetBaud         =   FALSE;
    BOOL        SetDataBits     =   FALSE;
    BOOL        SetStopBits     =   FALSE;
    BOOL        SetParity       =   FALSE;
    BOOL        SetRetry        =   FALSE;
    BOOL        SetTimeOut      =   FALSE;
    BOOL        SetXon          =   FALSE;
    BOOL        SetOdsr         =   FALSE;
    BOOL        SetIdsr         =   FALSE;
    BOOL        SetOcts         =   FALSE;
    BOOL        SetDtrControl   =   FALSE;
    BOOL        SetRtsControl   =   FALSE;

    DWORD       Baud;
    BYTE        DataBits;
    BYTE        StopBits;
    BYTE        Parity;
    BOOL        TimeOut;
    BOOL        Xon;
    BOOL        Odsr;
    BOOL        Idsr;
    BOOL        Octs;
    BYTE        DtrControl;
    BYTE        RtsControl;
    PARSE_CONTEXT C = {0};

    C.CharIndex = C.AdvanceIndex = (PSTR)L;

    //
    // This following call will query all of the *current* serial
    // provider names.  If it finds that the argurment string
    // contains the name (with an optional :) it will simply
    // advance past it.
    //

    IgnoreDeviceName(&C);

    if ( Match(&C, "#" ) ) {

        //
        //   Old syntax, where parameter are positional and comma-delimited.
        //
        //   We will use the following automata for parsing the input
        //   (eoi = end of input):
        //
        //           eoi
        //    [Baud]------------->[End]
        //      |            ^
        //      |,           |eoi
        //      v            |
        //     [a]-----------+
        //      |            ^
        //      | @          |eoi
        //      +-->[Parity]-+
        //      |     |      ^
        //      |     |,     |
        //      |<----+      |
        //      |            |
        //      |,           |eoi
        //      |            |
        //      v            |
        //     [b]-----------+
        //      |            ^
        //      | #          |eoi
        //      +-->[Data]---+
        //      |     |      ^
        //      |     |,     |
        //      |<----+      |
        //      |            |
        //      |,           |eoi
        //      v            |
        //     [c]-----------+
        //      |            ^
        //      | #          |eoi
        //      +-->[Stop]---+
        //

        //
        // Assume xon=off
        //

        SetXon      = TRUE;
        SetOdsr     = TRUE;
        SetOcts     = TRUE;
        SetDtrControl = TRUE;
        SetRtsControl = TRUE;
        Xon         = FALSE;
        Odsr        = FALSE;
        Octs        = FALSE;
        DtrControl = DTR_CONTROL_ENABLE;
        RtsControl = RTS_CONTROL_ENABLE;

        if (!ConvertBaudRate( GetNumber(&C), &Baud )) {
            return FALSE;
        }
        SetBaud = TRUE;
        Advance(&C);

        //
        //    A:
        //
        if ( !Match(&C, "," ) ) {
            goto Eoi;
        }
        Advance(&C);

        if ( !Match(&C, "," ) && Match(&C, "@" ) ) {

            //
            //    Parity
            //
            if (!ConvertParity( *C.MatchBegin,&Parity )) {
                return FALSE;
            }
            SetParity = TRUE;
            Advance(&C);
        }

        //
        //    B:
        //
        if ( !Match(&C, "," )) {
            goto Eoi;
        }
        Advance(&C);

        if ( Match(&C, "#" )) {

            //
            //    Data bits
            //
            if (!ConvertDataBits( GetNumber(&C),&DataBits )) {
                return FALSE;
            }
            SetDataBits = TRUE;
            Advance(&C);
        }

        //
        //    C:
        //
        if ( !Match(&C, "," )) {
            goto Eoi;
        }
        Advance(&C);

        if ( Match(&C, "1.5" ) ) {
            StopBits = ONE5STOPBITS;
            SetStopBits = TRUE;
            Advance(&C);
        } else if ( Match(&C, "#" ) ) {
            if (!ConvertStopBits( GetNumber(&C),&StopBits)) {
                return FALSE;
            }
            SetStopBits = TRUE;
            Advance(&C);
        }

        if ( !Match(&C, "," )) {
            goto Eoi;
        }

        Advance(&C);

        if ( Match(&C, "x" ) ) {

            //
            //  XON=ON
            //
            SetXon      = TRUE;
            SetOdsr     = TRUE;
            SetOcts     = TRUE;
            SetDtrControl = TRUE;
            SetRtsControl = TRUE;
            Xon         = TRUE;
            Odsr        = FALSE;
            Octs        = FALSE;
            DtrControl = DTR_CONTROL_ENABLE;
            RtsControl = RTS_CONTROL_ENABLE;
            Advance(&C);

        } else if ( Match(&C, "p" ) ) {

            //
            //  Permanent retry - Hardware handshaking
            //

            SetXon      = TRUE;
            SetOdsr     = TRUE;
            SetOcts     = TRUE;
            SetDtrControl = TRUE;
            SetRtsControl = TRUE;
            Xon         = FALSE;
            Odsr        = TRUE;
            Octs        = TRUE;
            DtrControl = DTR_CONTROL_HANDSHAKE;
            RtsControl = RTS_CONTROL_HANDSHAKE;
            Advance(&C);

        } else {

            //
            //  XON=OFF
            //
            SetXon      = TRUE;
            SetOdsr     = TRUE;
            SetOcts     = TRUE;
            SetDtrControl = TRUE;
            SetRtsControl = TRUE;
            Xon         = FALSE;
            Odsr        = FALSE;
            Octs        = FALSE;
            DtrControl = DTR_CONTROL_ENABLE;
            RtsControl = RTS_CONTROL_ENABLE;
        }

Eoi:
        if ( *C.CharIndex != '\0' ) {

            //
            //    Error
            //
            return FALSE;

        }

    } else {

        //
        // New Form
        //

        while ( *C.CharIndex != '\0' ) {

            if ( Match(&C, "BAUD=#" ) ) {
                //
                //  BAUD=
                //
                if ( !ConvertBaudRate(GetNumber(&C), &Baud ) ) {
                    return FALSE;
                }
                SetBaud     = TRUE;
                Advance(&C);

            } else if ( Match(&C, "PARITY=@"   ) ) {
                //
                //  PARITY=
                //
                if ( !ConvertParity( *C.MatchBegin, &Parity ) ) {
                    return FALSE;
                }
                SetParity   = TRUE;
                Advance(&C);

            } else if ( Match(&C, "DATA=#" ) ) {
                //
                //  DATA=
                //
                if ( !ConvertDataBits(GetNumber(&C), &DataBits ) ) {
                    return FALSE;
                }
                SetDataBits = TRUE;
                Advance(&C);

            } else if ( Match(&C, "STOP=1.5" ) ) {
                //
                //  STOP=1.5
                //
                StopBits    =  ONE5STOPBITS;
                SetStopBits = TRUE;
                Advance(&C);

            } else if ( Match(&C, "STOP=#" ) ) {
                //
                //  STOP=
                //
                if ( !ConvertStopBits(GetNumber(&C), &StopBits ) ) {
                    return FALSE;
                }
                SetStopBits = TRUE;
                Advance(&C);

            } else if ( Match(&C, "TO=ON" ) ) {
                //
                //  TO=ON
                //
                SetTimeOut  =   TRUE;
                TimeOut     =   TRUE;
                Advance(&C);

            } else if ( Match(&C, "TO=OFF" ) ) {
                //
                //  TO=ON
                //
                SetTimeOut  =   TRUE;
                TimeOut     =   FALSE;
                Advance(&C);

            } else if ( Match(&C, "XON=ON" ) ) {
                //
                //  XON=ON
                //
                SetXon      = TRUE;
                Xon         = TRUE;
                Advance(&C);

            } else if ( Match(&C, "XON=OFF" ) ) {
                //
                //  XON=OFF
                //
                SetXon      = TRUE;
                Xon         = FALSE;
                Advance(&C);

            } else if ( Match(&C, "ODSR=ON" ) ) {
                //
                //  ODSR=ON
                //
                SetOdsr     = TRUE;
                Odsr        = TRUE;
                Advance(&C);

            } else if ( Match(&C, "ODSR=OFF" ) ) {
                //
                //  ODSR=OFF
                //
                SetOdsr     = TRUE;
                Odsr        = FALSE;
                Advance(&C);

            } else if ( Match(&C, "IDSR=ON" ) ) {
                //
                //  IDSR=ON
                //
                SetIdsr = TRUE;
                Idsr    = TRUE;
                Advance(&C);

            } else if ( Match(&C, "IDSR=OFF" ) ) {
                //
                //  IDSR=OFF
                //
                SetIdsr = TRUE;
                Idsr    = FALSE;
                Advance(&C);

            } else if ( Match(&C, "OCTS=ON" ) ) {
                //
                //  OCS=ON
                //
                SetOcts     = TRUE;
                Octs        = TRUE;
                Advance(&C);

            } else if ( Match(&C, "OCTS=OFF" ) ) {
                //
                //  OCS=OFF
                //
                SetOcts     = TRUE;
                Octs        = FALSE;
                Advance(&C);

            } else if ( Match(&C, "DTR=*"   ) ) {
                //
                //  DTR=
                //
                if ( !ConvertDtrControl(C.MatchBegin, C.MatchEnd, &DtrControl ) ) {
                    return FALSE;
                }
                SetDtrControl   = TRUE;
                Advance(&C);

            } else if ( Match(&C, "RTS=*"   ) ) {
                //
                //  RTS=
                //
                if ( !ConvertRtsControl(C.MatchBegin, C.MatchEnd, &RtsControl ) ) {
                    return FALSE;
                }
                SetRtsControl   = TRUE;
                Advance(&C);

            } else {

                return FALSE;
            }
        }

    }

    if ( SetBaud ) {
        Dcb->BaudRate = Baud;
    }

    if ( SetDataBits ) {
        Dcb->ByteSize = DataBits;
    }

    if ( SetStopBits ) {
        Dcb->StopBits = StopBits;
    } else if ( SetBaud && (Baud == 110) ) {
        Dcb->StopBits = TWOSTOPBITS;
    } else {
        Dcb->StopBits = ONESTOPBIT;
    }

    if ( SetParity ) {
        Dcb->Parity = Parity;
    }

    if ( SetXon ) {
        if ( Xon ) {
            Dcb->fInX   = TRUE;
            Dcb->fOutX  = TRUE;
        } else {
            Dcb->fInX   = FALSE;
            Dcb->fOutX  = FALSE;
        }
    }

    if ( SetOcts ) {

        if ( Octs ) {
            Dcb->fOutxCtsFlow = TRUE;
        } else {
            Dcb->fOutxCtsFlow = FALSE;
        }
    }


    if ( SetOdsr ) {
        if ( Odsr ) {
            Dcb->fOutxDsrFlow = TRUE;
        } else {
            Dcb->fOutxDsrFlow = FALSE;
        }
    }

    if ( SetIdsr ) {
        if ( Idsr ) {
            Dcb->fDsrSensitivity = TRUE;
        } else {
            Dcb->fDsrSensitivity = FALSE;
        }
    }

    if ( SetDtrControl ) {
        Dcb->fDtrControl = DtrControl;
    }

    if ( SetRtsControl ) {
        Dcb->fRtsControl = RtsControl;
    }

    if ( SetTimeOut ) {
        if (TimeOut) {
            To->ReadIntervalTimeout = 0;
            To->ReadTotalTimeoutMultiplier = 0;
            To->ReadTotalTimeoutConstant = 0;
            To->WriteTotalTimeoutMultiplier = 0;
            To->WriteTotalTimeoutConstant = 60000;
        } else {
            To->ReadIntervalTimeout = 0;
            To->ReadTotalTimeoutMultiplier = 0;
            To->ReadTotalTimeoutConstant = 0;
            To->WriteTotalTimeoutMultiplier = 0;
            To->WriteTotalTimeoutConstant = 0;
        }
    }



    return TRUE;
}

static
BOOL
Match(
    PPARSE_CONTEXT C,
    PSTR Pattern
    )

/*++

Routine Description:

    This function matches a pattern against whatever
    is in the command line at the current position.

    Note that this does not advance our current position
    within the command line.

    If the pattern has a magic character, then the
    variables C->MatchBegin and C->MatchEnd delimit the
    substring of the command line that matched that
    magic character.

Arguments:

    C - The parse context.
    Pattern - Supplies pointer to the pattern to match

Return Value:

    BOOLEAN - TRUE if the pattern matched, FALSE otherwise

Notes:

--*/

{

    PSTR    CmdIndex;       //  Index within command line
    PSTR    PatternIndex;   //  Index within pattern
    CHAR    PatternChar;    //  Character in pattern
    CHAR    CmdChar;        //  Character in command line;

    CmdIndex        = C->CharIndex;
    PatternIndex    = Pattern;

    while ( (PatternChar = *PatternIndex) != '\0' ) {

        switch ( PatternChar ) {

        case '#':

            //
            //    Match a number
            //
            C->MatchBegin = CmdIndex;
            C->MatchEnd   = C->MatchBegin;

            //
            //    Get all consecutive digits
            //
            while ( ((CmdChar = *C->MatchEnd) != '\0') &&
                    isdigit( (char)CmdChar ) ) {
                C->MatchEnd++;
            }
            C->MatchEnd--;

            if ( C->MatchBegin > C->MatchEnd ) {
                //
                //    No number
                //
                return FALSE;
            }

            CmdIndex = C->MatchEnd + 1;
            PatternIndex++;

            break;


        case '@':

            //
            //    Match one character
            //
            if ( *CmdIndex == '\0' ) {
                return FALSE;
            }

            C->MatchBegin = C->MatchEnd = CmdIndex;
            CmdIndex++;
            PatternIndex++;

            break;


        case '*':

            //
            //    Match everything up to next blank (or end of input)
            //
            C->MatchBegin    = CmdIndex;
            C->MatchEnd    = C->MatchBegin;

            while ( ( (CmdChar = *C->MatchEnd ) != '\0' )  &&
                    ( CmdChar !=  ' ' ) ) {

                C->MatchEnd++;
            }
            C->MatchEnd--;

            CmdIndex = C->MatchEnd+1;
            PatternIndex++;

            break;

        case '[':

            //
            //    Optional sequence
            //
            PatternIndex++;

            PatternChar = *PatternIndex;
            CmdChar     = *CmdIndex;

            //
            //    If the first charcter in the input does not match the
            //    first character in the optional sequence, we just
            //    skip the optional sequence.
            //
            if ( ( CmdChar == '\0' ) ||
                 ( CmdChar == ' ')             ||
                 ( toupper(CmdChar) != toupper(PatternChar) ) ) {

                while ( PatternChar != ']' ) {
                    PatternIndex++;
                    PatternChar = *PatternIndex;
                }
                PatternIndex++;

            } else {

                //
                //    Since the first character in the sequence matched, now
                //    everything must match.
                //
                while ( PatternChar != ']' ) {

                    if ( toupper(PatternChar) != toupper(CmdChar) ) {
                        return FALSE;
                    }
                    CmdIndex++;
                    PatternIndex++;
                    CmdChar = *CmdIndex;
                    PatternChar = *PatternIndex;
                }

                PatternIndex++;
            }

            break;

        default:

            //
            //    Both characters must match
            //
            CmdChar = *CmdIndex;

            if ( ( CmdChar == '\0' ) ||
                 ( toupper(CmdChar) != toupper(PatternChar) ) ) {

                return FALSE;

            }

            CmdIndex++;
            PatternIndex++;

            break;

        }
    }

    C->AdvanceIndex = CmdIndex;

    return TRUE;

}

static
VOID
Advance(
    PPARSE_CONTEXT C
    )

/*++

Routine Description:

    Advances our pointers to the beginning of the next lexeme

Arguments:

    C - The parse context.

Return Value:

    None


--*/

{

    C->CharIndex = C->AdvanceIndex;

    //
    //    Skip blank space
    //
    if ( *C->CharIndex  == ' ' ) {

        while ( *C->CharIndex  == ' ' ) {

            C->CharIndex++;
        }

    }
}

static
DWORD
GetNumber(
    PPARSE_CONTEXT C
    )

/*++

Routine Description:

    Converts the substring delimited by C->MatchBegin and C->MatchEnd into
    a number.

Arguments:

    C - The parse context

Return Value:

    ULONG - The matched string converted to a number


--*/

{
    DWORD   Number;
    CHAR    c;
    PSTR    p = C->MatchEnd+1;

    c = *p;
//    *p = '\0';
    //intf( "Making number: %s\n", C->MatchBegin );
    Number = atol( C->MatchBegin );
//    *p  = c;

    return Number;

}

static
BOOL
ConvertBaudRate (
    DWORD BaudIn,
    PDWORD BaudRate
    )

/*++

Routine Description:

    Validates a baud rate given as an argument to the program, and converts
    it to something that the COMM_DEVICE understands.

Arguments:

    BaudIn - Supplies the baud rate given by the user
    BaudRate - if returning TRUE then the baud rate to use.

Return Value:

    If a valid baud rate then returns TRUE, otherwise FALSE.

--*/

{
    switch ( BaudIn ) {

    case 11:
    case 110:
        *BaudRate = 110;
        break;

    case 15:
    case 150:
        *BaudRate = 150;
        break;

    case 30:
    case 300:
        *BaudRate = 300;
        break;

    case 60:
    case 600:
        *BaudRate = 600;
        break;

    case 12:
    case 1200:
        *BaudRate = 1200;
        break;

    case 24:
    case 2400:
        *BaudRate = 2400;
        break;

    case 48:
    case 4800:
        *BaudRate = 4800;
        break;

    case 96:
    case 9600:
        *BaudRate = 9600;
        break;

    case 19:
    case 19200:
        *BaudRate = 19200;
        break;

    default:

        *BaudRate = BaudIn;

    }

    return TRUE;
}

static
BOOL
ConvertDataBits (
    DWORD DataBitsIn,
    PBYTE DataBitsOut
    )

/*++

Routine Description:

    Validates the number of data bits given as an argument to the program,
    and converts  it to something that the COMM_DEVICE understands.

Arguments:

    DataBitsIn - Supplies the number given by the user
    DataBitsOut - if returning TRUE, then the number of data bits.

Return Value:

    If a valid data bits then TRUE, otherwise FALSE.

--*/

{

    if ( ( DataBitsIn != 5 ) &&
         ( DataBitsIn != 6 ) &&
         ( DataBitsIn != 7 ) &&
         ( DataBitsIn != 8 ) ) {

        return FALSE;

    }

    *DataBitsOut = (BYTE)DataBitsIn;

    return TRUE;

}

static
BOOL
ConvertStopBits (
    DWORD StopBitsIn,
    PBYTE StopBits
    )

/*++

Routine Description:

    Validates a number of stop bits given as an argument to the program,
    and converts it to something that the COMM_DEVICE understands.

Arguments:

    StopBitsIn - Supplies the number given by the user
    StopBits - If returning true then a valid stop bits setting.

Return Value:

    If a valid stop bits setting then TRUE, otherwise false.

--*/

{

    switch ( StopBitsIn ) {

    case 1:
        *StopBits = ONESTOPBIT;
        break;

    case 2:
        *StopBits = TWOSTOPBITS;
        break;

    default:
        return FALSE;

    }

    return TRUE;

}

static
BOOL
ConvertParity (
    CHAR ParityIn,
    PBYTE Parity
    )

/*++

Routine Description:

    Validates a parity given as an argument to the program, and converts
    it to something that the COMM_DEVICE understands.

Arguments:

    ParityIn - Supplies the baud rate given by the user
    Parity - The valid parity if return true.

Return Value:

    If a valid parity setting then TRUE otherwise false.

--*/

{

    //
    //    Set the correct parity value depending on the character.
    //
    switch ( tolower(ParityIn) ) {

    case 'n':
        *Parity = NOPARITY;
        break;

    case 'o':
        *Parity = ODDPARITY;
        break;

    case 'e':
        *Parity = EVENPARITY;
        break;

    case 'm':
        *Parity = MARKPARITY;
        break;

    case 's':
        *Parity = SPACEPARITY;
        break;

    default:
        return FALSE;

    }

    return TRUE;
}

static
BOOL
ConvertDtrControl (
    PSTR IdxBegin,
    PSTR IdxEnd,
    PBYTE DtrControl
    )

/*++

Routine Description:

    Validates a DTR control value given as an argument to the
    program, and converts it to something that the COMM_DEVICE
    understands.

Arguments:

    IdxBegin - Supplies Index of first character
    IdxEnd - Supplies Index of last character
    DtrControl - If returning true, the valid dtr setting.

Return Value:

    DTR_CONTROL -   The DTR control value


--*/

{

    PSTR    p;

    p = IdxBegin;
    if ( (tolower(*p)  == 'o' ) &&
         p++                    &&
         (tolower(*p)  == 'n' ) &&
         (IdxEnd == p)) {


        *DtrControl = DTR_CONTROL_ENABLE;
        return TRUE;

    }

    p = IdxBegin;
    if ( (tolower(*p) == 'o')   &&
         p++                    &&
         (tolower(*p) == 'f')   &&
         p++                    &&
         (tolower(*p) == 'f')   &&
         (IdxEnd == p ) ) {

        *DtrControl =  DTR_CONTROL_DISABLE;
        return TRUE;
    }

    p = IdxBegin;
    if ( (tolower(*p) == 'h')   &&
         p++                    &&
         (tolower(*p++) == 's') &&
         (IdxEnd == p ) ) {

        *DtrControl =  DTR_CONTROL_HANDSHAKE;
        return TRUE;
    }

    return FALSE;
}

static
BOOL
ConvertRtsControl (
    PSTR IdxBegin,
    PSTR IdxEnd,
    PBYTE RtsControl
    )

/*++

Routine Description:

    Validates a RTS control value given as an argument to the
    program, and converts it to something that the COMM_DEVICE
    understands.

Arguments:

    IdxBegin - Supplies Index of first character
    IdxEnd - Supplies Index of last character
    RtsControl - If returning true, the valid rts setting.

Return Value:

    RTS_CONTROL -   The RTS control value

--*/

{

    PSTR    p;
    p = IdxBegin;
    if ( (tolower(*p)  == 'o' ) &&
         p++                    &&
         (tolower(*p)  == 'n' ) &&
         (IdxEnd == p)) {


        *RtsControl = RTS_CONTROL_ENABLE;
        return TRUE;

    }

    p = IdxBegin;
    if ( (tolower(*p) == 'o')   &&
         p++                    &&
         (tolower(*p) == 'f')   &&
         p++                    &&
         (tolower(*p) == 'f')   &&
         (IdxEnd == p ) ) {

        *RtsControl =  RTS_CONTROL_DISABLE;
        return TRUE;
    }

    p = IdxBegin;
    if ( (tolower(*p) == 'h')   &&
         p++                    &&
         (tolower(*p++) == 's') &&
         (IdxEnd == p ) ) {

        *RtsControl =  RTS_CONTROL_HANDSHAKE;
        return TRUE;
    }

    p = IdxBegin;
    if ( (tolower(*p) == 't')   &&
         p++                    &&
         (tolower(*p++) == 'g') &&
         (IdxEnd == p ) ) {

        *RtsControl =  RTS_CONTROL_TOGGLE;
        return TRUE;
    }

    return FALSE;

}

static
NTSTATUS
DeviceNameCompare(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

{

    PPARSE_CONTEXT C = EntryContext;
    UNICODE_STRING uniName;
    ANSI_STRING ansiName;

    RtlInitUnicodeString(
        &uniName,
        ValueData
        );

    if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(
                        &ansiName,
                        &uniName,
                        TRUE
                        ))) {

        //
        // Oh well, couldn't form the name.  Just get out.
        //
        return STATUS_SUCCESS;

    }

    //
    // See if we got a name match.
    //

    if (Match(C,ansiName.Buffer)) {

        //
        // Ok, got a name match, advance past it.
        //

        Advance(C);

        //
        // See if they've got the optional : following the
        // device name.
        //

        if (Match(C,":")) {

            //
            // Go past it.
            //

            Advance(C);

        }

    }
    RtlFreeAnsiString(&ansiName);
    return STATUS_SUCCESS;

}

static
VOID
IgnoreDeviceName(
    IN PPARSE_CONTEXT C
    )

{

    RTL_QUERY_REGISTRY_TABLE qTable[2] = {0};

    //
    // Build the query table.
    //

    qTable[0].QueryRoutine = DeviceNameCompare;
    qTable[0].EntryContext = C;

    RtlQueryRegistryValues(
        RTL_REGISTRY_DEVICEMAP,
        L"SERIALCOMM",
        &qTable[0],
        NULL,
        NULL
        );

}
