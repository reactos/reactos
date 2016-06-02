#include "miniport.h"
#include "storport.h"

typedef struct _AHCI_PORT
{
	ULONG	CLB;		// 0x00, command list base address, 1K-byte aligned
	ULONG	CLBU;		// 0x04, command list base address upper 32 bits
	ULONG	FB;			// 0x08, FIS base address, 256-byte aligned
	ULONG	FBU;		// 0x0C, FIS base address upper 32 bits
	ULONG	IS;			// 0x10, interrupt status
	ULONG	IE;			// 0x14, interrupt enable
	ULONG	CMD;		// 0x18, command and status
	ULONG	RSV0;		// 0x1C, Reserved
	ULONG	TFD;		// 0x20, task file data
	ULONG	SIG;		// 0x24, signature
	ULONG	SSTS;		// 0x28, SATA status (SCR0:SStatus)
	ULONG	SCTL;		// 0x2C, SATA control (SCR2:SControl)
	ULONG	SERR;		// 0x30, SATA error (SCR1:SError)
	ULONG	SACT;		// 0x34, SATA active (SCR3:SActive)
	ULONG	CI;			// 0x38, command issue
	ULONG	SNTF;		// 0x3C, SATA notification (SCR4:SNotification)
	ULONG	FBS;		// 0x40, FIS-based switch control
	ULONG	RSV1[11];	// 0x44 ~ 0x6F, Reserved
	ULONG	Vendor[4];	// 0x70 ~ 0x7F, vendor specific
} AHCI_PORT;

typedef struct _AHCI_MEMORY_REGISTERS
{
	// 0x00 - 0x2B, Generic Host Control
	ULONG CAP;						// 0x00, Host capability
	ULONG GHC;						// 0x04, Global host control
	ULONG IS;						// 0x08, Interrupt status
	ULONG PI;						// 0x0C, Port implemented
	ULONG VS;						// 0x10, Version
	ULONG CCC_CTL;					// 0x14, Command completion coalescing control
	ULONG CCC_PTS;					// 0x18, Command completion coalescing ports
	ULONG EM_LOC;					// 0x1C, Enclosure management location
	ULONG EM_CTL;					// 0x20, Enclosure management control
	ULONG CAP2;						// 0x24, Host capabilities extended
	ULONG BOHC;						// 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	ULONG Reserved[0xA0-0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	ULONG VendorSpecific[0x100-0xA0];

	AHCI_PORT PortList[32];//1~32

} AHCI_MEMORY_REGISTERS, *PAHCI_MEMORY_REGISTERS;

typedef struct _AHCI_ADAPTER_EXTENSION
{
	ULONG	AdapterNumber;
	ULONG	SystemIoBusNumber;
	ULONG	SlotNumber;
	ULONG	AhciBaseAddress;
	ULONG 	IS;					// Interrupt status
	ULONG	PortImplemented;

	USHORT 	VendorID;
	USHORT 	DeviceID;
	USHORT 	RevisionID;

	ULONG 	Version;
	ULONG 	CAP;
	ULONG 	CAP2;
	PAHCI_MEMORY_REGISTERS ABAR_Address;
} AHCI_ADAPTER_EXTENSION, *PAHCI_ADAPTER_EXTENSION;

typedef struct _AHCI_SRB_EXTENSION
{
	ULONG AdapterNumber;
} AHCI_SRB_EXTENSION;