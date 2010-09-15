/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ex/hdlsterm.c
 * PURPOSE:         Headless Terminal Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <debug.h>

/* GLOBALS *******************************************************************/

PHEADLESS_GLOBALS HeadlessGlobals;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HdlspSendStringAtBaud(
	IN PCHAR String
	)
{
	/* Send every byte */
	while (*String++ != ANSI_NULL)
	{
		InbvPortPutByte(HeadlessGlobals->TerminalPort, *String);
	}
}

NTSTATUS
NTAPI
HdlspEnableTerminal(
	IN BOOLEAN Enable
	)
{
	/* Enable if requested, as long as this isn't a PCI serial port crashing */
	if ((Enable) &&
		!(HeadlessGlobals->TerminalEnabled) &&
		!((HeadlessGlobals->IsMMIODevice) && (HeadlessGlobals->InBugCheck)))
	{
		/* Initialize the COM port with cportlib */
		HeadlessGlobals->TerminalEnabled = InbvPortInitialize(
			HeadlessGlobals->TerminalBaudRate,
			HeadlessGlobals->TerminalPortNumber,
			HeadlessGlobals->TerminalPortAddress,
			&HeadlessGlobals->TerminalPort,
			HeadlessGlobals->IsMMIODevice);
        if (!HeadlessGlobals->TerminalEnabled) return STATUS_UNSUCCESSFUL;

		/* Cleanup the screen and reset the cursor */
		HdlspSendStringAtBaud("\x1B[2J");
		HdlspSendStringAtBaud("\x1B[H");

		/* Enable FIFO */
		InbvPortEnableFifo(HeadlessGlobals->TerminalPort, TRUE);
	}
	else if (!Enable)
	{
		/* Specific case when headless is being disabled */
		InbvPortTerminate(HeadlessGlobals->TerminalPort);
		HeadlessGlobals->TerminalPort = 0;
		HeadlessGlobals->TerminalEnabled = FALSE;
	}
	return STATUS_SUCCESS;
}

VOID
NTAPI
HeadlessInit(
	IN PLOADER_PARAMETER_BLOCK LoaderBlock
	)
{
	PHEADLESS_LOADER_BLOCK HeadlessBlock;

	HeadlessBlock = LoaderBlock->Extension->HeadlessLoaderBlock;
	if (!HeadlessBlock) return;
	if ((HeadlessBlock->PortNumber > 4) && (HeadlessBlock->UsedBiosSettings)) return;

	HeadlessGlobals = ExAllocatePoolWithTag(
		NonPagedPool,
		sizeof(HEADLESS_GLOBALS),
		'sldH');
	if (!HeadlessGlobals) return;

	/* Zero and copy loader data */
	RtlZeroMemory(HeadlessGlobals, sizeof(HEADLESS_GLOBALS));
	HeadlessGlobals->TerminalPortNumber = HeadlessBlock->PortNumber;
	HeadlessGlobals->TerminalPortAddress = HeadlessBlock->PortAddress;
	HeadlessGlobals->TerminalBaudRate = HeadlessBlock->BaudRate;
	HeadlessGlobals->TerminalParity = HeadlessBlock->Parity;
	HeadlessGlobals->TerminalStopBits = HeadlessBlock->StopBits;
	HeadlessGlobals->UsedBiosSettings = HeadlessBlock->UsedBiosSettings;
	HeadlessGlobals->IsMMIODevice = HeadlessBlock->IsMMIODevice;
	HeadlessGlobals->TerminalType = HeadlessBlock->TerminalType;
	HeadlessGlobals->SystemGUID = HeadlessBlock->SystemGUID;

	/* These two are opposites of each other */
	if (HeadlessGlobals->IsMMIODevice) HeadlessGlobals->IsNonLegacyDevice = TRUE;

	/* Check for a PCI device, warn that this isn't supported */
	if (HeadlessBlock->PciDeviceId != PCI_INVALID_VENDORID)
	{
		DPRINT1("PCI Serial Ports not supported\n");
	}

	/* Log entries are not yet supported */
	DPRINT1("FIXME: No Headless logging support\n");

	/* Windows seems to apply some special hacks for 9600 bps */
	if (HeadlessGlobals->TerminalBaudRate == 9600)
	{
		DPRINT1("Please use other baud rate than 9600bps for now\n");
	}

	/* Enable the terminal */
	HdlspEnableTerminal(TRUE);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
HeadlessDispatch(
    IN HEADLESS_CMD Command,
    IN PVOID InputBuffer,
    IN SIZE_T InputBufferSize,
    OUT PVOID OutputBuffer,
    OUT PSIZE_T OutputBufferSize
	)
{
	//UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
