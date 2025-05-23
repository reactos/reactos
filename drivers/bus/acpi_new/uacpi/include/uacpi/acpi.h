#pragma once

#include <uacpi/platform/compiler.h>
#include <uacpi/helpers.h>
#include <uacpi/types.h>

/*
 * -----------------------------------------------------
 * Common structures provided by the ACPI specification
 * -----------------------------------------------------
 */

#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDT_SIGNATURE "RSDT"
#define ACPI_XSDT_SIGNATURE "XSDT"
#define ACPI_MADT_SIGNATURE "APIC"
#define ACPI_FADT_SIGNATURE "FACP"
#define ACPI_FACS_SIGNATURE "FACS"
#define ACPI_MCFG_SIGNATURE "MCFG"
#define ACPI_HPET_SIGNATURE "HPET"
#define ACPI_SRAT_SIGNATURE "SRAT"
#define ACPI_SLIT_SIGNATURE "SLIT"
#define ACPI_DSDT_SIGNATURE "DSDT"
#define ACPI_SSDT_SIGNATURE "SSDT"
#define ACPI_PSDT_SIGNATURE "PSDT"
#define ACPI_ECDT_SIGNATURE "ECDT"
#define ACPI_RHCT_SIGNATURE "RHCT"

#define ACPI_AS_ID_SYS_MEM       0x00
#define ACPI_AS_ID_SYS_IO        0x01
#define ACPI_AS_ID_PCI_CFG_SPACE 0x02
#define ACPI_AS_ID_EC            0x03
#define ACPI_AS_ID_SMBUS         0x04
#define ACPI_AS_ID_SYS_CMOS      0x05
#define ACPI_AS_ID_PCI_BAR_TGT   0x06
#define ACPI_AS_ID_IPMI          0x07
#define ACPI_AS_ID_GP_IO         0x08
#define ACPI_AS_ID_GENERIC_SBUS  0x09
#define ACPI_AS_ID_PCC           0x0A
#define ACPI_AS_ID_FFH           0x7F
#define ACPI_AS_ID_OEM_BASE      0xC0
#define ACPI_AS_ID_OEM_END       0xFF

#define ACPI_ACCESS_UD    0
#define ACPI_ACCESS_BYTE  1
#define ACPI_ACCESS_WORD  2
#define ACPI_ACCESS_DWORD 3
#define ACPI_ACCESS_QWORD 4

UACPI_PACKED(struct acpi_gas {
    uacpi_u8 address_space_id;
    uacpi_u8 register_bit_width;
    uacpi_u8 register_bit_offset;
    uacpi_u8 access_size;
    uacpi_u64 address;
})
UACPI_EXPECT_SIZEOF(struct acpi_gas, 12);

UACPI_PACKED(struct acpi_rsdp {
    uacpi_char signature[8];
    uacpi_u8 checksum;
    uacpi_char oemid[6];
    uacpi_u8 revision;
    uacpi_u32 rsdt_addr;

    // vvvv available if .revision >= 2.0 only
    uacpi_u32 length;
    uacpi_u64 xsdt_addr;
    uacpi_u8 extended_checksum;
    uacpi_u8 rsvd[3];
})
UACPI_EXPECT_SIZEOF(struct acpi_rsdp, 36);

UACPI_PACKED(struct acpi_sdt_hdr {
    uacpi_char signature[4];
    uacpi_u32 length;
    uacpi_u8 revision;
    uacpi_u8 checksum;
    uacpi_char oemid[6];
    uacpi_char oem_table_id[8];
    uacpi_u32 oem_revision;
    uacpi_u32 creator_id;
    uacpi_u32 creator_revision;
})
UACPI_EXPECT_SIZEOF(struct acpi_sdt_hdr, 36);

UACPI_PACKED(struct acpi_rsdt {
    struct acpi_sdt_hdr hdr;
    uacpi_u32 entries[];
})

UACPI_PACKED(struct acpi_xsdt {
    struct acpi_sdt_hdr hdr;
    uacpi_u64 entries[];
})

UACPI_PACKED(struct acpi_entry_hdr {
    /*
     * - acpi_madt_entry_type for the APIC table
     * - acpi_srat_entry_type for the SRAT table
     */
    uacpi_u8 type;
    uacpi_u8 length;
})

// acpi_madt->flags
#define ACPI_PCAT_COMPAT (1 << 0)

enum acpi_madt_entry_type {
    ACPI_MADT_ENTRY_TYPE_LAPIC = 0,
    ACPI_MADT_ENTRY_TYPE_IOAPIC = 1,
    ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE = 2,
    ACPI_MADT_ENTRY_TYPE_NMI_SOURCE = 3,
    ACPI_MADT_ENTRY_TYPE_LAPIC_NMI = 4,
    ACPI_MADT_ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE = 5,
    ACPI_MADT_ENTRY_TYPE_IOSAPIC = 6,
    ACPI_MADT_ENTRY_TYPE_LSAPIC = 7,
    ACPI_MADT_ENTRY_TYPE_PLATFORM_INTERRUPT_SOURCES = 8,
    ACPI_MADT_ENTRY_TYPE_LOCAL_X2APIC = 9,
    ACPI_MADT_ENTRY_TYPE_LOCAL_X2APIC_NMI = 0xA,
    ACPI_MADT_ENTRY_TYPE_GICC = 0xB,
    ACPI_MADT_ENTRY_TYPE_GICD = 0xC,
    ACPI_MADT_ENTRY_TYPE_GIC_MSI_FRAME = 0xD,
    ACPI_MADT_ENTRY_TYPE_GICR = 0xE,
    ACPI_MADT_ENTRY_TYPE_GIC_ITS = 0xF,
    ACPI_MADT_ENTRY_TYPE_MULTIPROCESSOR_WAKEUP = 0x10,
    ACPI_MADT_ENTRY_TYPE_CORE_PIC = 0x11,
    ACPI_MADT_ENTRY_TYPE_LIO_PIC = 0x12,
    ACPI_MADT_ENTRY_TYPE_HT_PIC = 0x13,
    ACPI_MADT_ENTRY_TYPE_EIO_PIC = 0x14,
    ACPI_MADT_ENTRY_TYPE_MSI_PIC = 0x15,
    ACPI_MADT_ENTRY_TYPE_BIO_PIC = 0x16,
    ACPI_MADT_ENTRY_TYPE_LPC_PIC = 0x17,
    ACPI_MADT_ENTRY_TYPE_RINTC = 0x18,
    ACPI_MADT_ENTRY_TYPE_IMSIC = 0x19,
    ACPI_MADT_ENTRY_TYPE_APLIC = 0x1A,
    ACPI_MADT_ENTRY_TYPE_PLIC = 0x1B,
    ACPI_MADT_ENTRY_TYPE_RESERVED = 0x1C, // 0x1C..0x7F
    ACPI_MADT_ENTRY_TYPE_OEM = 0x80, // 0x80..0xFF
};

UACPI_PACKED(struct acpi_madt {
    struct acpi_sdt_hdr hdr;
    uacpi_u32 local_interrupt_controller_address;
    uacpi_u32 flags;
    struct acpi_entry_hdr entries[];
})
UACPI_EXPECT_SIZEOF(struct acpi_madt, 44);

/*
 * - acpi_madt_lapic->flags
 * - acpi_madt_lsapic->flags
 * - acpi_madt_x2apic->flags
 */
#define ACPI_PIC_ENABLED (1 << 0)
#define ACPI_PIC_ONLINE_CAPABLE (1 << 1)

UACPI_PACKED(struct acpi_madt_lapic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 uid;
    uacpi_u8 id;
    uacpi_u32 flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_lapic, 8);

UACPI_PACKED(struct acpi_madt_ioapic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 id;
    uacpi_u8 rsvd;
    uacpi_u32 address;
    uacpi_u32 gsi_base;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_ioapic, 12);

/*
 * - acpi_madt_interrupt_source_override->flags
 * - acpi_madt_nmi_source->flags
 * - acpi_madt_lapic_nmi->flags
 * - acpi_madt_platform_interrupt_source->flags
 * - acpi_madt_x2apic_nmi->flags
 */
#define ACPI_MADT_POLARITY_MASK 0b11
#define ACPI_MADT_POLARITY_CONFORMING 0b00
#define ACPI_MADT_POLARITY_ACTIVE_HIGH 0b01
#define ACPI_MADT_POLARITY_ACTIVE_LOW 0b11

#define ACPI_MADT_TRIGGERING_MASK 0b1100
#define ACPI_MADT_TRIGGERING_CONFORMING 0b0000
#define ACPI_MADT_TRIGGERING_EDGE 0b0100
#define ACPI_MADT_TRIGGERING_LEVEL 0b1100

UACPI_PACKED(struct acpi_madt_interrupt_source_override {
    struct acpi_entry_hdr hdr;
    uacpi_u8 bus;
    uacpi_u8 source;
    uacpi_u32 gsi;
    uacpi_u16 flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_interrupt_source_override, 10);

UACPI_PACKED(struct acpi_madt_nmi_source {
    struct acpi_entry_hdr hdr;
    uacpi_u16 flags;
    uacpi_u32 gsi;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_nmi_source, 8);

UACPI_PACKED(struct acpi_madt_lapic_nmi {
    struct acpi_entry_hdr hdr;
    uacpi_u8 uid;
    uacpi_u16 flags;
    uacpi_u8 lint;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_lapic_nmi, 6);

UACPI_PACKED(struct acpi_madt_lapic_address_override {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd;
    uacpi_u64 address;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_lapic_address_override, 12);

UACPI_PACKED(struct acpi_madt_iosapic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 id;
    uacpi_u8 rsvd;
    uacpi_u32 gsi_base;
    uacpi_u64 address;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_iosapic, 16);

UACPI_PACKED(struct acpi_madt_lsapic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 acpi_id;
    uacpi_u8 id;
    uacpi_u8 eid;
    uacpi_u8 reserved[3];
    uacpi_u32 flags;
    uacpi_u32 uid;
    uacpi_char uid_string[];
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_lsapic, 16);

// acpi_madt_platform_interrupt_source->platform_flags
#define ACPI_CPEI_PROCESSOR_OVERRIDE (1 << 0)

UACPI_PACKED(struct acpi_madt_platform_interrupt_source {
    struct acpi_entry_hdr hdr;
    uacpi_u16 flags;
    uacpi_u8 type;
    uacpi_u8 processor_id;
    uacpi_u8 processor_eid;
    uacpi_u8 iosapic_vector;
    uacpi_u32 gsi;
    uacpi_u32 platform_flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_platform_interrupt_source, 16);

UACPI_PACKED(struct acpi_madt_x2apic {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd;
    uacpi_u32 id;
    uacpi_u32 flags;
    uacpi_u32 uid;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_x2apic, 16);

UACPI_PACKED(struct acpi_madt_x2apic_nmi {
    struct acpi_entry_hdr hdr;
    uacpi_u16 flags;
    uacpi_u32 uid;
    uacpi_u8 lint;
    uacpi_u8 reserved[3];
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_x2apic_nmi, 12);

// acpi_madt_gicc->flags
#define ACPI_GICC_ENABLED (1 << 0)
#define ACPI_GICC_PERF_INTERRUPT_MODE (1 << 1)
#define ACPI_GICC_VGIC_MAINTENANCE_INTERRUPT_MODE (1 << 2)
#define ACPI_GICC_ONLINE_CAPABLE (1 << 3)

// ACPI_GICC_*_INTERRUPT_MODE
#define ACPI_GICC_TRIGGERING_EDGE 1
#define ACPI_GICC_TRIGGERING_LEVEL 0

UACPI_PACKED(struct acpi_madt_gicc {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd0;
    uacpi_u32 interface_number;
    uacpi_u32 acpi_id;
    uacpi_u32 flags;
    uacpi_u32 parking_protocol_version;
    uacpi_u32 perf_interrupt_gsiv;
    uacpi_u64 parked_address;
    uacpi_u64 address;
    uacpi_u64 gicv;
    uacpi_u64 gich;
    uacpi_u32 vgic_maitenante_interrupt;
    uacpi_u64 gicr_base_address;
    uacpi_u64 mpidr;
    uacpi_u8 power_efficiency_class;
    uacpi_u8 rsvd1;
    uacpi_u16 spe_overflow_interrupt;
    uacpi_u16 trbe_interrupt;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_gicc, 82);

UACPI_PACKED(struct acpi_madt_gicd {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd0;
    uacpi_u32 id;
    uacpi_u64 address;
    uacpi_u32 system_vector_base;
    uacpi_u8 gic_version;
    uacpi_u8 reserved1[3];
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_gicd, 24);

// acpi_madt_gic_msi_frame->flags
#define ACPI_SPI_SELECT (1 << 0)

UACPI_PACKED(struct acpi_madt_gic_msi_frame {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd;
    uacpi_u32 id;
    uacpi_u64 address;
    uacpi_u32 flags;
    uacpi_u16 spi_count;
    uacpi_u16 spi_base;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_gic_msi_frame, 24);

UACPI_PACKED(struct acpi_madt_gicr {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd;
    uacpi_u64 address;
    uacpi_u32 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_gicr, 16);

UACPI_PACKED(struct acpi_madt_gic_its {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd0;
    uacpi_u32 id;
    uacpi_u64 address;
    uacpi_u32 rsvd1;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_gic_its, 20);

UACPI_PACKED(struct acpi_madt_multiprocessor_wakeup {
    struct acpi_entry_hdr hdr;
    uacpi_u16 mailbox_version;
    uacpi_u32 rsvd;
    uacpi_u64 mailbox_address;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_multiprocessor_wakeup, 16);

#define ACPI_CORE_PIC_ENABLED (1 << 0)

UACPI_PACKED(struct acpi_madt_core_pic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u32 acpi_id;
    uacpi_u32 id;
    uacpi_u32 flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_core_pic, 15);

UACPI_PACKED(struct acpi_madt_lio_pic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u64 address;
    uacpi_u16 size;
    uacpi_u16 cascade_vector;
    uacpi_u64 cascade_vector_mapping;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_lio_pic, 23);

UACPI_PACKED(struct acpi_madt_ht_pic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u64 address;
    uacpi_u16 size;
    uacpi_u64 cascade_vector;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_ht_pic, 21);

UACPI_PACKED(struct acpi_madt_eio_pic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u8 cascade_vector;
    uacpi_u8 node;
    uacpi_u64 node_map;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_eio_pic, 13);

UACPI_PACKED(struct acpi_madt_msi_pic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u64 address;
    uacpi_u32 start;
    uacpi_u32 count;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_msi_pic, 19);

UACPI_PACKED(struct acpi_madt_bio_pic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u64 address;
    uacpi_u16 size;
    uacpi_u16 hardware_id;
    uacpi_u16 gsi_base;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_bio_pic, 17);

UACPI_PACKED(struct acpi_madt_lpc_pic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u64 address;
    uacpi_u16 size;
    uacpi_u16 cascade_vector;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_lpc_pic, 15);

UACPI_PACKED(struct acpi_madt_rintc {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u8 rsvd;
    uacpi_u32 flags;
    uacpi_u64 hart_id;
    uacpi_u32 uid;
    uacpi_u32 ext_intc_id;
    uacpi_u64 address;
    uacpi_u32 size;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_rintc, 36);

UACPI_PACKED(struct acpi_madt_imsic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u8 rsvd;
    uacpi_u32 flags;
    uacpi_u16 num_ids;
    uacpi_u16 num_guest_ids;
    uacpi_u8 guest_index_bits;
    uacpi_u8 hart_index_bits;
    uacpi_u8 group_index_bits;
    uacpi_u8 group_index_shift;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_imsic, 16);

UACPI_PACKED(struct acpi_madt_aplic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u8 id;
    uacpi_u32 flags;
    uacpi_u64 hardware_id;
    uacpi_u16 idc_count;
    uacpi_u16 sources_count;
    uacpi_u32 gsi_base;
    uacpi_u64 address;
    uacpi_u32 size;
})
UACPI_EXPECT_SIZEOF(struct acpi_madt_aplic, 36);

UACPI_PACKED(struct acpi_madt_plic {
    struct acpi_entry_hdr hdr;
    uacpi_u8 version;
    uacpi_u8 id;
    uacpi_u64 hardware_id;
    uacpi_u16 sources_count;
    uacpi_u16 max_priority;
    uacpi_u32 flags;
    uacpi_u32 size;
    uacpi_u64 address;
    uacpi_u32 gsi_base;

})
UACPI_EXPECT_SIZEOF(struct acpi_madt_plic, 36);

enum acpi_srat_entry_type {
    ACPI_SRAT_ENTRY_TYPE_PROCESSOR_AFFINITY = 0,
    ACPI_SRAT_ENTRY_TYPE_MEMORY_AFFINITY = 1,
    ACPI_SRAT_ENTRY_TYPE_X2APIC_AFFINITY = 2,
    ACPI_SRAT_ENTRY_TYPE_GICC_AFFINITY = 3,
    ACPI_SRAT_ENTRY_TYPE_GIC_ITS_AFFINITY = 4,
    ACPI_SRAT_ENTRY_TYPE_GENERIC_INITIATOR_AFFINITY = 5,
    ACPI_SRAT_ENTRY_TYPE_GENERIC_PORT_AFFINITY = 6,
    ACPI_SRAT_ENTRY_TYPE_RINTC_AFFINITY = 7,
};

UACPI_PACKED(struct acpi_srat {
    struct acpi_sdt_hdr hdr;
    uacpi_u32 rsvd0;
    uacpi_u64 rsvd1;
    struct acpi_entry_hdr entries[];
})
UACPI_EXPECT_SIZEOF(struct acpi_srat, 48);

/*
 * acpi_srat_processor_affinity->flags
 * acpi_srat_x2apic_affinity->flags
 */
#define ACPI_SRAT_PROCESSOR_ENABLED (1 << 0)

UACPI_PACKED(struct acpi_srat_processor_affinity {
    struct acpi_entry_hdr hdr;
    uacpi_u8 proximity_domain_low;
    uacpi_u8 id;
    uacpi_u32 flags;
    uacpi_u8 eid;
    uacpi_u8 proximity_domain_high[3];
    uacpi_u32 clock_domain;
})
UACPI_EXPECT_SIZEOF(struct acpi_srat_processor_affinity, 16);

// acpi_srat_memory_affinity->flags
#define ACPI_SRAT_MEMORY_ENABLED (1 << 0)
#define ACPI_SRAT_MEMORY_HOTPLUGGABLE (1 << 1)
#define ACPI_SRAT_MEMORY_NON_VOLATILE (1 << 2)

UACPI_PACKED(struct acpi_srat_memory_affinity {
    struct acpi_entry_hdr hdr;
    uacpi_u32 proximity_domain;
    uacpi_u16 rsvd0;
    uacpi_u64 address;
    uacpi_u64 length;
    uacpi_u32 rsvd1;
    uacpi_u32 flags;
    uacpi_u64 rsdv2;
})
UACPI_EXPECT_SIZEOF(struct acpi_srat_memory_affinity, 40);

UACPI_PACKED(struct acpi_srat_x2apic_affinity {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd0;
    uacpi_u32 proximity_domain;
    uacpi_u32 id;
    uacpi_u32 flags;
    uacpi_u32 clock_domain;
    uacpi_u32 rsvd1;
})
UACPI_EXPECT_SIZEOF(struct acpi_srat_x2apic_affinity, 24);

// acpi_srat_gicc_affinity->flags
#define ACPI_SRAT_GICC_ENABLED (1 << 0)

UACPI_PACKED(struct acpi_srat_gicc_affinity {
    struct acpi_entry_hdr hdr;
    uacpi_u32 proximity_domain;
    uacpi_u32 uid;
    uacpi_u32 flags;
    uacpi_u32 clock_domain;
})
UACPI_EXPECT_SIZEOF(struct acpi_srat_gicc_affinity, 18);

UACPI_PACKED(struct acpi_srat_gic_its_affinity {
    struct acpi_entry_hdr hdr;
    uacpi_u32 proximity_domain;
    uacpi_u16 rsvd;
    uacpi_u32 id;
})
UACPI_EXPECT_SIZEOF(struct acpi_srat_gic_its_affinity, 12);

// acpi_srat_generic_affinity->flags
#define ACPI_GENERIC_AFFINITY_ENABLED (1 << 0)
#define ACPI_GENERIC_AFFINITY_ARCH_TRANSACTIONS (1 << 1)

UACPI_PACKED(struct acpi_srat_generic_affinity {
    struct acpi_entry_hdr hdr;
    uacpi_u8 rsvd0;
    uacpi_u8 handle_type;
    uacpi_u32 proximity_domain;
    uacpi_u8 handle[16];
    uacpi_u32 flags;
    uacpi_u32 rsvd1;
})
UACPI_EXPECT_SIZEOF(struct acpi_srat_generic_affinity, 32);

// acpi_srat_rintc_affinity->flags
#define ACPI_SRAT_RINTC_AFFINITY_ENABLED (1 << 0)

UACPI_PACKED(struct acpi_srat_rintc_affinity {
    struct acpi_entry_hdr hdr;
    uacpi_u16 rsvd;
    uacpi_u32 proximity_domain;
    uacpi_u32 uid;
    uacpi_u32 flags;
    uacpi_u32 clock_domain;
})
UACPI_EXPECT_SIZEOF(struct acpi_srat_rintc_affinity, 20);

UACPI_PACKED(struct acpi_slit {
    struct acpi_sdt_hdr hdr;
    uacpi_u64 num_localities;
    uacpi_u8 matrix[];
})
UACPI_EXPECT_SIZEOF(struct acpi_slit, 44);

/*
 * acpi_gtdt->el*_flags
 * acpi_gtdt_timer_entry->physical_flags
 * acpi_gtdt_timer_entry->virtual_flags
 * acpi_gtdt_watchdog->flags
 */
#define ACPI_GTDT_TRIGGERING (1 << 0)
#define ACPI_GTDT_TRIGGERING_EDGE 1
#define ACPI_GTDT_TRIGGERING_LEVEL 0

/*
 * acpi_gtdt->el*_flags
 * acpi_gtdt_timer_entry->physical_flags
 * acpi_gtdt_timer_entry->virtual_flags
 * acpi_gtdt_watchdog->flags
 */
#define ACPI_GTDT_POLARITY (1 << 1)
#define ACPI_GTDT_POLARITY_ACTIVE_LOW 1
#define ACPI_GTDT_POLARITY_ACTIVE_HIGH 0

// acpi_gtdt->el*_flags
#define ACPI_GTDT_ALWAYS_ON_CAPABLE (1 << 2)

UACPI_PACKED(struct acpi_gtdt {
    struct acpi_sdt_hdr hdr;
    uacpi_u64 cnt_control_base;
    uacpi_u32 rsvd;
    uacpi_u32 el1_secure_gsiv;
    uacpi_u32 el1_secure_flags;
    uacpi_u32 el1_non_secure_gsiv;
    uacpi_u32 el1_non_secure_flags;
    uacpi_u32 el1_virtual_gsiv;
    uacpi_u32 el1_virtual_flags;
    uacpi_u32 el2_gsiv;
    uacpi_u32 el2_flags;
    uacpi_u64 cnt_read_base;
    uacpi_u32 platform_timer_count;
    uacpi_u32 platform_timer_offset;

    // revision >= 3
    uacpi_u32 el2_virtual_gsiv;
    uacpi_u32 el2_virtual_flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_gtdt, 104);

enum acpi_gtdt_entry_type {
    ACPI_GTDT_ENTRY_TYPE_TIMER = 0,
    ACPI_GTDT_ENTRY_TYPE_WATCHDOG = 1,
};

UACPI_PACKED(struct acpi_gtdt_entry_hdr {
    uacpi_u8 type;
    uacpi_u16 length;
})

UACPI_PACKED(struct acpi_gtdt_timer {
    struct acpi_gtdt_entry_hdr hdr;
    uacpi_u8 rsvd;
    uacpi_u64 cnt_ctl_base;
    uacpi_u32 timer_count;
    uacpi_u32 timer_offset;
})
UACPI_EXPECT_SIZEOF(struct acpi_gtdt_timer, 20);

// acpi_gtdt_timer_entry->common_flags
#define ACPI_GTDT_TIMER_ENTRY_SECURE (1 << 0)
#define ACPI_GTDT_TIMER_ENTRY_ALWAYS_ON_CAPABLE (1 << 1)

UACPI_PACKED(struct acpi_gtdt_timer_entry {
    uacpi_u8 frame_number;
    uacpi_u8 rsvd[3];
    uacpi_u64 cnt_base;
    uacpi_u64 el0_cnt_base;
    uacpi_u32 physical_gsiv;
    uacpi_u32 physical_flags;
    uacpi_u32 virtual_gsiv;
    uacpi_u32 virtual_flags;
    uacpi_u32 common_flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_gtdt_timer_entry, 40);

// acpi_gtdt_watchdog->flags
#define ACPI_GTDT_WATCHDOG_SECURE (1 << 2)

UACPI_PACKED(struct acpi_gtdt_watchdog {
    struct acpi_gtdt_entry_hdr hdr;
    uacpi_u8 rsvd;
    uacpi_u64 refresh_frame;
    uacpi_u64 control_frame;
    uacpi_u32 gsiv;
    uacpi_u32 flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_gtdt_watchdog, 28);

// acpi_fdt->iapc_flags
#define ACPI_IA_PC_LEGACY_DEVS  (1 << 0)
#define ACPI_IA_PC_8042         (1 << 1)
#define ACPI_IA_PC_NO_VGA       (1 << 2)
#define ACPI_IA_PC_NO_MSI       (1 << 3)
#define ACPI_IA_PC_NO_PCIE_ASPM (1 << 4)
#define ACPI_IA_PC_NO_CMOS_RTC  (1 << 5)

// acpi_fdt->flags
#define ACPI_WBINVD                    (1 << 0)
#define ACPI_WBINVD_FLUSH              (1 << 1)
#define ACPI_PROC_C1                   (1 << 2)
#define ACPI_P_LVL2_UP                 (1 << 3)
#define ACPI_PWR_BUTTON                (1 << 4)
#define ACPI_SLP_BUTTON                (1 << 5)
#define ACPI_FIX_RTC                   (1 << 6)
#define ACPI_RTC_S4                    (1 << 7)
#define ACPI_TMR_VAL_EXT               (1 << 8)
#define ACPI_DCK_CAP                   (1 << 9)
#define ACPI_RESET_REG_SUP             (1 << 10)
#define ACPI_SEALED_CASE               (1 << 11)
#define ACPI_HEADLESS                  (1 << 12)
#define ACPI_CPU_SW_SLP                (1 << 13)
#define ACPI_PCI_EXP_WAK               (1 << 14)
#define ACPI_USE_PLATFORM_CLOCK        (1 << 15)
#define ACPI_S4_RTC_STS_VALID          (1 << 16)
#define ACPI_REMOTE_POWER_ON_CAPABLE   (1 << 17)
#define ACPI_FORCE_APIC_CLUSTER_MODEL  (1 << 18)
#define ACPI_FORCE_APIC_PHYS_DEST_MODE (1 << 19)
#define ACPI_HW_REDUCED_ACPI           (1 << 20)
#define ACPI_LOW_POWER_S0_IDLE_CAPABLE (1 << 21)

// acpi_fdt->arm_flags
#define ACPI_ARM_PSCI_COMPLIANT (1 << 0)
#define ACPI_ARM_PSCI_USE_HVC   (1 << 1)

UACPI_PACKED(struct acpi_fadt {
    struct acpi_sdt_hdr hdr;
    uacpi_u32 firmware_ctrl;
    uacpi_u32 dsdt;
    uacpi_u8 int_model;
    uacpi_u8 preferred_pm_profile;
    uacpi_u16 sci_int;
    uacpi_u32 smi_cmd;
    uacpi_u8 acpi_enable;
    uacpi_u8 acpi_disable;
    uacpi_u8 s4bios_req;
    uacpi_u8 pstate_cnt;
    uacpi_u32 pm1a_evt_blk;
    uacpi_u32 pm1b_evt_blk;
    uacpi_u32 pm1a_cnt_blk;
    uacpi_u32 pm1b_cnt_blk;
    uacpi_u32 pm2_cnt_blk;
    uacpi_u32 pm_tmr_blk;
    uacpi_u32 gpe0_blk;
    uacpi_u32 gpe1_blk;
    uacpi_u8 pm1_evt_len;
    uacpi_u8 pm1_cnt_len;
    uacpi_u8 pm2_cnt_len;
    uacpi_u8 pm_tmr_len;
    uacpi_u8 gpe0_blk_len;
    uacpi_u8 gpe1_blk_len;
    uacpi_u8 gpe1_base;
    uacpi_u8 cst_cnt;
    uacpi_u16 p_lvl2_lat;
    uacpi_u16 p_lvl3_lat;
    uacpi_u16 flush_size;
    uacpi_u16 flush_stride;
    uacpi_u8 duty_offset;
    uacpi_u8 duty_width;
    uacpi_u8 day_alrm;
    uacpi_u8 mon_alrm;
    uacpi_u8 century;
    uacpi_u16 iapc_boot_arch;
    uacpi_u8 rsvd;
    uacpi_u32 flags;
    struct acpi_gas reset_reg;
    uacpi_u8 reset_value;
    uacpi_u16 arm_boot_arch;
    uacpi_u8 fadt_minor_verison;
    uacpi_u64 x_firmware_ctrl;
    uacpi_u64 x_dsdt;
    struct acpi_gas x_pm1a_evt_blk;
    struct acpi_gas x_pm1b_evt_blk;
    struct acpi_gas x_pm1a_cnt_blk;
    struct acpi_gas x_pm1b_cnt_blk;
    struct acpi_gas x_pm2_cnt_blk;
    struct acpi_gas x_pm_tmr_blk;
    struct acpi_gas x_gpe0_blk;
    struct acpi_gas x_gpe1_blk;
    struct acpi_gas sleep_control_reg;
    struct acpi_gas sleep_status_reg;
    uacpi_u64 hypervisor_vendor_identity;
})
UACPI_EXPECT_SIZEOF(struct acpi_fadt, 276);

// acpi_facs->flags
#define ACPI_S4BIOS_F               (1 << 0)
#define ACPI_64BIT_WAKE_SUPPORTED_F (1 << 1)

// acpi_facs->ospm_flags
#define ACPI_64BIT_WAKE_F           (1 << 0)

struct acpi_facs {
    uacpi_char signature[4];
    uacpi_u32 length;
    uacpi_u32 hardware_signature;
    uacpi_u32 firmware_waking_vector;
    uacpi_u32 global_lock;
    uacpi_u32 flags;
    uacpi_u64 x_firmware_waking_vector;
    uacpi_u8 version;
    uacpi_char rsvd0[3];
    uacpi_u32 ospm_flags;
    uacpi_char rsvd1[24];
};
UACPI_EXPECT_SIZEOF(struct acpi_facs, 64);

UACPI_PACKED(struct acpi_mcfg_allocation {
    uacpi_u64 address;
    uacpi_u16 segment;
    uacpi_u8 start_bus;
    uacpi_u8 end_bus;
    uacpi_u32 rsvd;
})
UACPI_EXPECT_SIZEOF(struct acpi_mcfg_allocation, 16);

UACPI_PACKED(struct acpi_mcfg {
    struct acpi_sdt_hdr hdr;
    uacpi_u64 rsvd;
    struct acpi_mcfg_allocation entries[];
})
UACPI_EXPECT_SIZEOF(struct acpi_mcfg, 44);

// acpi_hpet->block_id
#define ACPI_HPET_PCI_VENDOR_ID_SHIFT 16
#define ACPI_HPET_LEGACY_REPLACEMENT_IRQ_ROUTING_CAPABLE (1 << 15)
#define ACPI_HPET_COUNT_SIZE_CAP (1 << 13)
#define ACPI_HPET_NUMBER_OF_COMPARATORS_SHIFT 8
#define ACPI_HPET_NUMBER_OF_COMPARATORS_MASK 0b11111
#define ACPI_HPET_HARDWARE_REV_ID_MASK 0b11111111

// acpi_hpet->flags
#define ACPI_HPET_PAGE_PROTECTION_MASK 0b11
#define ACPI_HPET_PAGE_NO_PROTECTION 0
#define ACPI_HPET_PAGE_4K_PROTECTED 1
#define ACPI_HPET_PAGE_64K_PROTECTED 2

UACPI_PACKED(struct acpi_hpet {
    struct acpi_sdt_hdr hdr;
    uacpi_u32 block_id;
    struct acpi_gas address;
    uacpi_u8 number;
    uacpi_u16 min_clock_tick;
    uacpi_u8 flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_hpet, 56);

// PM1{a,b}_STS
#define ACPI_PM1_STS_TMR_STS_IDX 0
#define ACPI_PM1_STS_BM_STS_IDX 4
#define ACPI_PM1_STS_GBL_STS_IDX 5
#define ACPI_PM1_STS_PWRBTN_STS_IDX 8
#define ACPI_PM1_STS_SLPBTN_STS_IDX 9
#define ACPI_PM1_STS_RTC_STS_IDX 10
#define ACPI_PM1_STS_IGN0_IDX 11
#define ACPI_PM1_STS_PCIEXP_WAKE_STS_IDX 14
#define ACPI_PM1_STS_WAKE_STS_IDX 15

#define ACPI_PM1_STS_TMR_STS_MASK (1 << ACPI_PM1_STS_TMR_STS_IDX)
#define ACPI_PM1_STS_BM_STS_MASK (1 << ACPI_PM1_STS_BM_STS_IDX)
#define ACPI_PM1_STS_GBL_STS_MASK (1 << ACPI_PM1_STS_GBL_STS_IDX)
#define ACPI_PM1_STS_PWRBTN_STS_MASK (1 << ACPI_PM1_STS_PWRBTN_STS_IDX)
#define ACPI_PM1_STS_SLPBTN_STS_MASK (1 << ACPI_PM1_STS_SLPBTN_STS_IDX)
#define ACPI_PM1_STS_RTC_STS_MASK (1 << ACPI_PM1_STS_RTC_STS_IDX)
#define ACPI_PM1_STS_IGN0_MASK (1 << ACPI_PM1_STS_IGN0_IDX)
#define ACPI_PM1_STS_PCIEXP_WAKE_STS_MASK (1 << ACPI_PM1_STS_PCIEXP_WAKE_STS_IDX)
#define ACPI_PM1_STS_WAKE_STS_MASK (1 << ACPI_PM1_STS_WAKE_STS_IDX)

#define ACPI_PM1_STS_CLEAR 1

// PM1{a,b}_EN
#define ACPI_PM1_EN_TMR_EN_IDX 0
#define ACPI_PM1_EN_GBL_EN_IDX 5
#define ACPI_PM1_EN_PWRBTN_EN_IDX 8
#define ACPI_PM1_EN_SLPBTN_EN_IDX 9
#define ACPI_PM1_EN_RTC_EN_IDX 10
#define ACPI_PM1_EN_PCIEXP_WAKE_DIS_IDX 14

#define ACPI_PM1_EN_TMR_EN_MASK (1 << ACPI_PM1_EN_TMR_EN_IDX)
#define ACPI_PM1_EN_GBL_EN_MASK (1 << ACPI_PM1_EN_GBL_EN_IDX)
#define ACPI_PM1_EN_PWRBTN_EN_MASK (1 << ACPI_PM1_EN_PWRBTN_EN_IDX)
#define ACPI_PM1_EN_SLPBTN_EN_MASK (1 << ACPI_PM1_EN_SLPBTN_EN_IDX)
#define ACPI_PM1_EN_RTC_EN_MASK (1 << ACPI_PM1_EN_RTC_EN_IDX)
#define ACPI_PM1_EN_PCIEXP_WAKE_DIS_MASK (1 << ACPI_PM1_EN_PCIEXP_WAKE_DIS_IDX)

// PM1{a,b}_CNT_BLK
#define ACPI_PM1_CNT_SCI_EN_IDX 0
#define ACPI_PM1_CNT_BM_RLD_IDX 1
#define ACPI_PM1_CNT_GBL_RLS_IDX 2
#define ACPI_PM1_CNT_RSVD0_IDX 3
#define ACPI_PM1_CNT_RSVD1_IDX 4
#define ACPI_PM1_CNT_RSVD2_IDX 5
#define ACPI_PM1_CNT_RSVD3_IDX 6
#define ACPI_PM1_CNT_RSVD4_IDX 7
#define ACPI_PM1_CNT_RSVD5_IDX 8
#define ACPI_PM1_CNT_IGN0_IDX 9
#define ACPI_PM1_CNT_SLP_TYP_IDX 10
#define ACPI_PM1_CNT_SLP_EN_IDX 13
#define ACPI_PM1_CNT_RSVD6_IDX 14
#define ACPI_PM1_CNT_RSVD7_IDX 15

#define ACPI_SLP_TYP_MAX 0b111

#define ACPI_PM1_CNT_SCI_EN_MASK (1 << ACPI_PM1_CNT_SCI_EN_IDX)
#define ACPI_PM1_CNT_BM_RLD_MASK (1 << ACPI_PM1_CNT_BM_RLD_IDX)
#define ACPI_PM1_CNT_GBL_RLS_MASK (1 << ACPI_PM1_CNT_GBL_RLS_IDX)
#define ACPI_PM1_CNT_SLP_TYP_MASK (ACPI_SLP_TYP_MAX << ACPI_PM1_CNT_SLP_TYP_IDX)
#define ACPI_PM1_CNT_SLP_EN_MASK (1 << ACPI_PM1_CNT_SLP_EN_IDX)

/*
 * SCI_EN is not in this mask even though the spec says it must be preserved.
 * This is because it's known to be bugged on some hardware that relies on
 * software writing 1 to it after resume (as indicated by a similar comment in
 * ACPICA)
 */
#define ACPI_PM1_CNT_PRESERVE_MASK ( \
    (1 << ACPI_PM1_CNT_RSVD0_IDX) |  \
    (1 << ACPI_PM1_CNT_RSVD1_IDX) |  \
    (1 << ACPI_PM1_CNT_RSVD2_IDX) |  \
    (1 << ACPI_PM1_CNT_RSVD3_IDX) |  \
    (1 << ACPI_PM1_CNT_RSVD4_IDX) |  \
    (1 << ACPI_PM1_CNT_RSVD5_IDX) |  \
    (1 << ACPI_PM1_CNT_IGN0_IDX ) |  \
    (1 << ACPI_PM1_CNT_RSVD6_IDX) |  \
    (1 << ACPI_PM1_CNT_RSVD7_IDX)    \
)

// PM2_CNT
#define ACPI_PM2_CNT_ARB_DIS_IDX 0
#define ACPI_PM2_CNT_ARB_DIS_MASK (1 << ACPI_PM2_CNT_ARB_DIS_IDX)

// All bits are reserved but this first one
#define ACPI_PM2_CNT_PRESERVE_MASK (~((uacpi_u64)ACPI_PM2_CNT_ARB_DIS_MASK))

// SLEEP_CONTROL_REG
#define ACPI_SLP_CNT_RSVD0_IDX 0
#define ACPI_SLP_CNT_IGN0_IDX 1
#define ACPI_SLP_CNT_SLP_TYP_IDX 2
#define ACPI_SLP_CNT_SLP_EN_IDX 5
#define ACPI_SLP_CNT_RSVD1_IDX 6
#define ACPI_SLP_CNT_RSVD2_IDX 7

#define ACPI_SLP_CNT_SLP_TYP_MASK (ACPI_SLP_TYP_MAX << ACPI_SLP_CNT_SLP_TYP_IDX)
#define ACPI_SLP_CNT_SLP_EN_MASK (1 << ACPI_SLP_CNT_SLP_EN_IDX)

#define ACPI_SLP_CNT_PRESERVE_MASK ( \
    (1 << ACPI_SLP_CNT_RSVD0_IDX) |  \
    (1 << ACPI_SLP_CNT_IGN0_IDX)  |  \
    (1 << ACPI_SLP_CNT_RSVD1_IDX) |  \
    (1 << ACPI_SLP_CNT_RSVD2_IDX)    \
)

// SLEEP_STATUS_REG
#define ACPI_SLP_STS_WAK_STS_IDX 7

#define ACPI_SLP_STS_WAK_STS_MASK (1 << ACPI_SLP_STS_WAK_STS_IDX)

// All bits are reserved but this last one
#define ACPI_SLP_STS_PRESERVE_MASK (~((uacpi_u64)ACPI_SLP_STS_WAK_STS_MASK))

#define ACPI_SLP_STS_CLEAR 1

UACPI_PACKED(struct acpi_dsdt {
    struct acpi_sdt_hdr hdr;
    uacpi_u8 definition_block[];
})

UACPI_PACKED(struct acpi_ssdt {
    struct acpi_sdt_hdr hdr;
    uacpi_u8 definition_block[];
})

/*
 * ACPI 6.5 specification:
 * Bit [0] - Set if the device is present.
 * Bit [1] - Set if the device is enabled and decoding its resources.
 * Bit [2] - Set if the device should be shown in the UI.
 * Bit [3] - Set if the device is functioning properly (cleared if device
 *           failed its diagnostics).
 * Bit [4] - Set if the battery is present.
 */
#define ACPI_STA_RESULT_DEVICE_PRESENT (1 << 0)
#define ACPI_STA_RESULT_DEVICE_ENABLED (1 << 1)
#define ACPI_STA_RESULT_DEVICE_SHOWN_IN_UI (1 << 2)
#define ACPI_STA_RESULT_DEVICE_FUNCTIONING (1 << 3)
#define ACPI_STA_RESULT_DEVICE_BATTERY_PRESENT (1 << 4)

#define ACPI_REG_DISCONNECT 0
#define ACPI_REG_CONNECT 1

UACPI_PACKED(struct acpi_ecdt {
    struct acpi_sdt_hdr hdr;
    struct acpi_gas ec_control;
    struct acpi_gas ec_data;
    uacpi_u32 uid;
    uacpi_u8 gpe_bit;
    uacpi_char ec_id[];
})
UACPI_EXPECT_SIZEOF(struct acpi_ecdt, 65);

UACPI_PACKED(struct acpi_rhct_hdr {
    uacpi_u16 type;
    uacpi_u16 length;
    uacpi_u16 revision;
})
UACPI_EXPECT_SIZEOF(struct acpi_rhct_hdr, 6);

// acpi_rhct->flags
#define ACPI_TIMER_CANNOT_WAKE_CPU (1 << 0)

UACPI_PACKED(struct acpi_rhct {
    struct acpi_sdt_hdr hdr;
    uacpi_u32 flags;
    uacpi_u64 timebase_frequency;
    uacpi_u32 node_count;
    uacpi_u32 nodes_offset;
    struct acpi_rhct_hdr entries[];
})
UACPI_EXPECT_SIZEOF(struct acpi_rhct, 56);

enum acpi_rhct_entry_type {
    ACPI_RHCT_ENTRY_TYPE_ISA_STRING = 0,
    ACPI_RHCT_ENTRY_TYPE_CMO = 1,
    ACPI_RHCT_ENTRY_TYPE_MMU = 2,
    ACPI_RHCT_ENTRY_TYPE_HART_INFO = 65535,
};

UACPI_PACKED(struct acpi_rhct_isa_string {
    struct acpi_rhct_hdr hdr;
    uacpi_u16 length;
    uacpi_u8 isa[];
})
UACPI_EXPECT_SIZEOF(struct acpi_rhct_isa_string, 8);

UACPI_PACKED(struct acpi_rhct_cmo {
    struct acpi_rhct_hdr hdr;
    uacpi_u8 rsvd;
    uacpi_u8 cbom_size;
    uacpi_u8 cbop_size;
    uacpi_u8 cboz_size;
})
UACPI_EXPECT_SIZEOF(struct acpi_rhct_cmo, 10);

enum acpi_rhct_mmu_type {
    ACPI_RHCT_MMU_TYPE_SV39 = 0,
    ACPI_RHCT_MMU_TYPE_SV48 = 1,
    ACPI_RHCT_MMU_TYPE_SV57 = 2,
};

UACPI_PACKED(struct acpi_rhct_mmu {
    struct acpi_rhct_hdr hdr;
    uacpi_u8 rsvd;
    uacpi_u8 type;
})
UACPI_EXPECT_SIZEOF(struct acpi_rhct_mmu, 8);

UACPI_PACKED(struct acpi_rhct_hart_info {
    struct acpi_rhct_hdr hdr;
    uacpi_u16 offset_count;
    uacpi_u32 uid;
    uacpi_u32 offsets[];
})
UACPI_EXPECT_SIZEOF(struct acpi_rhct_hart_info, 12);

#define ACPI_LARGE_ITEM (1 << 7)

#define ACPI_SMALL_ITEM_NAME_IDX 3
#define ACPI_SMALL_ITEM_NAME_MASK 0b1111
#define ACPI_SMALL_ITEM_LENGTH_MASK 0b111

#define ACPI_LARGE_ITEM_NAME_MASK 0b1111111

// Small items
#define ACPI_RESOURCE_IRQ 0x04
#define ACPI_RESOURCE_DMA 0x05
#define ACPI_RESOURCE_START_DEPENDENT 0x06
#define ACPI_RESOURCE_END_DEPENDENT 0x07
#define ACPI_RESOURCE_IO 0x08
#define ACPI_RESOURCE_FIXED_IO 0x09
#define ACPI_RESOURCE_FIXED_DMA 0x0A
#define ACPI_RESOURCE_VENDOR_TYPE0 0x0E
#define ACPI_RESOURCE_END_TAG 0x0F

// Large items
#define ACPI_RESOURCE_MEMORY24 0x01
#define ACPI_RESOURCE_GENERIC_REGISTER 0x02
#define ACPI_RESOURCE_VENDOR_TYPE1 0x04
#define ACPI_RESOURCE_MEMORY32 0x05
#define ACPI_RESOURCE_FIXED_MEMORY32 0x06
#define ACPI_RESOURCE_ADDRESS32 0x07
#define ACPI_RESOURCE_ADDRESS16 0x08
#define ACPI_RESOURCE_EXTENDED_IRQ 0x09
#define ACPI_RESOURCE_ADDRESS64 0x0A
#define ACPI_RESOURCE_ADDRESS64_EXTENDED 0x0B
#define ACPI_RESOURCE_GPIO_CONNECTION 0x0C
#define ACPI_RESOURCE_PIN_FUNCTION 0x0D
#define ACPI_RESOURCE_SERIAL_CONNECTION 0x0E
#define ACPI_RESOURCE_PIN_CONFIGURATION 0x0F
#define ACPI_RESOURCE_PIN_GROUP 0x10
#define ACPI_RESOURCE_PIN_GROUP_FUNCTION 0x11
#define ACPI_RESOURCE_PIN_GROUP_CONFIGURATION 0x12
#define ACPI_RESOURCE_CLOCK_INPUT 0x13

/*
 * Resources as encoded by the raw AML byte stream.
 * For decode API & human usable structures refer to uacpi/resources.h
 */
UACPI_PACKED(struct acpi_small_item {
    uacpi_u8 type_and_length;
})
UACPI_EXPECT_SIZEOF(struct acpi_small_item,  1);

UACPI_PACKED(struct acpi_resource_irq {
    struct acpi_small_item common;
    uacpi_u16 irq_mask;
    uacpi_u8 flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_irq, 4);

UACPI_PACKED(struct acpi_resource_dma {
    struct acpi_small_item common;
    uacpi_u8 channel_mask;
    uacpi_u8 flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_dma, 3);

UACPI_PACKED(struct acpi_resource_start_dependent {
    struct acpi_small_item common;
    uacpi_u8 flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_start_dependent, 2);

UACPI_PACKED(struct acpi_resource_end_dependent {
    struct acpi_small_item common;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_end_dependent, 1);

UACPI_PACKED(struct acpi_resource_io {
    struct acpi_small_item common;
    uacpi_u8 information;
    uacpi_u16 minimum;
    uacpi_u16 maximum;
    uacpi_u8 alignment;
    uacpi_u8 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_io, 8);

UACPI_PACKED(struct acpi_resource_fixed_io {
    struct acpi_small_item common;
    uacpi_u16 address;
    uacpi_u8 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_fixed_io, 4);

UACPI_PACKED(struct acpi_resource_fixed_dma {
    struct acpi_small_item common;
    uacpi_u16 request_line;
    uacpi_u16 channel;
    uacpi_u8 transfer_width;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_fixed_dma, 6);

UACPI_PACKED(struct acpi_resource_vendor_defined_type0 {
    struct acpi_small_item common;
    uacpi_u8 byte_data[];
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_vendor_defined_type0, 1);

UACPI_PACKED(struct acpi_resource_end_tag {
    struct acpi_small_item common;
    uacpi_u8 checksum;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_end_tag, 2);

UACPI_PACKED(struct acpi_large_item {
    uacpi_u8 type;
    uacpi_u16 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_large_item, 3);

UACPI_PACKED(struct acpi_resource_memory24 {
    struct acpi_large_item common;
    uacpi_u8 information;
    uacpi_u16 minimum;
    uacpi_u16 maximum;
    uacpi_u16 alignment;
    uacpi_u16 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_memory24, 12);

UACPI_PACKED(struct acpi_resource_vendor_defined_type1 {
    struct acpi_large_item common;
    uacpi_u8 byte_data[];
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_vendor_defined_type1, 3);

UACPI_PACKED(struct acpi_resource_memory32 {
    struct acpi_large_item common;
    uacpi_u8 information;
    uacpi_u32 minimum;
    uacpi_u32 maximum;
    uacpi_u32 alignment;
    uacpi_u32 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_memory32, 20);

UACPI_PACKED(struct acpi_resource_fixed_memory32 {
    struct acpi_large_item common;
    uacpi_u8 information;
    uacpi_u32 address;
    uacpi_u32 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_fixed_memory32, 12);

UACPI_PACKED(struct acpi_resource_address {
    struct acpi_large_item common;
    uacpi_u8 type;
    uacpi_u8 flags;
    uacpi_u8 type_flags;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_address, 6);

UACPI_PACKED(struct acpi_resource_address64 {
    struct acpi_resource_address common;
    uacpi_u64 granularity;
    uacpi_u64 minimum;
    uacpi_u64 maximum;
    uacpi_u64 translation_offset;
    uacpi_u64 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_address64, 46);

UACPI_PACKED(struct acpi_resource_address32 {
    struct acpi_resource_address common;
    uacpi_u32 granularity;
    uacpi_u32 minimum;
    uacpi_u32 maximum;
    uacpi_u32 translation_offset;
    uacpi_u32 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_address32, 26);

UACPI_PACKED(struct acpi_resource_address16 {
    struct acpi_resource_address common;
    uacpi_u16 granularity;
    uacpi_u16 minimum;
    uacpi_u16 maximum;
    uacpi_u16 translation_offset;
    uacpi_u16 length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_address16, 16);

UACPI_PACKED(struct acpi_resource_address64_extended {
    struct acpi_resource_address common;
    uacpi_u8 revision_id;
    uacpi_u8 rsvd;
    uacpi_u64 granularity;
    uacpi_u64 minimum;
    uacpi_u64 maximum;
    uacpi_u64 translation_offset;
    uacpi_u64 length;
    uacpi_u64 attributes;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_address64_extended, 56);

UACPI_PACKED(struct acpi_resource_extended_irq {
    struct acpi_large_item common;
    uacpi_u8 flags;
    uacpi_u8 num_irqs;
    uacpi_u32 irqs[];
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_extended_irq, 5);

UACPI_PACKED(struct acpi_resource_generic_register {
    struct acpi_large_item common;
    uacpi_u8 address_space_id;
    uacpi_u8 bit_width;
    uacpi_u8 bit_offset;
    uacpi_u8 access_size;
    uacpi_u64 address;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_generic_register, 15);

UACPI_PACKED(struct acpi_resource_gpio_connection {
    struct acpi_large_item common;
    uacpi_u8 revision_id;
    uacpi_u8 type;
    uacpi_u16 general_flags;
    uacpi_u16 connection_flags;
    uacpi_u8 pull_configuration;
    uacpi_u16 drive_strength;
    uacpi_u16 debounce_timeout;
    uacpi_u16 pin_table_offset;
    uacpi_u8 source_index;
    uacpi_u16 source_offset;
    uacpi_u16 vendor_data_offset;
    uacpi_u16 vendor_data_length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_gpio_connection, 23);

#define ACPI_SERIAL_TYPE_I2C 1
#define ACPI_SERIAL_TYPE_SPI 2
#define ACPI_SERIAL_TYPE_UART 3
#define ACPI_SERIAL_TYPE_CSI2 4
#define ACPI_SERIAL_TYPE_MAX ACPI_SERIAL_TYPE_CSI2

UACPI_PACKED(struct acpi_resource_serial {
    struct acpi_large_item common;
    uacpi_u8 revision_id;
    uacpi_u8 source_index;
    uacpi_u8 type;
    uacpi_u8 flags;
    uacpi_u16 type_specific_flags;
    uacpi_u8 type_specific_revision_id;
    uacpi_u16 type_data_length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_serial, 12);

UACPI_PACKED(struct acpi_resource_serial_i2c {
    struct acpi_resource_serial common;
    uacpi_u32 connection_speed;
    uacpi_u16 slave_address;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_serial_i2c, 18);

UACPI_PACKED(struct acpi_resource_serial_spi {
    struct acpi_resource_serial common;
    uacpi_u32 connection_speed;
    uacpi_u8 data_bit_length;
    uacpi_u8 phase;
    uacpi_u8 polarity;
    uacpi_u16 device_selection;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_serial_spi, 21);

UACPI_PACKED(struct acpi_resource_serial_uart {
    struct acpi_resource_serial common;
    uacpi_u32 baud_rate;
    uacpi_u16 rx_fifo;
    uacpi_u16 tx_fifo;
    uacpi_u8 parity;
    uacpi_u8 lines_enabled;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_serial_uart, 22);

UACPI_PACKED(struct acpi_resource_serial_csi2 {
    struct acpi_resource_serial common;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_serial_csi2, 12);

UACPI_PACKED(struct acpi_resource_pin_function {
    struct acpi_large_item common;
    uacpi_u8 revision_id;
    uacpi_u16 flags;
    uacpi_u8 pull_configuration;
    uacpi_u16 function_number;
    uacpi_u16 pin_table_offset;
    uacpi_u8 source_index;
    uacpi_u16 source_offset;
    uacpi_u16 vendor_data_offset;
    uacpi_u16 vendor_data_length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_pin_function, 18);

UACPI_PACKED(struct acpi_resource_pin_configuration {
    struct acpi_large_item common;
    uacpi_u8 revision_id;
    uacpi_u16 flags;
    uacpi_u8 type;
    uacpi_u32 value;
    uacpi_u16 pin_table_offset;
    uacpi_u8 source_index;
    uacpi_u16 source_offset;
    uacpi_u16 vendor_data_offset;
    uacpi_u16 vendor_data_length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_pin_configuration, 20);

UACPI_PACKED(struct acpi_resource_pin_group {
    struct acpi_large_item common;
    uacpi_u8 revision_id;
    uacpi_u16 flags;
    uacpi_u16 pin_table_offset;
    uacpi_u16 source_lable_offset;
    uacpi_u16 vendor_data_offset;
    uacpi_u16 vendor_data_length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_pin_group, 14);

UACPI_PACKED(struct acpi_resource_pin_group_function {
    struct acpi_large_item common;
    uacpi_u8 revision_id;
    uacpi_u16 flags;
    uacpi_u16 function;
    uacpi_u8 source_index;
    uacpi_u16 source_offset;
    uacpi_u16 source_lable_offset;
    uacpi_u16 vendor_data_offset;
    uacpi_u16 vendor_data_length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_pin_group_function, 17);

UACPI_PACKED(struct acpi_resource_pin_group_configuration {
    struct acpi_large_item common;
    uacpi_u8 revision_id;
    uacpi_u16 flags;
    uacpi_u8 type;
    uacpi_u32 value;
    uacpi_u8 source_index;
    uacpi_u16 source_offset;
    uacpi_u16 source_lable_offset;
    uacpi_u16 vendor_data_offset;
    uacpi_u16 vendor_data_length;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_pin_group_configuration, 20);

UACPI_PACKED(struct acpi_resource_clock_input {
    struct acpi_large_item common;
    uacpi_u8 revision_id;
    uacpi_u16 flags;
    uacpi_u16 divisor;
    uacpi_u32 numerator;
    uacpi_u8 source_index;
})
UACPI_EXPECT_SIZEOF(struct acpi_resource_clock_input, 13);
