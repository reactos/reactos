/*****************************************************************************
 *
 *      (C) Copyright MICROSOFT Corp., 1996
 *
 *      Title:          ACPITABL.H --- Definitions and descriptions of the various BIOS supplied ACPI tables.
 *
 *      Version:        1.00
 *
 *      Date:           6-17-96
 *
 *      Author:         Jason Clark (jasoncl)
 *
 *------------------------------------------------------------------------------
 *
 *      Change log:
 *
 *         DATE     REV DESCRIPTION
 *      ----------- --- -----------------------------------------------------------
 *
 ****************************************************************************/

//      These map to bios provided structures, so turn on 1 byte packing

#include <pshpack1.h>

#define RSDP_SIGNATURE 0x2052545020445352       // "RSD PTR "

typedef struct  _RSDP   {       // Root System Description Table Pointer Structure

ULONGLONG       Signature;              // 8 UCHAR table signature 'RSD PTR '
UCHAR   Checksum;                       // sum of all UCHARs of structure must = 0
UCHAR   OEMID[6];                       //      String that uniquely ID's the OEM
UCHAR   Reserved[1];            // must be 0
ULONG   RsdtAddress;            // physical address of Root System Description Table

}       RSDP;

typedef RSDP    *PRSDP;

#ifndef NEC_98
#define RSDP_SEARCH_RANGE_BEGIN         0xE0000         // physical address where we begin searching for the RSDP
#else   // NEC_98
#define RSDP_SEARCH_RANGE_BEGIN         0xE8000         // physical address where we begin searching for the RSDP
#endif  // NEC_98
#define RSDP_SEARCH_RANGE_END           0xFFFFF
#define RSDP_SEARCH_RANGE_LENGTH        (RSDP_SEARCH_RANGE_END-RSDP_SEARCH_RANGE_BEGIN+1)
#define RSDP_SEARCH_INTERVAL            16      // search on 16 byte boundaries

typedef struct _DESCRIPTION_HEADER      {       // Header structure appears at the beginning of each ACPI table

ULONG   Signature;                      //      Signature used to identify the type of table
ULONG   Length;                         //      Length of entire table including the DESCRIPTION_HEADER
UCHAR   Revision;                       //      Minor version of ACPI spec to which this table conforms
UCHAR   Checksum;                       //      sum of all bytes in the entire TABLE should = 0
UCHAR   OEMID[6];                       //      String that uniquely ID's the OEM
UCHAR   OEMTableID[8];                  //      String that uniquely ID's this table (used for table patching and replacement).
ULONG   OEMRevision;                    //      OEM supplied table revision number.  Bigger number = newer table.
UCHAR   CreatorID[4];                   //      Vendor ID of utility which created this table.
ULONG   CreatorRev;                     //      Revision of utility that created the table.

}       DESCRIPTION_HEADER;

typedef DESCRIPTION_HEADER      *PDESCRIPTION_HEADER;

// Header constants

#define ACPI_MAX_SIGNATURE       4
#define ACPI_MAX_OEM_ID          6
#define ACPI_MAX_TABLE_ID        8
#define ACPI_MAX_TABLE_STRINGS   ACPI_MAX_SIGNATURE + ACPI_MAX_OEM_ID + ACPI_MAX_TABLE_ID

#define FACS_SIGNATURE  0x53434146      // "FACS"

typedef struct _FACS    {       // Firmware ACPI Control Structure.  Note that this table does not have a header, it is pointed to by the FADT

ULONG   Signature;      //      'FACS'
ULONG   Length;         //      Length of entire firmware ACPI control structure (must be 64 bytes or larger)
ULONG   HardwareSignature;
ULONG   pFirmwareWakingVector;  // physical address of location where the OS needs to put the firmware waking vector
ULONG   GlobalLock;     // 32 bit structure used for sharing Embedded Controller
ULONG   Flags;
UCHAR   Reserved[40];
}       FACS;

typedef FACS    *PFACS;

// FACS.GlobalLock bit field definitions

#define         GL_PENDING_BIT          0x00
#define         GL_PENDING                      (1 << GL_PENDING_BIT)

#define         GL_OWNER_BIT            0x01
#define         GL_OWNER                        (1 << GL_OWNER_BIT)

#define GL_NON_RESERVED_BITS_MASK       (GL_PENDING+GL_OWNED)

// Generic Register Address Structure

typedef struct _GEN_ADDR {
    UCHAR               AddressSpaceID;
    UCHAR               BitWidth;
    UCHAR               BitOffset;
    UCHAR               Reserved;
    PHYSICAL_ADDRESS    Address;
} GEN_ADDR, *PGEN_ADDR;

// FACS Flags definitions

#define         FACS_S4BIOS_SUPPORTED_BIT   0   // flag indicates whether or not the BIOS will save/restore memory around S4
#define         FACS_S4BIOS_SUPPORTED       (1 << FACS_S4BIOS_SUPPORTED_BIT)


#define FADT_SIGNATURE  0x50434146      // "FACP"

typedef struct _FADT    {               // Fixed ACPI description table

DESCRIPTION_HEADER      Header;
ULONG                           facs;                           // Physical address of the Firmware ACPI Control Structure
ULONG                           dsdt;                           // Physical address of the Differentiated System Description Table
UCHAR                           int_model;                      // System's Interrupt mode, 0=Dual PIC, 1=Multiple APIC, >1 reserved
UCHAR                           reserved4;
USHORT                          sci_int_vector;         // Vector of SCI interrupt.
ULONG                           smi_cmd_io_port;        // Address in System I/O Space of the SMI Command port, used to enable and disable ACPI.
UCHAR                           acpi_on_value;          // Value out'd to smi_cmd_port to activate ACPI
UCHAR                           acpi_off_value;         // Value out'd to smi_cmd_port to deactivate ACPI
UCHAR                           s4bios_req;             // Value to write to SMI_CMD to enter the S4 state.
UCHAR                           reserved1;                      // Must Be 0
ULONG                           pm1a_evt_blk_io_port;   // Address in System I/O Space of the PM1a_EVT_BLK register block
ULONG                           pm1b_evt_blk_io_port;   // Address in System I/O Space of the PM1b_EVT_BLK register block
ULONG                           pm1a_ctrl_blk_io_port;  // Address in System I/O Space of the PM1a_CNT_BLK register block
ULONG                           pm1b_ctrl_blk_io_port;  // Address in System I/O Space of the PM1b_CNT_BLK register block
ULONG                           pm2_ctrl_blk_io_port;   // Address in System I/O Space of the PM2_CNT_BLK register block
ULONG                           pm_tmr_blk_io_port;             // Address in System I/O Space of the PM_TMR register block
ULONG                           gp0_blk_io_port;        //      Address in System I/O Space of the GP0 register block
ULONG                           gp1_blk_io_port;        //      Address in System I/O Space of the GP1 register block
UCHAR                           pm1_evt_len;            // number of bytes decoded for PM1_BLK (must be >= 4)
UCHAR                           pm1_ctrl_len;           // number of bytes decoded for PM1_CNT (must be >= 2)
UCHAR                           pm2_ctrl_len;           // number of bytes decoded for PM1a_CNT (must be >= 1)
UCHAR                           pm_tmr_len;                     // number of bytes decoded for PM_TMR (must be >= 4)
UCHAR                           gp0_blk_len;            // number of bytes decoded for GP0_BLK (must be multiple of 2)
UCHAR                           gp1_blk_len;            // number of bytes decoded for GP1_BLK (must be multiple of 2)
UCHAR                           gp1_base;               // index at which GP1 based events start
UCHAR                           reserved2;              // Must Be 0
USHORT                          lvl2_latency;           // Worst case latency in microseconds required to enter and leave the C2 processor state
USHORT                          lvl3_latency;           // Worst case latency in microseconds required to enter and leave the C3 processor state
USHORT                          flush_size;                     // Ignored if WBINVD flag is 1 -- indicates size of memory read to flush dirty lines from
                                                                                // any processors memory caches. A size of zero indicates this is not supported.
USHORT                          flush_stride;           // Ignored if WBINVD flag is 1 -- the memory stride width, in bytes, to perform reads to flush
                                                                                // the processor's memory caches.
UCHAR                           duty_offset;            // zero based index of where the processor's duty cycle setting is within the processor's P_CNT register.
UCHAR                           duty_width;                     // bit width of the processor's duty cycle setting value in the P_CNT register.
                                                                                // a value of zero indicates that processor duty cycle is not supported
UCHAR                           day_alarm_index;
UCHAR                           month_alarm_index;
UCHAR                           century_alarm_index;
USHORT                          boot_arch;
UCHAR                           reserved3[1];
ULONG                           flags;
GEN_ADDR                        reset_reg;
UCHAR                           reset_val;

}       FADT;

typedef FADT            *PFADT;

// definition of FADT.flags bits

// this one bit flag indicates whether or not the WBINVD instruction works properly,if this bit is not set we can not use S2, S3 states, or
// C3 on MP machines
#define         WRITEBACKINVALIDATE_WORKS_BIT           0
#define         WRITEBACKINVALIDATE_WORKS               (1 << WRITEBACKINVALIDATE_WORKS_BIT)

//  this flag indicates if wbinvd works EXCEPT that it does not invalidate the cache
#define         WRITEBACKINVALIDATE_DOESNT_INVALIDATE_BIT   1
#define         WRITEBACKINVALIDATE_DOESNT_INVALIDATE       (1 << WRITEBACKINVALIDATE_DOESNT_INVALIDATE_BIT)

//  this flag indicates that the C1 state is supported on all processors.
#define         SYSTEM_SUPPORTS_C1_BIT                  2
#define         SYSTEM_SUPPORTS_C1                      (1 << SYSTEM_SUPPORTS_C1_BIT)

// this one bit flag indicates whether support for the C2 state is restricted to uniprocessor machines
#define         P_LVL2_UP_ONLY_BIT                      3
#define         P_LVL2_UP_ONLY                          (1 << P_LVL2_UP_ONLY_BIT)

//      this bit indicates whether the PWR button is treated as a fix feature (0) or a generic feature (1)
#define         PWR_BUTTON_GENERIC_BIT                  4
#define         PWR_BUTTON_GENERIC                      (1 << PWR_BUTTON_GENERIC_BIT)

#define         SLEEP_BUTTON_GENERIC_BIT                5
#define         SLEEP_BUTTON_GENERIC                    (1 << SLEEP_BUTTON_GENERIC_BIT)

//      this bit indicates whether the RTC wakeup status is reported in fix register space (0) or not (1)
#define         RTC_WAKE_GENERIC_BIT                    6
#define         RTC_WAKE_GENERIC                        (1 << RTC_WAKE_GENERIC_BIT)

#define         RTC_WAKE_FROM_S4_BIT                    7
#define         RTC_WAKE_FROM_S4                        (1 << RTC_WAKE_FROM_S4_BIT)

// This bit indicates whether the machine implements a 24 or 32 bit timer.
#define         TMR_VAL_EXT_BIT                         8
#define         TMR_VAL_EXT                             (1 << TMR_VAL_EXT_BIT)

// This bit indicates whether the machine supports docking
#define         DCK_CAP_BIT                             9
#define         DCK_CAP                                 (1 << DCK_CAP_BIT)

// This bit indicates whether the machine supports reset
#define         RESET_CAP_BIT                           10
#define         RESET_CAP                               (1 << RESET_CAP_BIT)

//      spec defines maximum entry/exit latency values for C2 and C3, if the FADT indicates that these values are
//      exceeded then we do not use that C state.

#define         C2_MAX_LATENCY  100
#define         C3_MAX_LATENCY  1000

//
// Definition of FADT.boot_arch flags
//

#define LEGACY_DEVICES  1
#define I8042           2


#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY   1
#endif

// Multiple APIC description table

typedef struct _MAPIC   {

DESCRIPTION_HEADER  Header;
ULONG               LocalAPICAddress;   // Physical Address at which each processor can access its local APIC
ULONG               Flags;
ULONG               APICTables[ANYSIZE_ARRAY];  // A list of APIC tables.

}       MAPIC;

typedef MAPIC *PMAPIC;

// Multiple APIC structure flags

#define PCAT_COMPAT_BIT 0   // indicates that the system also has a dual 8259 pic setup.
#define PCAT_COMPAT     (1 << PCAT_COMPAT_BIT)

// APIC Structure Types
#define PROCESSOR_LOCAL_APIC            0
#define IO_APIC                         1
#define ISA_VECTOR_OVERRIDE             2
#define IO_NMI_SOURCE                   3
#define LOCAL_NMI_SOURCE                4
#define PROCESSOR_LOCAL_APIC_LENGTH     8
#define IO_APIC_LENGTH                  12
#define ISA_VECTOR_OVERRIDE_LENGTH      10
#define IO_NMI_SOURCE_LENGTH            8
#define LOCAL_NMI_SOURCE_LENGTH         6

// These defines come from the MPS 1.4 spec, section 4.3.4 and they are referenced as
// such in the ACPI spec.
#define PO_BITS                     3
#define POLARITY_HIGH               1
#define POLARITY_LOW                3
#define POLARITY_CONFORMS_WITH_BUS  0
#define EL_BITS                     0xc
#define EL_BIT_SHIFT                2
#define EL_EDGE_TRIGGERED           4
#define EL_LEVEL_TRIGGERED          0xc
#define EL_CONFORMS_WITH_BUS        0

// The shared beginning info in all APIC Structures

typedef struct _APICTABLE {
   UCHAR Type;
   UCHAR Length;
} APICTABLE;

typedef APICTABLE *PAPICTABLE;

typedef struct _PROCLOCALAPIC   {

UCHAR   Type;   // should be zero to identify a ProcessorLocalAPIC structure
UCHAR   Length; // better be 8
UCHAR   ACPIProcessorID;    // ProcessorID for which this processor is listed in the ACPI processor declaration
                            // operator.
UCHAR   APICID; //  The processor's local APIC ID.
ULONG   Flags;

}       PROCLOCALAPIC;

typedef PROCLOCALAPIC *PPROCLOCALAPIC;

// Processor Local APIC Flags
#define PLAF_ENABLED_BIT    0
#define PLAF_ENABLED        (1 << PLAF_ENABLED_BIT)

typedef struct _IOAPIC  {

UCHAR   Type;
UCHAR   Length; // better be 12
UCHAR   IOAPICID;
UCHAR   Reserved;
ULONG   IOAPICAddress; // Physical address at which this IO APIC resides.
ULONG   SystemVectorBase; // system interrupt vector index for this APIC

}       IOAPIC;

typedef IOAPIC *PIOAPIC;

// Interrupt Source Override
typedef struct {
    UCHAR   Type;                           // Must be 2
    UCHAR   Length;                         // Must be 10
    UCHAR   Bus;                            // Must be 0
    UCHAR   Source;                         // BusRelative IRQ
    ULONG   GlobalSystemInterruptVector;    // Global IRQ
    USHORT  Flags;                          // Same as MPS INTI Flags
} ISA_VECTOR, *PISA_VECTOR;

// I/O Non-Maskable Source Interrupt
typedef struct {
    UCHAR   Type;                           // must be 3
    UCHAR   Length;                         // better be 8
    USHORT  Flags;                          // Same as MPS INTI Flags
    ULONG   GlobalSystemInterruptVector;    // Interrupt connected to NMI
} IO_NMISOURCE, *PIO_NMISOURCE;

// Local Non-Maskable Interrupt Source
typedef struct {
    UCHAR   Type;                           // must be 4
    UCHAR   Length;                         // better be 6
    UCHAR   ProcessorID;                    // which processor?  0xff means all
    USHORT  Flags;
    UCHAR   LINTIN;                         // which LINTIN# signal on the processor
} LOCAL_NMISOURCE, *PLOCAL_NMISOURCE;

typedef struct _SMARTBATTTABLE   {

DESCRIPTION_HEADER  Header;
ULONG   WarningEnergyLevel; // mWh at which the OEM suggests we warn the user that the battery is getting low.
ULONG   LowEnergyLevel;     // mWh at which the OEM suggests we put the machine into a sleep state.
ULONG   CriticalEnergyLevel; // mWH at which the OEM suggests we do an emergency shutdown.

}       SMARTBATTTABLE;

typedef SMARTBATTTABLE *PSMARTBATTTABLE;

#define RSDT_SIGNATURE  0x54445352      // "RSDT"

typedef struct _RSDT    {       // Root System Description Table

DESCRIPTION_HEADER      Header;
ULONG   Tables[ANYSIZE_ARRAY];          // The structure contains an n length array of physical addresses each of which point to another table.

}       RSDT;

typedef RSDT            *PRSDT;

// The below macro uses the min macro to protect against the case where we are running on machine which is compliant with
// a spec prior to .99.  If you had a .92 compliant header and one table pointer we would end of subtracting 32-36 resulting
// in a really big number and hence we would think we had lots and lots of tables...  Using the min macro we end up subtracting
// the length-length getting zero which will be harmless and cause us to fail to load (with a red screen on Win9x) which is
// the best we can do in this case.

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

#define NumTableEntriesFromRSDTPointer(p)   (p->Header.Length-min(p->Header.Length,sizeof(DESCRIPTION_HEADER)))/4


#define APIC_SIGNATURE  0x43495041      // "APIC"
#define DSDT_SIGNATURE  0x54445344      // "DSDT"
#define SSDT_SIGNATURE  0x54445353      // "SSDT"
#define PSDT_SIGNATURE  0x54445350      // "PSDT"
#define SBST_SIGNATURE  0x54534253      // "SBST"
#define DBGP_SIGNATURE  0x50474244      // "DBGP"

typedef struct _DSDT    {       // Differentiated System Description Table

DESCRIPTION_HEADER      Header;
UCHAR                   DiffDefBlock[ANYSIZE_ARRAY];    // this is the AML describing the base system.

}       DSDT;

typedef DSDT            *PDSDT;

//      Resume normal structure packing

#include <poppack.h>

typedef struct {
    UCHAR   NamespaceProcID;
    UCHAR   ApicID;
    UCHAR   NtNumber;
    BOOLEAN Started;
    BOOLEAN Enumerated;
} PROC_LOCAL_APIC, *PPROC_LOCAL_APIC;

extern PROC_LOCAL_APIC HalpProcLocalApicTable[];

//
// Debug Port Table
//

typedef struct _DEBUG_PORT_TABLE {

    DESCRIPTION_HEADER  Header;
    UCHAR               InterfaceType;
    UCHAR               Reserved[3];
    GEN_ADDR            BaseAddress;
} DEBUG_PORT_TABLE, *PDEBUG_PORT_TABLE;

