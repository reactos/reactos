/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            dos/dos32krnl/condrv.c
 * PURPOSE:         DOS32 CON Driver
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "dos.h"
#include "dos/dem.h"

#include "bios/bios.h"

/* PRIVATE VARIABLES **********************************************************/

PDOS_DEVICE_NODE Con = NULL;
BYTE ExtendedCode = 0;

/* PRIVATE FUNCTIONS **********************************************************/

WORD NTAPI ConDrvReadInput(PDOS_DEVICE_NODE Device, DWORD Buffer, PWORD Length)
{
    CHAR Character;
    WORD BytesRead = 0;
    PCHAR Pointer = (PCHAR)FAR_POINTER(Buffer);

    /* Save AX */
    USHORT AX = getAX();

    /*
     * Use BIOS Get Keystroke function
     */
    while (BytesRead < *Length)
    {
        if (!ExtendedCode)
        {
            /* Call the BIOS INT 16h, AH=00h "Get Keystroke" */
            setAH(0x00);
            Int32Call(&DosContext, BIOS_KBD_INTERRUPT);

            /* Retrieve the character in AL (scan code is in AH) */
            Character = getAL();
        }
        else
        {
            /* Return the extended code */
            Character = ExtendedCode;

            /* And then clear it */
            ExtendedCode = 0;
        }

        /* Check if this is a special character */
        if (Character == 0) ExtendedCode = getAH();

        Pointer[BytesRead++] = Character;

        if (Character != 0 && DoEcho)
            DosPrintCharacter(DOS_OUTPUT_HANDLE, Character);

        /* Stop on first carriage return */
        if (Character == '\r')
        {
            if (DoEcho) DosPrintCharacter(DOS_OUTPUT_HANDLE, '\n');
            break;
        }
    }

    *Length = BytesRead;

    /* Restore AX */
    setAX(AX);
    return DOS_DEVSTAT_DONE;
}

WORD NTAPI ConDrvInputStatus(PDOS_DEVICE_NODE Device)
{
    /* Save AX */
    USHORT AX = getAX();

    /* Call the BIOS */
    setAH(0x01); // or 0x11 for enhanced, but what to choose?
    Int32Call(&DosContext, BIOS_KBD_INTERRUPT);

    /* Restore AX */
    setAX(AX);

    /* If ZF is set, set the busy bit */
    if (getZF() && !ExtendedCode) return DOS_DEVSTAT_BUSY;
    else return DOS_DEVSTAT_DONE;
}

WORD NTAPI ConDrvWriteOutput(PDOS_DEVICE_NODE Device, DWORD Buffer, PWORD Length)
{
    WORD BytesWritten;
    PCHAR Pointer = (PCHAR)FAR_POINTER(Buffer);

    /* Save AX */
    USHORT AX = getAX();

    for (BytesWritten = 0; BytesWritten < *Length; BytesWritten++)
    {
        /* Set the character */
        setAL(Pointer[BytesWritten]);

        /* Call the BIOS INT 29h "Fast Console Output" function */
        Int32Call(&DosContext, 0x29);
    }

    /* Restore AX */
    setAX(AX);
    return DOS_DEVSTAT_DONE;
}

WORD NTAPI ConDrvOpen(PDOS_DEVICE_NODE Device)
{
    DPRINT("Handle to %Z opened\n", &Device->Name);
    return DOS_DEVSTAT_DONE;
}

WORD NTAPI ConDrvClose(PDOS_DEVICE_NODE Device)
{
    DPRINT("Handle to %Z closed\n", &Device->Name);
    return DOS_DEVSTAT_DONE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID ConDrvInitialize(VOID)
{
    Con = DosCreateDevice(DOS_DEVATTR_STDIN
                          | DOS_DEVATTR_STDOUT
                          | DOS_DEVATTR_CON
                          | DOS_DEVATTR_CHARACTER,
                          "CON");

    Con->ReadRoutine        = ConDrvReadInput;
    Con->InputStatusRoutine = ConDrvInputStatus;
    Con->WriteRoutine       = ConDrvWriteOutput;
    Con->OpenRoutine        = ConDrvOpen;
    Con->CloseRoutine       = ConDrvClose;
}

VOID ConDrvCleanup(VOID)
{
    if (Con) DosDeleteDevice(Con);
}
