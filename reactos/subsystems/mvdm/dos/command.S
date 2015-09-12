/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            command.S
 * PURPOSE:         DOS32 command.com for NTVDM
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

//
// See http://www.ganino.com/games/scripts/dos-6.0%20source%20code%3F/dev/smartdrv/umbload.asm
// and https://books.google.fr/books?id=rtmJgtfaxz8C&pg=PA256&lpg=PA256&dq=int+21+4b+program+exec+block&source=bl&ots=OfAF7qFwfl&sig=PrW73CE1dzFm3rwrTsYZkC77U4Y&hl=en&sa=X&redir_esc=y#v=onepage&q=int%2021%204b%20program%20exec%20block&f=false
//

/* INCLUDES *******************************************************************/

#include <asm.inc>
#include "asmxtras.inc"
#include <isvbop.inc>

/* DEFINES ********************************************************************/

#define MAX_PATH            260
#define DOS_CMDLINE_LENGTH  127

#define DOS_VERSION     HEX(0005)   // MAKEWORD(5, 00)
#define NTDOS_VERSION   HEX(3205)   // MAKEWORD(5, 50)

#define SYSTEM_PSP      HEX(08)

#define BOP                 .byte HEX(C4), HEX(C4),
// #define BOP_START_DOS       HEX(2C)
#define BOP_CMD             HEX(54)

/**** PSP MEMBERS ****/
#define PSP_VAR(x)  es:[x]
#define PSP         HEX(0000)
#define ParentPsp   HEX(0016)   // word
#define EnvBlock    HEX(002C)   // word
#define CmdLineLen  HEX(0080)   // byte
#define CmdLineStr  HEX(0081)   // byte[DOS_CMDLINE_LENGTH]

/**** DATA stored inside the stack ****/
#define CmdLine BaseStack   // Command line for the program to be started
#define PgmName [CmdLine + (1 + DOS_CMDLINE_LENGTH)]


// WARNING! Using the VAL(x) macro doesn't work with GCC/GAS (because it inserts
// a spurious space in front of the parameter when the macro is expanded).
// So that: 'VAL(foo)' is expanded to: '\ foo', instead of '\foo' as one would expect.

/* NEXT_CMD structure */
STRUCT(NEXT_CMD, 2)
    FIELD_DECL(EnvBlockSeg, word, 0)
    FIELD_DECL(EnvBlockLen, word, 0)
    FIELD_DECL(CurDrive   , word, 0)
    FIELD_DECL(NumDrives  , word, 1)
    FIELD_DECL(CmdLineSeg , word, 0)
    FIELD_DECL(CmdLineOff , word, 0)
    FIELD_DECL(Unknown0   , word, 2)
    FIELD_DECL(ExitCode   , word, 3)
    FIELD_DECL(Unknown1   , word, 4)
    FIELD_DECL(Unknown2   , long, 5)
    FIELD_DECL(CodePage   , word, 6)
    FIELD_DECL(Unknown3   , word, 7)
    FIELD_DECL(Unknown4   , word, 8)
    FIELD_DECL(AppNameSeg , word, 0)
    FIELD_DECL(AppNameOff , word, 0)
    FIELD_DECL(AppNameLen , word, 0)
    FIELD_DECL(Flags      , word, 0)
ENDS(NEXT_CMD)

/* DOS_EXEC_PARAM_BLOCK structure */
STRUCT(DOS_EXEC_PARAM_BLOCK, 2)
    FIELD_DECL(EnvSeg, word, 0)    // Use parent's environment (ours)
    FIELD_DECL(CmdLineOff, word, OFF(CmdLine))
    FIELD_DECL(CmdLineSeg, word, 0)
    FIELD_DECL(Fcb1Off, word, OFF(Fcb1))
    FIELD_DECL(Fcb1Seg, word, 0)
    FIELD_DECL(Fcb2Off, word, OFF(Fcb2))
    FIELD_DECL(Fcb2Seg, word, 0)
ENDS(DOS_EXEC_PARAM_BLOCK)



/* RESIDENT CODE AND DATA *****************************************************/

.code16
// .org HEX(0100)
ASSUME CS:.text, DS:.text, ES:.text


/* CODE *******************************/

EntryPoint:
    jmp Main
.align 2

ResidentMain:
    /*
     * Relocate our stack.
     * Our local stack is big enough for holding the command line and the path
     * to the executable to start, plus a full DOS_REGISTER_STATE and for one pusha 16-bit.
     *
     * FIXME: enlarge it for pushing the needed Load&Exec data.
     */
    cli
    mov ax, ds
    mov ss, ax
    mov bp, offset BaseStack
    lea sp, [bp + (1 + DOS_CMDLINE_LENGTH) + MAX_PATH + 255]
    sti

    /* Resize ourselves */
    mov bx, sp                  // Get size in bytes...
//  sub bx, offset PSP_VAR(PSP)
    add bx, HEX(0F)             // (for rounding to the next paragraph)
    shr bx, 4                   // ... then the number of paragraphs
    mov ah, HEX(4A)
    int HEX(21)

    /* Check whether we need to start the 32-bit command interpreter */
    BOP BOP_CMD, HEX(10)        // Check whether we were started from a new console (SessionId != 0)
    test al, al                 // and we are not reentering (32-bit process starting a 16-bit process).
    jz Run
    cmp word ptr OldParentPsp, SYSTEM_PSP   // Check whether our parent is SYSTEM
    je Run
/********************************/
    mov dx, offset Msg1
    mov ah, HEX(09)
    int HEX(21)
/********************************/
    BOP BOP_CMD, HEX(0A)        // Start 32-bit COMSPEC
    jnc Quit

    /* Loop for new commands to run */
Run:
    /* Initialize the NextCmd structure */
    mov word ptr FIELD(NextCmd, EnvBlockSeg), ds
    mov word ptr FIELD(NextCmd, EnvBlockLen), 0 // FIXME
    mov word ptr FIELD(NextCmd, CmdLineSeg), ds
    mov word ptr FIELD(NextCmd, CmdLineOff), offset CmdLine
    mov word ptr FIELD(NextCmd, AppNameSeg), ds
    mov word ptr FIELD(NextCmd, AppNameOff), offset PgmName

    /* Wait for the next command */
/********************************/
    mov dx, offset Msg2
    mov ah, HEX(09)
    int HEX(21)
/********************************/
    // FIXME: Initialize memory with structure for holding CmdLine etc...
//  mov ds, seg NextCmd
    mov dx, offset NextCmd
    BOP BOP_CMD, HEX(01)
    /* Quit if we shell-out */
    jc Quit

    /* Initialize the DosLoadExec structure */
//  mov word ptr FIELD(DosLoadExec, EnvSeg), 0
    mov word ptr FIELD(DosLoadExec, CmdLineSeg), ds
    mov word ptr FIELD(DosLoadExec, Fcb1Seg), ds
    mov word ptr FIELD(DosLoadExec, Fcb2Seg), ds

    /* Run the command */
    mov ds, word ptr FIELD(NextCmd, AppNameSeg)
    mov dx, word ptr FIELD(NextCmd, AppNameOff)
//  mov es, seg DosLoadExec
    mov bx, offset DosLoadExec
    pusha // Save the registers in case stuff go wrong
    // FIXME: Save also SS !!
    mov ax, HEX(4B00)
    int HEX(21)
    popa  // Restore the registers
    // FIXME: Restore also SS !!

    /* Retrieve and set its exit code. Also detect whether
     * we need to continue or whether we need to quit. */
    // xor ax, ax
    // mov ah, HEX(4D)
    mov ax, HEX(4D00)
    int HEX(21)
    /* Send exit code back to NTVDM */
    mov dx, ax
    BOP BOP_CMD, HEX(0B)

    /* If we don't shell-out, go and get a new app! */
    jc Run

    mov al, HEX(00) // ERROR_SUCCESS

Quit:
    mov bl, al // Save AL in BL
/********************************/
    cmp al, HEX(0A)
    jne XXXX
    mov dx, offset Msg3
    mov ah, HEX(09)
    int HEX(21)
XXXX:
/********************************/
    /* Say bye-bye */
//  mov ds, seg QuitMsg
    mov dx, offset QuitMsg
    mov ah, HEX(09)
    int HEX(21)

    /* Restore our old parent PSP */
    mov ax, word ptr OldParentPsp
    mov PSP_VAR(ParentPsp), ax

    mov al, bl // Restore AL from BL

Exit:
    /* Return to caller (with possible error code in AL) */
    mov ah, HEX(4C)
    int HEX(21)
    int 3

    /* Does not return */

/* DATA *******************************/

QuitMsg:
    .ascii "Bye bye!", CR, LF, "$"

/********************************/
Msg1: .ascii "Starting COMSPEC...", CR, LF, "$"
Msg2: .ascii "Waiting for new command...", CR, LF, "$"
Msg3: .ascii "Bad environment!", CR, LF, "$"
/********************************/

OldParentPsp:   .word 0
CurrentPsp:     .word 0

// BOP_CMD, HEX(01) "Get a new app to start" structure
VAR_STRUCT(NextCmd, NEXT_CMD)

// DOS INT 21h, AH=4Bh "Load and Execute" structure
VAR_STRUCT(DosLoadExec, DOS_EXEC_PARAM_BLOCK)

// Blank FCB blocks needed for DOS INT 21h, AH=4Bh
Fcb1:
    .byte 0
    .space 11, ' '
    .space 25, 0
Fcb2:
    .byte 0
    .space 11, ' '
    .space 25, 0

// The stack resides at the end of the resident code+data
// and it overwrites the transient part.
BaseStack:


/* TRANSIENT CODE AND DATA ****************************************************/

/* DATA *******************************/

WelcomeMsg:
    .ascii "ReactOS DOS32 Command", CR, LF, \
           "Copyright (C) ReactOS Team 2015" , CR, LF, "$"
VerErrMsg:
    .ascii "Incorrect DOS version", CR, LF, "$"

/* CODE *******************************/

.align 2

Main:
    /* Setup segment registers */
    mov ax, cs  // cs contains the PSP segment on entry
    mov ds, ax
    mov es, ax
//  mov fs, ax
//  mov gs, ax
    /* Stack is set to cs:FFFE down to cs:09xx by the DOS.
     * We will need to relocate it before we resize ourselves. */

    /*
     * Verify DOS version:
     * - Check whether we are on DOS 5+ (subject to SETVER);
     * - If so, check our real DOS version and see
     *   whether we are running on NTVDM (version 5.50).
     */
    mov ax, HEX(3000)
    int HEX(21)
    cmp ax, DOS_VERSION     // AH:AL contains minor:major version number
    jne VerErr

    mov ax, HEX(3306)
    int HEX(21)
    cmp bx, NTDOS_VERSION   // BH:BL contains minor:major version number
    je Continue

VerErr:
    /* Display wrong version error message and exit */
//  mov ds, seg VerErrMsg
    mov dx, offset VerErrMsg
    mov ah, HEX(09)
    int HEX(21)
    jmp Exit

Continue:
    /* Save our PSP */
    mov word ptr CurrentPsp, cs
/*
 * The DOS way:
 *
 * mov ah, HEX(51) // DOS 2+ internal, or HEX(62) since DOS 3+
 * int HEX(21)
 * mov word ptr CurrentPsp, bx
 */

    /* Save our old parent PSP */
    mov ax, PSP_VAR(ParentPsp)
    mov word ptr OldParentPsp, ax

    /* Give us shell privileges: set our PSP as our new parent */
    mov ax, word ptr CurrentPsp
    mov PSP_VAR(ParentPsp), ax

    /* Say hello */
//  mov ds, seg WelcomeMsg
    mov dx, offset WelcomeMsg
    mov ah, HEX(09)
    int HEX(21)

    /* Jump to resident code */
    jmp ResidentMain

.endcode16
END

/* EOF */
